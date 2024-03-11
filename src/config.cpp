#include <Arduino.h>
#include "config.h"

String sc_wifi_ssid;
String sc_wifi_password;
String sc_mqtt_host;
int sc_mqtt_port;
String sc_mqtt_user;
String sc_mqtt_pass;
bool skipPositionAndTimeSetup = false; // fallback
const long utcOffsetInSeconds = 8 * 3600; // UTC+8 offset in seconds

void setupConfig()
{
  #ifdef WIFI_SSID_NAME
    if (!String(WIFI_SSID_NAME).isEmpty())
    {
      sc_wifi_ssid = String(WIFI_SSID_NAME);
    }
  #endif

  #ifdef WIFI_SSID_PASSWORD
    if (!String(WIFI_SSID_PASSWORD).isEmpty())
    {
      sc_wifi_password = String(WIFI_SSID_PASSWORD);
    }
  #endif

  #ifdef MQTT_HOST
    if (!String(MQTT_HOST).isEmpty())
    {
      sc_mqtt_host = String(MQTT_HOST);
    }
  #endif

  #ifdef MQTT_PORT
    if (!String(MQTT_PORT).isEmpty())
    {
      // convert string to int
      sc_mqtt_port = String(MQTT_PORT).toInt();
    }
  #endif

  #ifdef MQTT_USER
    if (!String(MQTT_USER).isEmpty())
    {
      sc_mqtt_user = String(MQTT_USER);
    }
  #endif

  #ifdef MQTT_PASSWORD
    if (!String(MQTT_PASSWORD).isEmpty())
    {
      sc_mqtt_pass = String(MQTT_PASSWORD);
    }
  #endif
}
