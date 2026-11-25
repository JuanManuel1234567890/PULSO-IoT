# Conexiones del prototipo

| Origen | Destino | Nota |
|---|---|---|
| XD-58C salida S | R1 = 10 kΩ | R1 queda entre la salida S y el nodo ADC. |
| Nodo entre R1 y R2 | ESP32 GPIO34 | Entrada analógica PPG. |
| Nodo entre R1 y R2 | R2 = 22 kΩ | R2 conecta el nodo con GND. |
| R2 | GND común | Referencia común sensor/ESP32. |
| Pulsador de presencia | GPIO27 y GND | INPUT_PULLUP; activo en LOW; debe permanecer accionado durante la medición documentada. |
| Interruptor de sesión | GPIO26 y GND | INPUT_PULLUP; activo en LOW. |

## Divisor resistivo

`V_GPIO34 = V_sensor × R2/(R1+R2) = V_sensor × 22/32 ≈ 0.6875 V_sensor`.

El divisor no sustituye la comprobación de los límites absolutos del ADC. Verifica la alimentación y la amplitud máxima real antes de conectar el sensor.
