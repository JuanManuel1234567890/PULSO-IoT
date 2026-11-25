# Seguridad y tratamiento de credenciales

- No subir contraseñas Wi-Fi, claves MQTT, tokens, archivos `passwords.txt`, `config.h` ni `flows_cred.json`.
- Utilizar `firmware/config.example.h` como plantilla y mantener `config.h` solo localmente.
- Si una contraseña real se publica accidentalmente, revocarla/cambiarla; borrar el commit no invalida una credencial ya expuesta.
- No abrir indiscriminadamente el puerto 1883 a Internet. El diseño documentado es de laboratorio/red local y MQTT 1883 no cifra el tráfico.
