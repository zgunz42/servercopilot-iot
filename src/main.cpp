#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <config.h>
#include <TimeLib.h>
#include <LittleFS.h>
#include <GitHubOTA.h>
#include <GitHubFsOTA.h>

#define DHTTYPE    DHT11     // DHT 22 (AM2302)
#define TOPIC "server/command"
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

ESP8266WiFiMulti WiFiMulti;

// A single, global CertStore which can be used by all
// connections.  Needs to stay live the entire time any of
// the WiFiClientBearSSLs are present.
#include <CertStoreBearSSL.h>
BearSSL::CertStore certStore;

// GitHubOTA OsOta(RELEASE_VERSION, RELEASE_URL);
GitHubOTA OsOta(RELEASE_VERSION, RELEASE_URL, "firmware.bin", true);
GitHubFsOTA FsOta(RELEASE_VERSION, RELEASE_URL, "filesystem.bin", true);

void listRoot();
void printStatus(sensor_t sensor, NTPClient time_client);
void fetchURL(BearSSL::WiFiClientSecure *client, const char *host, const uint16_t port, const char *path);

// Set time via NTP, as required for x.509 validation
void setClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");  // UTC

  Serial.print(F("Waiting for NTP time sync: "));
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    yield();
    delay(500);
    Serial.print(F("."));
    now = time(nullptr);
  }

  Serial.println(F(""));
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println("Connecting to wifi");
  Serial.println(sc_wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(sc_wifi_ssid, sc_wifi_password);
  WiFiMulti.addAP(sc_wifi_ssid.c_str(), sc_wifi_password.c_str());

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

void callback(char* topic, byte* payload, unsigned int length)
{
    payload[length] = '\0';
    int value = String((char*) payload).toInt();


    if (topic == TOPIC && value == 1) {
        BearSSL::WiFiClientSecure net_client;
        bool mfln = net_client.probeMaxFragmentLength("github.com", 443, 1024);  // server must be the same as in ESPhttpUpdate.update()
        Serial.printf("MFLN supported: %s\n", mfln ? "yes" : "no");
        if (mfln) { net_client.setBufferSizes(1024, 1024); }
        net_client.setCertStore(&certStore);
        // Chech for updates
        Serial.printf("Attempting to fetch https://www.github.com/...\n");
        fetchURL(&net_client, "www.github.com", 443, "/");
        Serial.printf("Fetched https://api.1layar.com...\n");
        fetchURL(&net_client, "api.1layar.com", 443, "/");

        String firmwareUrl = "https://api.1layar.com/api/v1/device/firmware";
        String filesystemUrl = "https://api.1layar.com/api/v1/device/filesystem";

        FsOta.handle(&net_client, filesystemUrl);
        OsOta.handle(&net_client, firmwareUrl);
    }

    Serial.println(topic);
    Serial.println(value);
}

void setup()  
 { 
  setupConfig();
  Serial.begin(115200);
  setup_wifi();
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
  }

  listRoot();

  int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
  Serial.print(F("Number of CA certs read: "));
  Serial.println(numCerts);  
  Serial.println("Connected!");
  
  setClock();

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
  // printStatus(sensor, timeClient);
  client.setCallback(callback);
  client.subscribe(TOPIC);
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

void fetchURL(BearSSL::WiFiClientSecure *client, const char *host, const uint16_t port, const char *path) {
  if (!path) {
    path = "/";
  }

  Serial.printf("Trying: %s:443...", host);
  client->connect(host, port);
  if (!client->connected()) {
    Serial.printf("*** Can't connect. ***\n-------\n");
    return;
  }
  Serial.printf("Connected!\n-------\n");
  client->write("GET ");
  client->write(path);
  client->write(" HTTP/1.0\r\nHost: ");
  client->write(host);
  client->write("\r\nUser-Agent: ESP8266\r\n");
  client->write("\r\n");
  uint32_t to = millis() + 5000;
  if (client->connected()) {
    do {
      char tmp[32];
      memset(tmp, 0, 32);
      int rlen = client->read((uint8_t*)tmp, sizeof(tmp) - 1);
      yield();
      if (rlen < 0) {
        break;
      }
      // Only print out first line up to \r, then abort connection
      char *nl = strchr(tmp, '\r');
      if (nl) {
        *nl = 0;
        Serial.print(tmp);
        break;
      }
      Serial.print(tmp);
    } while (millis() < to);
  }
  client->stop();
  Serial.printf("\n-------\n");
}