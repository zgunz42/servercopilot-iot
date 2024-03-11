#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <config.h>
#include <TimeLib.h>
#include <LittleFS.h>
#include <GitHubOTA.h>
#include <GitHubFsOTA.h>

#define DHTTYPE    DHT11     // DHT 22 (AM2302)

// #ifdef ARDUINO_ARCH_ESP32
//       #include <soc/soc.h>
//       #include "soc/rtc_cntl_reg.h"
// #endif

DHT_Unified dht(D4, DHTTYPE);

uint32_t delayMS;

WiFiClient espClient;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


// GitHubOTA OsOta(RELEASE_VERSION, RELEASE_URL);
GitHubOTA OsOta(RELEASE_VERSION, RELEASE_URL, "firmware.bin", false);
GitHubFsOTA FsOta(RELEASE_VERSION, RELEASE_URL, "filesystem.bin", false);

void listRoot();
void printStatus(sensor_t sensor, NTPClient time_client);

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to wifi");
  Serial.println(sc_wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(sc_wifi_ssid, sc_wifi_password);

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
    if (client.connect(clientId.c_str(), sc_mqtt_user.c_str(), sc_mqtt_pass.c_str())) {
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
  setupConfig();
  Serial.begin(115200);
  LittleFS.begin();
  listRoot();
  setup_wifi();
  Serial.println("Connected!");
  // Initialize time client
  timeClient.begin();
  client.setServer(sc_mqtt_host.c_str(), sc_mqtt_port);
  pinMode(D0, OUTPUT);
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);

  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
  digitalWrite(D0, HIGH);

  // Update time
  timeClient.update();
  printStatus(sensor, timeClient);
  // Chech for updates
  FsOta.handle();
  OsOta.handle();
}  

void listRoot(){
  Serial.printf("Listing root directory\r\n");

  File root = LittleFS.open("/", "r");
  File file = root.openNextFile();

  while(file){
    Serial.printf("  FILE: %s\r\n", file.name());
    file = root.openNextFile();
  }
}

void printStatus(sensor_t sensor, NTPClient time_client) {
  unsigned long dateEpoc = time_client.getEpochTime();

  // Convert epoch time to a time structure
  tmElements_t tm;
  breakTime(dateEpoc, tm);

  // Print formatted time
  Serial.print("Current time: ");
  // Format the date and time string
  char formattedDateTime[25]; // Enough space to hold "YYYY-MM-DD HH:MM:SS" + null terminator
  sprintf(formattedDateTime, "%04d-%02d-%02d %02d:%02d:%02d", tm.Year + 1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);

  Serial.println("Current time: ");
  Serial.println(formattedDateTime);
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
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
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