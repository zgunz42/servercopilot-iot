#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <PubSubClient.h>

#define MQTT_USER  "1layar"
#define MQTT_PASS  "Novi421++"
#define MQTT_HOST  "192.168.1.7"
#define MQTT_PORT  30000
#define DHTTYPE    DHT11     // DHT 22 (AM2302)

// #ifdef ARDUINO_ARCH_ESP32
//       #include <soc/soc.h>
//       #include "soc/rtc_cntl_reg.h"
// #endif

DHT_Unified dht(D4, DHTTYPE);

uint32_t delayMS;

String ssid;
String password;
bool skipPositionAndTimeSetup = false; // fallback

#ifdef WIFI_SSID_NAME
  if (!String(WIFI_SSID_NAME).isEmpty())
  {
    ssid = String(WIFI_SSID_NAME);
  }
#endif

#ifdef WIFI_SSID_PASSWORD
  if (!String(WIFI_SSID_PASSWORD).isEmpty())
  {
    password = WIFI_SSID_PASSWORD;
  }
#endif

const long utcOffsetInSeconds = 8 * 3600; // UTC+8 offset in seconds

WiFiClient espClient;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("device/ping", "hello world");
      // ... and resubscribe
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()  
 { 
  Serial.begin(115200);
  setup_wifi();
  Serial.println("Connected!");
  
  // Initialize time client
  timeClient.begin();

  
  client.setServer(MQTT_HOST, MQTT_PORT);
  pinMode(D0, OUTPUT);
    // Initialize device.
  dht.begin();
  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
  digitalWrite(D0, HIGH);

  // Update time
  timeClient.update();

  // Print formatted time
  Serial.print("Current time: ");
  Serial.println(timeClient.getFormattedTime());
}  

void loop() {  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);                   
  digitalWrite(LED_BUILTIN, LOW); 
  // Delay between measurements.
  delay(delayMS);
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
    // Convert temperature into char array
    char temperature[20];
    dtostrf(event.temperature, 1, 2, temperature);

    client.publish("device/temperature", temperature);
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));

    char humidity[20];
    dtostrf(event.relative_humidity, 1, 2, humidity);

    client.publish("device/humidity", humidity);
  }
}