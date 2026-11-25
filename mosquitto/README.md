# Mosquitto

El broker recibe las publicaciones del ESP32 y las distribuye a los clientes suscritos. Puerto documentado: TCP 1883.

1. Copia `mosquitto.example.conf` a `mosquitto.conf`.
2. Crea localmente `passwords.txt` con las cuentas `pulso_esp32` y `pulso_dashboard` usando `mosquitto_passwd`.
3. Copia `acl.example.txt` a `acl.txt`.
4. Ajusta las rutas de la configuración al directorio de instalación.
5. Restringe el firewall al equipo ESP32 y a la red privada/hotspot que corresponda.

No publiques `passwords.txt`, contraseñas ni volcados de configuración con secretos. La sintaxis puede requerir adaptación a la versión/instalación de Mosquitto usada en Windows.
