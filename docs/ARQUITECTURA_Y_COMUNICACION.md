# Arquitectura y comunicación

## Flujo de extremo a extremo

1. El XD-58C genera una señal analógica PPG.
2. Un divisor 10 kΩ / 22 kΩ adapta la señal al nodo de lectura GPIO34.
3. El ADC del ESP32 toma muestras de 12 bits a 100 Hz.
4. El firmware filtra la señal mediante HP 0.4 Hz y LP 5 Hz.
5. El detector identifica picos, calcula IBI y estima BPM.
6. El firmware clasifica la adquisición mediante estados y `quality_code`.
7. Una tarea FreeRTOS independiente gestiona Wi-Fi/MQTT y publica telemetría cada 5 s.
8. Mosquitto actúa como broker; Node-RED se suscribe al topic y procesa el JSON.
9. El dashboard representa BPM, estado, calidad y tendencia.

## Distinciones

- El muestreo interno (100 Hz) no es el intervalo de publicación MQTT (5 s).
- MQTT organiza la mensajería; Mosquitto es la implementación del broker.
- IP identifica al equipo en la red; TCP 1883 identifica el servicio MQTT del broker.
- El firewall controla el tráfico que puede alcanzar el puerto; no interpreta mensajes MQTT.
- `QUALITY` es un código categórico 0–3, no un porcentaje.
