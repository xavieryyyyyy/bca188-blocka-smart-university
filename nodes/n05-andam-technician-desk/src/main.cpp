#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "secrets.h"
#include "Config.h"
#include "NodeConfig.h"
#include "AndamSecurity.h"
#include "AndamNetwork.h"
#include "MqttBase.h"
#include "DeskAlerts.h"

constexpr uint32_t SNAPSHOT_TTL_MS = 10000;
AndamNetwork net;
AndamSecurity security;
MqttBase mqtt;
Preferences preferences;
DeskAlerts alerts;
uint8_t rooms = 0;
bool haveSnapshot = false;
bool initialized = false;
uint32_t snapshotAt = 0;
uint32_t lastReportAt = 0;
uint32_t lastReportedCount = 0;
bool reportNow = true;

bool rememberTicket(uint32_t id) {
    if (preferences.putUInt("last-ticket", id) == sizeof(id)) return true;
    Serial.println("Could not persist ANDAM alert ID");
    return false;
}

void onCommand(char* topic, uint8_t* payload, unsigned int size) {
    if (strcmp(topic, NodeConfig::CMD)) return;
    JsonDocument data;
    if (deserializeJson(data, payload, size) || !data["ts"].is<uint32_t>()) return;

    const char* node = data["node"] | "";
    const char* action = data["action"] | "";
    const char* signature = data["sig"] | "";
    const uint32_t timestamp = data["ts"].as<uint32_t>();
    const uint32_t now = net.epoch();
    if (strcmp(node, NodeConfig::ID) || !now || !timestamp ||
        (now > timestamp ? now - timestamp : timestamp - now) > Config::MAX_AGE_S) return;

    const bool isBeep = !strcmp(action, "beep");
    const bool isSnapshot = !strcmp(action, "rooms");
    if (!isBeep && !isSnapshot) return;
    const char* valueKey = isBeep ? "ticket" : "rooms";
    if (!data[valueKey].is<uint32_t>()) return;
    const uint32_t value = data[valueKey].as<uint32_t>();
    if (isSnapshot && value > 3) return;
    const String canonical = String(NodeConfig::ID) + "|" + action + "|" + value + "|" + timestamp;
    if (!security.verify(canonical, String(signature))) return;

    if (isBeep) {
        alerts.accept(value, rememberTicket);
    } else {
        rooms = static_cast<uint8_t>(value);
        haveSnapshot = true;
        snapshotAt = millis();
    }
}

void reportCount(uint32_t count, uint32_t now) {
    JsonDocument data;
    data["node"] = NodeConfig::ID;
    data["metric"] = "alert-count";
    data["unit"] = "count";
    data["value"] = count;
    data["uptime"] = now / 1000;
    String payload;
    serializeJson(data, payload);
    if (!mqtt.publish(NodeConfig::COUNT, payload)) Serial.println("Could not publish alert count");
}

void silence() {
    alerts.silence();
    digitalWrite(NodeConfig::BUZZER, LOW);
}

void setup() {
    Serial.begin(115200);
    pinMode(NodeConfig::ROOM1, OUTPUT);
    pinMode(NodeConfig::ROOM2, OUTPUT);
    pinMode(NodeConfig::GREEN, OUTPUT);
    pinMode(NodeConfig::BUZZER, OUTPUT);
    pinMode(NodeConfig::MUTE, INPUT_PULLUP);
    digitalWrite(NodeConfig::ROOM1, LOW);
    digitalWrite(NodeConfig::ROOM2, LOW);
    digitalWrite(NodeConfig::GREEN, LOW);
    digitalWrite(NodeConfig::BUZZER, LOW);

    if (!security.begin(HMAC_KEY_HEX) || !preferences.begin("andam-n05", false)) {
        Serial.println("ANDAM signing key or flash initialization failed");
        return;
    }
    alerts = DeskAlerts(preferences.getUInt("last-ticket", 0));
    net.begin(WIFI_SSID, WIFI_PASS);
    mqtt.begin(NodeConfig::ID, NodeConfig::CMD, onCommand);
    initialized = true;
}

void loop() {
    if (!initialized) return;
    net.loop();
    const bool ready = net.connected() && net.epoch();
    if (!ready || !mqtt.connected()) {
        haveSnapshot = false;
        silence();
    }
    mqtt.loop(ready);
    const bool connected = ready && mqtt.connected();
    const uint32_t now = millis();

    static bool lastMute = HIGH;
    static bool stableMute = HIGH;
    static uint32_t muteChangedAt = 0;
    const bool mute = digitalRead(NodeConfig::MUTE);
    if (mute != lastMute) {
        lastMute = mute;
        muteChangedAt = now;
    }
    if (now - muteChangedAt >= Config::DEBOUNCE_MS && mute != stableMute) {
        stableMute = mute;
        if (stableMute == LOW) silence();
    }

    if (!connected) haveSnapshot = false;
    const bool fresh = connected && haveSnapshot && now - snapshotAt <= SNAPSHOT_TTL_MS;
    digitalWrite(NodeConfig::ROOM1, fresh && (rooms & 1));
    digitalWrite(NodeConfig::ROOM2, fresh && (rooms & 2));
    digitalWrite(NodeConfig::GREEN, fresh || (now / 500) % 2);
    digitalWrite(NodeConfig::BUZZER, alerts.update(now, connected));

    const uint32_t count = alerts.played();
    if (!connected) {
        reportNow = true;
    } else if (reportNow || count != lastReportedCount || now - lastReportAt >= Config::HEARTBEAT_MS) {
        reportCount(count, now);
        lastReportedCount = count;
        lastReportAt = now;
        reportNow = false;
    }
}
