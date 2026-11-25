# Crear el repositorio remoto y publicar PULSO-IoT

## 1. Crear un repositorio vacío

1. Entra a https://github.com/new con la cuenta donde quieres alojar el proyecto.
2. Nombre recomendado: `PULSO-IoT`.
3. Descripción: `Sistema académico de adquisición y monitorización de pulso cardíaco mediante PPG e IoT`.
4. Visibilidad recomendada inicialmente: **Private**. Se puede cambiar a Public cuando todos los integrantes autoricen la publicación de fotos y documentos.
5. No marques `Add a README`, `.gitignore` ni licencia; el paquete ya contiene esos archivos.
6. Pulsa **Create repository**.

## 2. Extraer y publicar el paquete

1. Descarga `PULSO-IoT_GitHub_Starter.zip` y extráelo en una carpeta local.
2. Instala Git for Windows si todavía no está instalado: https://git-scm.com/download/win.
3. Abre la carpeta extraída y ejecuta `PUBLICAR_EN_GITHUB.bat`.
4. Copia la URL HTTPS que GitHub muestra en la página del repositorio recién creado (formato `https://github.com/USUARIO/PULSO-IoT.git`) y pégala cuando el script la solicite.
5. Introduce nombre y correo para los commits. Puedes utilizar el correo privado `noreply` que GitHub ofrece en Settings → Emails.
6. Sigue el proceso de autenticación de Git Credential Manager / navegador si Git lo solicita.
7. Al finalizar, abre la URL del repositorio y verifica que se vean carpetas, README, documentos y evidencias.

No introduzcas credenciales MQTT ni Wi-Fi en el script; no las solicita.

## 3. Completar la parte Node-RED

El archivo `node-red/flows.json` está pendiente porque no venía incluido entre los archivos entregados. En el editor Node-RED real, usa ☰ → Export → selecciona el flujo del proyecto → copia el JSON y guárdalo como `node-red/flows.json`. Revisa que no incluya credenciales. Nunca publiques `flows_cred.json`.

## 4. Añadir versiones de software

En `docs/` documenta las versiones exactas utilizadas de Arduino IDE, ESP32 Arduino Core, PubSubClient, Mosquitto y Node-RED. No se han supuesto ni inventado en este paquete.

## 5. Permisos del equipo y licencia

Antes de hacerlo público, confirma que los integrantes autorizan publicar informe, presentación y fotografías. El repositorio no incorpora todavía una licencia abierta porque no se ha confirmado el consentimiento de todos los titulares.
