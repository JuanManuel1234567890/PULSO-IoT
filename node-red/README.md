# Node-RED

El flujo documentado recibe telemetría MQTT desde `pulso-iot/device01/telemetry`, convierte el payload JSON y distribuye cuatro salidas: BPM válido, estado del dispositivo, código de calidad y evolución de BPM. También se usaron nodos Debug para verificar los datos durante la integración.

## Añadir el flujo real al repositorio

Desde el editor Node-RED utilizado en el proyecto: menú ☰ → Export → selecciona el flujo correspondiente → copia el JSON exportado y guárdalo aquí como `flows.json`. Antes de confirmar el commit, revisa que no contenga contraseñas, tokens, nombres/IP privadas ni credenciales embebidas. No publiques `flows_cred.json`.

La exportación original no estaba entre los archivos fuente recibidos para preparar este repositorio; por eso no se incluye un JSON inventado.

## Configuración MQTT

- Broker: IP local actual del computador.
- Puerto: 1883.
- Topic: `pulso-iot/device01/telemetry`.
- Usuario: `pulso_dashboard` (solo lectura).
- El dashboard se sirve desde Node-RED en la red local; la dirección `localhost` solo funciona desde el propio computador.
