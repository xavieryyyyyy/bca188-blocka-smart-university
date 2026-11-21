#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#if SIM_SENSOR
  // ---- Simulation settings: PUBLIC test broker, never commit real secrets ----
  #define NODE_ID      "n04"                        // placeholder, confirm with leader
  #define CLIENT_ID    "sim-n04-CHANGEME"           // must be unique: add your name
  #define TOPIC_BASE   "sim/bca188-CHANGEME/hayag/n04"
  #define WIFI_SSID    "Wokwi-GUEST"
  #define WIFI_PASS    ""
  #define MQTT_HOST    "broker.hivemq.com"
  #define MQTT_USER    NULL
  #define MQTT_PASS    NULL
  #define WIFI_BEGIN() WiFi.begin(WIFI_SSID, WIFI_PASS, 6)   // Wokwi uses channel 6
  const uint8_t PIN_POT = 34;      // potentiometer = fake lux
  const uint8_t PIN_FAIL = 27;     // button held = fake "sensor disconnected"
#else
  #include <Wire.h>
  #include <Adafruit_Sensor.h>
  #include <Adafruit_TSL2561_U.h>
  #include "secrets.h"
  #include "node_config.h"
  #define WIFI_BEGIN() WiFi.begin(WIFI_SSID, WIFI_PASS)
  Adafruit_TSL2561_Unified tsl(TSL2561_ADDR_FLOAT, 12345);
  const uint8_t TSL_ADDR = 0x39;
#endif

const char* TOPIC_LUX    = TOPIC_BASE "/desk-lux";
const char* TOPIC_STATUS = TOPIC_BASE "/status";

const unsigned long SAMPLE_MS    = 2000;
const unsigned long HEARTBEAT_MS = 60000;
const unsigned long RETRY_MS     = 5000;
const float CHANGE_ABS = 10.0;
const float CHANGE_PCT = 0.15;

WiFiClient wifi;
PubSubClient mqtt(wifi);

bool sensorOk = false;
float lastPublished = -1;
unsigned long lastSample = 0, lastPublish = 0;
unsigned long lastWifiTry = 0, lastMqttTry = 0;

// ---------- Sensor layer: the only part that differs between sim and real ----------
#if SIM_SENSOR
bool sensorPresent() { return digitalRead(PIN_FAIL) == HIGH; }  // pressed = missing
bool initSensor()    { return sensorPresent(); }
bool readLux(float &lux) {
  lux = analogRead(PIN_POT) / 4095.0f * 1000.0f;   // 0 to 1000 lux
  return true;
}
#else
bool sensorPresent() {
  Wire.beginTransmission(TSL_ADDR);
  return Wire.endTransmission() == 0;
}
bool initSensor() {
  if (!sensorPresent() || !tsl.begin()) return false;
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);
  return true;
}
bool readLux(float &lux) {
  sensors_event_t e;
  if (!tsl.getEvent(&e)) return false;
  lux = e.light;
  return true;
}
#endif

// ---------- Shared logic: identical in sim and real ----------
void publishLux(float lux) {
  StaticJsonDocument<256> doc;
  doc["node"]   = NODE_ID;
  doc["metric"] = "desk-lux";
  doc["value"]  = lux;
  doc["unit"]   = "lx";
  doc["uptime"] = millis() / 1000;
  char buf[256];
  size_t n = serializeJson(doc, buf, sizeof(buf));
  if (mqtt.publish(TOPIC_LUX, (const uint8_t*)buf, n)) {
    lastPublished = lux;
    lastPublish = millis();
    Serial.println(buf);
  } else {
    Serial.println("Publish failed");
  }
}

void keepWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (millis() - lastWifiTry < RETRY_MS) return;
  lastWifiTry = millis();
  WiFi.disconnect();
  WIFI_BEGIN();
  Serial.println("Retrying Wi-Fi...");
}

void keepMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) return;
  if (millis() - lastMqttTry < RETRY_MS) return;
  lastMqttTry = millis();
  if (mqtt.connect(CLIENT_ID, MQTT_USER, MQTT_PASS,
                   TOPIC_STATUS, 1, true, "offline")) {
    mqtt.publish(TOPIC_STATUS, "online", true);
    Serial.println("MQTT connected");
    lastPublish = 0;
  } else {
    Serial.printf("MQTT failed, rc=%d\n", mqtt.state());
  }
}

void setup() {
  Serial.begin(115200);
#if SIM_SENSOR
  pinMode(PIN_FAIL, INPUT_PULLUP);
  Serial.println("*** SIMULATION MODE: fake lux from potentiometer ***");
#else
  Wire.begin(PIN_SDA, PIN_SCL);
#endif
  WiFi.mode(WIFI_STA);
  mqtt.setServer(MQTT_HOST, 1883);
  mqtt.setSocketTimeout(3);
  sensorOk = initSensor();
  if (!sensorOk) Serial.println("ERROR: sensor not found");
}

void loop() {
  keepWifi();
  keepMqtt();
  mqtt.loop();

  if (millis() - lastSample < SAMPLE_MS) return;
  lastSample = millis();

  if (!sensorOk) {
    sensorOk = initSensor();
    if (!sensorOk) { Serial.println("ERROR: sensor missing, not publishing"); return; }
  }
  if (!sensorPresent()) {
    Serial.println("ERROR: sensor disconnected, not publishing");
    sensorOk = false;
    return;
  }

  float lux;
  if (!readLux(lux)) {
    Serial.println("ERROR: read failed, not publishing");
    sensorOk = false;
    return;
  }
  if (isnan(lux) || lux < 0 || lux > 40000) {
    Serial.println("ERROR: invalid lux, not publishing");
    return;
  }

  if (!mqtt.connected()) return;

  bool changed = (lastPublished < 0) ||
      fabs(lux - lastPublished) >= max(CHANGE_ABS, lastPublished * CHANGE_PCT);
  bool heartbeatDue = (millis() - lastPublish) >= HEARTBEAT_MS;
  if (changed || heartbeatDue) publishLux(lux);
}