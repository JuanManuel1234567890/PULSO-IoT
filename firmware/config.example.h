#pragma once
#include <Arduino.h>

// Copia este archivo como config.h y completa los valores de tu red.
// NO publiques config.h: contiene credenciales privadas.

const char* WIFI_SSID = "NOMBRE_DE_TU_RED_2_4_GHZ";
const char* WIFI_PASSWORD = "CLAVE_WIFI";

const char* MQTT_USER = "pulso_esp32";
const char* MQTT_PASSWORD = "CLAVE_MQTT_DEL_USUARIO_ESP32";

// Dirección IPv4 del computador donde se ejecuta Mosquitto.
// Sustituye esta dirección por la IPv4 actual del PC (ipconfig).
IPAddress MQTT_BROKER(192, 168, 1, 100);
