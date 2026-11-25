# Firmware ESP32

Firmware v0.9 de PULSO-IoT para ESP32 DevKit V1 y sensor XD-58C.

## Preparación

1. Instala Arduino IDE y el soporte de placas ESP32.
2. Instala la biblioteca **PubSubClient** desde el Library Manager.
3. Copia `config.example.h` como `config.h`.
4. Completa en `config.h` el SSID, contraseña Wi-Fi, usuario/contraseña MQTT y la IP del computador que ejecuta Mosquitto.
5. Abre `PULSO_IoT_v0_9_MQTT.ino`, selecciona tu placa ESP32 y carga el firmware.

`config.h` está excluido de Git mediante `.gitignore`. Nunca subas credenciales reales.

## Parámetros documentados

- Entrada PPG: GPIO34.
- Pulsador de presencia de dedo: GPIO27 (INPUT_PULLUP, activo en LOW).
- Interruptor de sesión: GPIO26 (INPUT_PULLUP, activo en LOW).
- ADC: 12 bits, atenuación ADC_11db.
- Muestreo: 100 Hz (10 ms).
- Filtros IIR: pasa-altas 0.4 Hz y pasa-bajas 5 Hz.
- Umbral mínimo: 60 mV; el umbral operativo es adaptativo.
- IBI aceptado: 300–2000 ms.
- Ancho de pulso aceptado: 40–600 ms.
- Historial: hasta cinco intervalos; CV máximo 20 %.
- Publicación MQTT: cada 5 s.

## Importante

El programa estima BPM a partir de PPG. No implementa cálculo de SpO₂ ni constituye un dispositivo médico.
