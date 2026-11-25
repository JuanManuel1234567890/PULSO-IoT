/************************************************************
 * PROYECTO: PULSO-IoT
 * VERSION: 0.9 - Firmware PPG + MQTT
 *
 * HARDWARE:
 *   ESP32 DevKit V1
 *   Sensor XD-58C
 *
 * GPIO34: Entrada analogica PPG (a traves del divisor resistivo)
 * GPIO27: Presencia fisica del dedo (INPUT_PULLUP)
 * GPIO26: Switch ON/OFF de sesion (INPUT_PULLUP)
 *
 * ADQUISICION:
 *   100 Hz - ADC de 12 bits
 *
 * PROCESAMIENTO:
 *   Filtro HP 0.4 Hz
 *   Filtro LP 5 Hz
 *   Deteccion de maximos
 *   Validacion temporal y de amplitud
 *   Coeficiente de variacion (CV)
 *
 * IoT:
 *   Wi-Fi: credenciales locales definidas en config.h (no versionado)
 *   MQTT: Mosquitto en la IPv4 local del computador, puerto 1883
 *   Topico: pulso-iot/device01/telemetry
 *   Publicacion: cada 5 segundos
 *
 * IMPORTANTE:
 *   - La adquisicion y el procesamiento PPG se ejecutan en loop().
 *   - MQTT se ejecuta en una tarea FreeRTOS independiente, fijada
 *     al nucleo 0. El loop() de Arduino se ejecuta normalmente en
 *     el nucleo 1 del ESP32 clasico.
 *   - quality_code es una categoria 0..3, NO un porcentaje.
 ************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>
#include <stdio.h>
#include "config.h"  // Credenciales y direccion local: archivo excluido de Git

// ========================================================
// 1. CONFIGURACION GENERAL Y HARDWARE
// ========================================================

constexpr uint8_t PIN_PPG = 34;
constexpr uint8_t PIN_DEDO = 27;
constexpr uint8_t PIN_SESION = 26;

constexpr float FS = 100.0f;
constexpr uint32_t SAMPLE_US = 10000UL;

constexpr uint32_t DEBOUNCE_MS = 35;
constexpr uint32_t SETTLE_MS = 1500;

// Filtros digitales
constexpr float FC_HP = 0.4f;
constexpr float FC_LP = 5.0f;

float alphaHP = 0.0f;
float alphaLP = 0.0f;

// ========================================================
// 2. CONFIGURACION WI-FI Y MQTT
// ========================================================

// Las credenciales y la IP del broker se definen en config.h (no versionado).
constexpr uint16_t MQTT_PORT = 1883;

const char* MQTT_TOPIC = "pulso-iot/device01/telemetry";

constexpr uint32_t MQTT_PUBLISH_INTERVAL_MS = 5000UL;
constexpr uint32_t MQTT_RETRY_INTERVAL_MS = 3000UL;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 10000UL;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

char mqttClientId[32];

// ========================================================
// 3. ESTRUCTURA COMPARTIDA DE TELEMETRIA
// ========================================================

// Codigos categoricos de calidad de deteccion:
// 0 = sin medicion disponible
// 1 = buscando / adquiriendo pulsaciones
// 2 = medicion validada
// 3 = señal inestable

enum QualityCode : uint8_t {
  QUALITY_NONE = 0,
  QUALITY_SEARCHING = 1,
  QUALITY_VALID = 2,
  QUALITY_UNSTABLE = 3
};

struct TelemetrySnapshot {
  uint32_t session;
  uint32_t uptimeMs;
  uint8_t state;
  uint8_t qualityCode;
  float bpm;
  float filteredMv;
  float thresholdMv;
  bool valid;
};

TelemetrySnapshot telemetrySnapshot = {};
portMUX_TYPE telemetryMux = portMUX_INITIALIZER_UNLOCKED;

uint32_t sessionCounter = 0;

const char* stateName(uint8_t value) {
  switch (value) {
    case 0: return "OFF";
    case 1: return "WAITING";
    case 2: return "STABILIZING";
    case 3: return "SEARCHING";
    case 4: return "VALID";
    case 5: return "UNSTABLE";
    default: return "UNKNOWN";
  }
}

const char* qualityName(uint8_t value) {
  switch (value) {
    case QUALITY_NONE: return "NO_DATA";
    case QUALITY_SEARCHING: return "SEARCHING";
    case QUALITY_VALID: return "VALID";
    case QUALITY_UNSTABLE: return "UNSTABLE";
    default: return "UNKNOWN";
  }
}

void updateTelemetrySnapshot(uint32_t nowMs);
void mqttWorker(void* parameter);

// ========================================================
// 4. CONTROLES CON ANTIRREBOTE
// ========================================================

class DebouncedInput {

  private:
    uint8_t pin;
    bool raw;
    bool stable;
    uint32_t changeTime;

  public:
    explicit DebouncedInput(uint8_t p) : pin(p) {}

    void begin() {
      pinMode(pin, INPUT_PULLUP);
      raw = digitalRead(pin);
      stable = raw;
      changeTime = millis();
    }

    void update() {
      bool reading = digitalRead(pin);

      if (reading != raw) {
        raw = reading;
        changeTime = millis();
      }

      if (millis() - changeTime >= DEBOUNCE_MS) {
        stable = raw;
      }
    }

    bool active() const {
      return stable == LOW;
    }
};

DebouncedInput fingerButton(PIN_DEDO);
DebouncedInput sessionSwitch(PIN_SESION);

// ========================================================
// 5. ESTADOS DEL DISPOSITIVO
// ========================================================

enum DeviceState : uint8_t {
  OFF = 0,
  WAITING = 1,
  STABILIZING = 2,
  SEARCHING = 3,
  VALID = 4,
  UNSTABLE = 5
};

DeviceState state = OFF;

bool previousSession = false;
bool previousFinger = false;

uint32_t fingerStartTime = 0;

uint8_t qualityCode = QUALITY_NONE;
float latestFiltered = 0.0f;

// ========================================================
// 6. FILTRADO DIGITAL
// ========================================================

float previousInput = 0.0f;
float previousHP = 0.0f;
float previousLP = 0.0f;

bool filterReady = false;

void resetFilter() {
  previousInput = 0.0f;
  previousHP = 0.0f;
  previousLP = 0.0f;
  filterReady = false;
}

float filterPPG(float input) {

  if (!filterReady) {
    previousInput = input;
    previousHP = 0.0f;
    previousLP = 0.0f;
    filterReady = true;
  }

  float hp = alphaHP * (previousHP + input - previousInput);
  float lp = previousLP + alphaLP * (hp - previousLP);

  previousInput = input;
  previousHP = hp;
  previousLP = lp;

  return lp;
}

// ========================================================
// 7. PARAMETROS DEL DETECTOR
// ========================================================

constexpr float MIN_THRESHOLD_MV = 60.0f;

constexpr uint32_t MIN_IBI_MS = 300;
constexpr uint32_t MAX_IBI_MS = 2000;

constexpr uint32_t MAX_BEAT_WIDTH_MS = 600;
constexpr uint32_t MIN_BEAT_WIDTH_MS = 40;

constexpr uint32_t SIGNAL_TIMEOUT_MS = 2500;
constexpr float MAX_CV = 0.20f;

bool inPulse = false;

float pulsePeak = 0.0f;
uint32_t pulsePeakTime = 0;
uint32_t pulseStartTime = 0;

float previousFiltered = 0.0f;
float positiveEnvelope = 0.0f;
float threshold = MIN_THRESHOLD_MV;

// ========================================================
// 8. HISTORIAL DE INTERVALOS
// ========================================================

constexpr uint8_t HISTORY_SIZE = 5;

uint32_t intervals[HISTORY_SIZE];
float beatAmplitudes[HISTORY_SIZE];

uint8_t intervalCount = 0;
uint8_t intervalIndex = 0;

uint8_t amplitudeCount = 0;
uint8_t amplitudeIndex = 0;

uint32_t lastBeatTime = 0;
bool hasLastBeat = false;

float currentBPM = 0.0f;
uint32_t unstableUntil = 0;

// ========================================================
// 9. FUNCIONES ESTADISTICAS
// ========================================================

void clearHistory() {
  intervalCount = 0;
  intervalIndex = 0;

  amplitudeCount = 0;
  amplitudeIndex = 0;

  currentBPM = 0.0f;
}

float medianFloat(const float* values, uint8_t n) {

  float temp[HISTORY_SIZE];

  for (uint8_t i = 0; i < n; i++) {
    temp[i] = values[i];
  }

  for (uint8_t i = 0; i < n; i++) {
    for (uint8_t j = i + 1; j < n; j++) {
      if (temp[j] < temp[i]) {
        float t = temp[i];
        temp[i] = temp[j];
        temp[j] = t;
      }
    }
  }

  if (n % 2 == 1) {
    return temp[n / 2];
  }

  return (temp[n / 2 - 1] + temp[n / 2]) / 2.0f;
}

float medianIntervals() {

  float temp[HISTORY_SIZE];

  for (uint8_t i = 0; i < intervalCount; i++) {
    temp[i] = (float)intervals[i];
  }

  for (uint8_t i = 0; i < intervalCount; i++) {
    for (uint8_t j = i + 1; j < intervalCount; j++) {
      if (temp[j] < temp[i]) {
        float t = temp[i];
        temp[i] = temp[j];
        temp[j] = t;
      }
    }
  }

  if (intervalCount % 2 == 1) {
    return temp[intervalCount / 2];
  }

  return (temp[intervalCount / 2 - 1] + temp[intervalCount / 2]) / 2.0f;
}

float calculateCV() {

  if (intervalCount < 2) return 1.0f;

  float mean = 0.0f;

  for (uint8_t i = 0; i < intervalCount; i++) {
    mean += intervals[i];
  }

  mean /= intervalCount;

  float variance = 0.0f;

  for (uint8_t i = 0; i < intervalCount; i++) {
    float difference = intervals[i] - mean;
    variance += difference * difference;
  }

  variance /= intervalCount;

  float standardDeviation = sqrtf(variance);
  return standardDeviation / mean;
}

// ========================================================
// 10. REGISTRO DE PULSACIONES
// ========================================================

void registerBeat(uint32_t beatTime, float amplitude) {

  if (!hasLastBeat) {
    lastBeatTime = beatTime;
    hasLastBeat = true;

    beatAmplitudes[amplitudeIndex] = amplitude;
    amplitudeIndex = (amplitudeIndex + 1) % HISTORY_SIZE;
    amplitudeCount = 1;

    return;
  }

  uint32_t ibi = beatTime - lastBeatTime;
  lastBeatTime = beatTime;

  if (ibi < MIN_IBI_MS || ibi > MAX_IBI_MS) {
    clearHistory();
    unstableUntil = beatTime + 1000;

    beatAmplitudes[amplitudeIndex] = amplitude;
    amplitudeIndex = (amplitudeIndex + 1) % HISTORY_SIZE;
    amplitudeCount = 1;

    return;
  }

  if (amplitudeCount >= 3) {
    float referenceAmplitude = medianFloat(beatAmplitudes, amplitudeCount);
    float ratio = amplitude / referenceAmplitude;

    if (ratio < 0.35f || ratio > 2.5f) {
      clearHistory();
      unstableUntil = beatTime + 1000;

      beatAmplitudes[0] = amplitude;
      amplitudeCount = 1;
      amplitudeIndex = 1;

      return;
    }
  }

  beatAmplitudes[amplitudeIndex] = amplitude;
  amplitudeIndex = (amplitudeIndex + 1) % HISTORY_SIZE;

  if (amplitudeCount < HISTORY_SIZE) {
    amplitudeCount++;
  }

  intervals[intervalIndex] = ibi;
  intervalIndex = (intervalIndex + 1) % HISTORY_SIZE;

  if (intervalCount < HISTORY_SIZE) {
    intervalCount++;
  }

  if (intervalCount >= 3) {
    float cv = calculateCV();

    if (cv <= MAX_CV) {
      float medianIBI = medianIntervals();
      currentBPM = 60000.0f / medianIBI;
    } else {
      clearHistory();

      beatAmplitudes[0] = amplitude;
      amplitudeCount = 1;
      amplitudeIndex = 1;

      unstableUntil = beatTime + 1000;
    }
  }
}

// ========================================================
// 11. DETECTOR DE MAXIMOS
// ========================================================

void detectPeaks(float signal, uint32_t timeMs) {

  if (!inPulse) {
    positiveEnvelope *= 0.995f;
  }

  threshold = fmaxf(MIN_THRESHOLD_MV, positiveEnvelope * 0.45f);

  if (!inPulse && signal > threshold && previousFiltered <= threshold) {
    inPulse = true;

    pulseStartTime = timeMs;
    pulsePeak = signal;
    pulsePeakTime = timeMs;
  }
  else if (inPulse) {

    if (signal > pulsePeak) {
      pulsePeak = signal;
      pulsePeakTime = timeMs;
    }

    uint32_t duration = timeMs - pulseStartTime;

    if (duration > MAX_BEAT_WIDTH_MS + 100) {
      inPulse = false;
      unstableUntil = timeMs + 1000;
    }
    else if (duration >= MIN_BEAT_WIDTH_MS && signal < pulsePeak * 0.55f) {

      uint32_t width = timeMs - pulseStartTime;

      if (pulsePeak >= MIN_THRESHOLD_MV &&
          width >= MIN_BEAT_WIDTH_MS &&
          width <= MAX_BEAT_WIDTH_MS) {

        positiveEnvelope = fmaxf(positiveEnvelope, pulsePeak);
        registerBeat(pulsePeakTime, pulsePeak);
      }

      inPulse = false;
    }
  }

  previousFiltered = signal;
}

// ========================================================
// 12. REINICIO DE MEDICION
// ========================================================

void resetMeasurement() {

  resetFilter();
  clearHistory();

  inPulse = false;
  pulsePeak = 0.0f;
  pulsePeakTime = 0;
  pulseStartTime = 0;

  previousFiltered = 0.0f;
  positiveEnvelope = 0.0f;
  threshold = MIN_THRESHOLD_MV;

  hasLastBeat = false;
  lastBeatTime = 0;
  unstableUntil = 0;

  qualityCode = QUALITY_NONE;
  latestFiltered = 0.0f;
}

// ========================================================
// 13. ACTUALIZACION DE LA COPIA DE TELEMETRIA
// ========================================================

void updateTelemetrySnapshot(uint32_t nowMs) {

  TelemetrySnapshot local;

  local.session = sessionCounter;
  local.uptimeMs = nowMs;
  local.state = (uint8_t)state;
  local.qualityCode = qualityCode;
  local.valid = (qualityCode == QUALITY_VALID && currentBPM > 0.0f);
  local.bpm = local.valid ? currentBPM : 0.0f;
  local.filteredMv = latestFiltered;
  local.thresholdMv = threshold;

  portENTER_CRITICAL(&telemetryMux);
  telemetrySnapshot = local;
  portEXIT_CRITICAL(&telemetryMux);
}

// ========================================================
// 14. TAREA MQTT (CORE 0)
// ========================================================

void mqttWorker(void* parameter) {

  uint32_t lastMqttAttempt = 0;
  uint32_t lastWifiAttempt = millis();
  uint32_t lastPublish = 0;

  for (;;) {

    uint32_t now = millis();

    // ----------------------------------------------------
    // Gestion de Wi-Fi sin bloquear el loop de adquisicion
    // ----------------------------------------------------

    if (WiFi.status() != WL_CONNECTED) {

      if (mqttClient.connected()) {
        mqttClient.disconnect();
      }

      if (now - lastWifiAttempt >= WIFI_RETRY_INTERVAL_MS) {
        lastWifiAttempt = now;
        WiFi.reconnect();
      }

      vTaskDelay(pdMS_TO_TICKS(250));
      continue;
    }

    // ----------------------------------------------------
    // Conexion MQTT: los intentos ocurren en esta tarea,
    // nunca dentro del loop de adquisicion PPG.
    // ----------------------------------------------------

    if (!mqttClient.connected()) {

      if (lastMqttAttempt == 0 ||
          now - lastMqttAttempt >= MQTT_RETRY_INTERVAL_MS) {

        lastMqttAttempt = now;

        if (mqttClient.connect(mqttClientId, MQTT_USER, MQTT_PASSWORD)) {
          lastPublish = 0; // publicar un primer estado al conectar
        }
      }
    }

    // ----------------------------------------------------
    // Servicio MQTT y publicacion periodica
    // ----------------------------------------------------

    if (mqttClient.connected()) {

      mqttClient.loop();

      now = millis();

      if (now - lastPublish >= MQTT_PUBLISH_INTERVAL_MS) {

        TelemetrySnapshot local;

        portENTER_CRITICAL(&telemetryMux);
        local = telemetrySnapshot;
        portEXIT_CRITICAL(&telemetryMux);

        int rssi = WiFi.RSSI();
        const char* stateText = stateName(local.state);
        const char* qualityText = qualityName(local.qualityCode);

        char payload[320];

        int length = snprintf(
          payload,
          sizeof(payload),
          "{\"device\":\"PULSO-IoT-01\","
          "\"source\":\"LIVE\","
          "\"session\":%lu,"
          "\"uptime_ms\":%lu,"
          "\"state\":\"%s\","
          "\"bpm\":%.1f,"
          "\"quality_code\":%u,"
          "\"quality_label\":\"%s\","
          "\"valid\":%s,"
          "\"ppg_filtered_mV\":%.1f,"
          "\"threshold_mV\":%.1f,"
          "\"sample_rate_hz\":100,"
          "\"rssi_dBm\":%d}",
          (unsigned long)local.session,
          (unsigned long)local.uptimeMs,
          stateText,
          local.bpm,
          (unsigned int)local.qualityCode,
          qualityText,
          local.valid ? "true" : "false",
          local.filteredMv,
          local.thresholdMv,
          rssi
        );

        if (length > 0 && length < (int)sizeof(payload)) {
          mqttClient.publish(MQTT_TOPIC, payload, false);
        }

        lastPublish = now;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ========================================================
// 15. INICIALIZACION
// ========================================================

uint32_t nextSampleUs = 0;

void setup() {

  Serial.begin(115200);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_PPG, ADC_11db);

  fingerButton.begin();
  sessionSwitch.begin();

  alphaHP = expf(-2.0f * PI * FC_HP / FS);
  alphaLP = 1.0f - expf(-2.0f * PI * FC_LP / FS);

  previousSession = sessionSwitch.active();
  previousFinger = fingerButton.active();

  if (previousSession) {
    sessionCounter = 1;
  }

  if (previousSession && previousFinger) {
    fingerStartTime = millis();
  }

  // Identificador MQTT derivado de la MAC para evitar colisiones.
  uint64_t chipId = ESP.getEfuseMac();
  snprintf(
    mqttClientId,
    sizeof(mqttClientId),
    "PULSO_IoT_%04X%08X",
    (uint16_t)(chipId >> 32),
    (uint32_t)chipId
  );

  // Configuracion del cliente MQTT.
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setKeepAlive(30);
  mqttClient.setSocketTimeout(2);
  mqttClient.setBufferSize(384);

  // Inicio Wi-Fi no bloqueante.
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  nextSampleUs = micros() + SAMPLE_US;

  // MQTT corre en el core 0; loop() conserva la adquisicion PPG.
  xTaskCreatePinnedToCore(
    mqttWorker,
    "PULSO_MQTT",
    6144,
    nullptr,
    1,
    nullptr,
    0
  );

  updateTelemetrySnapshot(millis());
}

// ========================================================
// 16. BUCLE PRINCIPAL: ADQUISICION Y PROCESAMIENTO PPG
// ========================================================

void loop() {

  fingerButton.update();
  sessionSwitch.update();

  bool sessionOn = sessionSwitch.active();
  bool fingerPresent = fingerButton.active();

  uint32_t nowMs = millis();

  // Detectar cambios en controles y reiniciar la medicion.
  if (sessionOn != previousSession || fingerPresent != previousFinger) {

    if (sessionOn && !previousSession) {
      sessionCounter++;
    }

    resetMeasurement();

    if (sessionOn && fingerPresent) {
      fingerStartTime = nowMs;
    }

    previousSession = sessionOn;
    previousFinger = fingerPresent;
  }

  // Estado base determinado por los controles y la estabilizacion.
  if (!sessionOn) {
    state = OFF;
    qualityCode = QUALITY_NONE;
  }
  else if (!fingerPresent) {
    state = WAITING;
    qualityCode = QUALITY_NONE;
  }
  else if (nowMs - fingerStartTime < SETTLE_MS) {
    state = STABILIZING;
    qualityCode = QUALITY_NONE;
  }
  else if (state == OFF || state == WAITING || state == STABILIZING) {
    state = SEARCHING;
    qualityCode = QUALITY_SEARCHING;
  }

  // Temporizacion de adquisicion a 100 Hz.
  uint32_t nowUs = micros();

  if ((int32_t)(nowUs - nextSampleUs) >= 0) {

    uint32_t sampleTimeUs = micros();
    nextSampleUs += SAMPLE_US;

    // Si el programa se retraso mas de un periodo, evita acumular
    // muestras atrasadas artificiales y reinicia el siguiente plazo.
    if ((int32_t)(sampleTimeUs - nextSampleUs) >= (int32_t)SAMPLE_US) {
      nextSampleUs = sampleTimeUs + SAMPLE_US;
    }

    uint32_t sampleTimeMs = millis();

    uint32_t raw = analogReadMilliVolts(PIN_PPG);
    float filtered = 0.0f;

    bool ready = sessionOn && fingerPresent &&
                 (sampleTimeMs - fingerStartTime >= SETTLE_MS);

    if (ready) {

      filtered = filterPPG((float)raw);
      latestFiltered = filtered;

      detectPeaks(filtered, sampleTimeMs);

      // Expiracion si dejan de detectarse pulsaciones.
      if (hasLastBeat &&
          sampleTimeMs - lastBeatTime > SIGNAL_TIMEOUT_MS) {

        clearHistory();
        hasLastBeat = false;
        currentBPM = 0.0f;
        inPulse = false;
        positiveEnvelope = 0.0f;
      }

      // Clasificacion de la deteccion; no es un porcentaje de calidad.
      if (sampleTimeMs < unstableUntil) {
        qualityCode = QUALITY_UNSTABLE;
        state = UNSTABLE;
      }
      else if (intervalCount >= 3) {

        if (calculateCV() <= MAX_CV && currentBPM > 0.0f) {
          qualityCode = QUALITY_VALID;
          state = VALID;
        }
        else {
          qualityCode = QUALITY_UNSTABLE;
          state = UNSTABLE;
        }
      }
      else {
        qualityCode = QUALITY_SEARCHING;
        state = SEARCHING;
      }
    }
    else {
      latestFiltered = 0.0f;
      qualityCode = QUALITY_NONE;
    }

    // BPM visible unicamente cuando la deteccion esta validada.
    float displayedBPM =
      (qualityCode == QUALITY_VALID) ? currentBPM : 0.0f;

    // Salida compatible con Arduino Serial Plotter.
    Serial.print("PPG:");
    Serial.print(filtered, 1);

    Serial.print("\tTH:");
    Serial.print(threshold, 1);

    Serial.print("\tBPM:");
    Serial.print(displayedBPM, 1);

    Serial.print("\tQUALITY:");
    Serial.println((unsigned int)qualityCode);
  }

  // La tarea MQTT lee esta copia protegida, no las variables internas
  // del detector. Se actualiza continuamente sin bloquear la adquisicion.
  updateTelemetrySnapshot(millis());
}
