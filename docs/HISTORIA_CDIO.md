# Historia del proyecto bajo CDIO

## C — Concepción
La propuesta inicial “Cuida Tu Salud” buscaba monitorizar frecuencia cardíaca y saturación de oxígeno en deportistas. Se presentaron matrices de selección para proyecto, sensor, tarjeta de desarrollo e IDE. La matriz inicial consideró MAX30100 y VS Code; la implementación final documentada evolucionó hacia XD-58C, ESP32 DevKit V1 y Arduino IDE.

## D — Diseño
Se definió la cadena de adquisición PPG, adaptación de señal, ADC, filtrado y cálculo de BPM. En paralelo se diseñó comunicación cliente-servidor con Wi-Fi, direcciones IP, MQTT, Mosquitto y Node-RED.

## I — Implementación
Se montó el circuito con ESP32, sensor y divisor resistivo; se realizaron pruebas de señal con osciloscopio, lectura desde Arduino IDE, monitor serie y Serial Plotter. Se integraron firmware, broker, autenticación y flujo Node-RED.

## O — Operación
La interfaz dashboard presentó BPM, estado, calidad y tendencia temporal. Las evidencias muestran una lectura de 72.3 BPM en una captura y 76.4 BPM / QUALITY 2 en otra. Son registros de pruebas, no un estudio de exactitud clínica.
