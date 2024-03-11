#ifndef CONFIG_H
#define CONFIG_H

extern String sc_wifi_ssid;
extern String sc_wifi_password;
extern String sc_mqtt_host;
extern int sc_mqtt_port;
extern String sc_mqtt_user;
extern String sc_mqtt_pass;
extern const long utcOffsetInSeconds;
extern bool skipPositionAndTimeSetup;
extern const unsigned long DELAY_MS;

#define RELEASE_URL "https://github.com/zgunz42/servercopilot-iot/releases/latest"
#define DELAY_MS 1000
#define RELEASE_VERSION "v0.0.1"
#endif