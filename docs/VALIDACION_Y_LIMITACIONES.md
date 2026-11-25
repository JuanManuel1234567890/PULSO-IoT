# Pruebas, resultados y limitaciones

## Pruebas documentadas

- Observación de señal PPG mediante osciloscopio Rigol MSO1104Z.
- Inspección del circuito montado con ESP32 y sensor XD-58C.
- Lectura de variables por Monitor Serie: `PPG`, `TH`, `BPM`, `QUALITY`.
- Representación en Serial Plotter de BPM y PPG.
- Comprobación de publicaciones/recepción en la consola Mosquitto ejecutada desde PowerShell.
- Inspección de flujo Node-RED y visualización de BPM, estado, calidad y tendencia en dashboard.

## Parámetros del firmware

- FS = 100 Hz; Ts = 10 ms.
- ADC 12 bits y `ADC_11db`.
- HP = 0.4 Hz; LP = 5 Hz.
- Umbral mínimo = 60 mV, adaptativo.
- IBI = 300–2000 ms; ancho de pulso = 40–600 ms.
- Historial máximo = 5 intervalos; CV máximo = 0.20.
- Timeout de señal = 2500 ms; estabilización inicial = 1500 ms.
- MQTT cada 5000 ms.

## Limitaciones

La señal se ve afectada por presión, posición del dedo, movimiento y luz ambiente. No se documenta calibración metrológica ni validación estadística frente a equipo de referencia; tampoco se implementó SpO₂ en esta versión. La condición `VALID` describe el cumplimiento del algoritmo, no una certificación de precisión médica.
