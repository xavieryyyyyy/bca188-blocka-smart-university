#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "secrets.h"
#include "Config.h"
#include "NodeConfig.h"
#include "AndamSecurity.h"
#include "Network.h"
#include "MqttBase.h"
#include "RequestState.h"

constexpr unsigned long RETRY_MS = 5000;
constexpr unsigned long CONFIRMATION_MS = 1000;
Network net;
AndamSecurity security;
MqttBase mqtt;
Preferences requests;
RequestState request;
uint32_t helpCount = 0;
unsigned long lastSend = 0;
unsigned long confirmedAt = 0;
bool showConfirmation = false;
bool reportNow = true;
bool initialized = false;

bool saveRequest(const RequestState& next, uint32_t count) {
    // Keep the last count in the upper bits even when the room is idle.
    const uint64_t stored = next.storedValue(count);
    if (requests.putULong64("request-state", stored) != sizeof(stored)) {
        Serial.println("Could not persist ANDAM request");
        return false;
    }
    request = next;
    helpCount = count;
    return true;
}

bool publishCount(const char* event) {
    const uint32_t timestamp = net.epoch();
    const uint32_t uptime = millis() / 1000;
    if (!timestamp) return false;

    const String canonical = String(NodeConfig::ID) + "|" + event + "|" + helpCount +
                             "|" + uptime + "|" + timestamp;
    const String signature = security.sign(canonical);
    if (!signature.length()) return false;

    JsonDocument data;
    data["node"] = NodeConfig::ID;
    data["metric"] = "help-request-count";
    data["unit"] = "count";
    data["event"] = event;
    data["value"] = helpCount;
    data["uptime"] = uptime;
    data["ts"] = timestamp;
    data["sig"] = signature;
    String payload;
    serializeJson(data, payload);
    return mqtt.publish(NodeConfig::COUNT, payload);
}

void onHelp() {
    if (helpCount == UINT32_MAX) return;
    RequestState next = request;
    if (next.start(helpCount + 1) && saveRequest(next, helpCount + 1)) reportNow = true;
}

void onResolve() {
    RequestState next = request;
    if (next.requestResolution() && saveRequest(next, helpCount)) reportNow = true;
}

void onCommand(char* topic, uint8_t* payload, unsigned int size) {
    if (strcmp(topic, NodeConfig::CMD)) return;
    JsonDocument data;
    if (deserializeJson(data, payload, size)) return;

    const char* node = data["node"] | "";
    const char* action = data["action"] | "";
    const char* signature = data["sig"] | "";
    if (!data["req"].is<uint32_t>() || !data["ts"].is<uint32_t>()) return;
    const uint32_t id = data["req"].as<uint32_t>();
    const uint32_t timestamp = data["ts"].as<uint32_t>();
    const uint32_t now = net.epoch();
    if (strcmp(node, NodeConfig::ID) || !now || !timestamp ||
        (now > timestamp ? now - timestamp : timestamp - now) > Config::MAX_AGE_S) return;

    RequestState next = request;
    if (!next.confirm(id, action)) return;
    const String canonical = String(NodeConfig::ID) + "|" + action + "|" + id + "|" + timestamp;
    if (security.verify(canonical, String(signature)) && saveRequest(next, helpCount)) {
        Serial.printf("ANDAM request %lu: %s\n", (unsigned long)id, action);
        showConfirmation = next.phase() == RequestPhase::Saved;
        confirmedAt = millis();
        reportNow = true;
    }
}

struct Button {
    uint8_t pin;
    bool last;
    bool stable;
    unsigned long changedAt;

    void check(unsigned long now, void (*pressed)()) {
        const bool level = digitalRead(pin);
        if (level != last) {
            last = level;
            changedAt = now;
        }
        if (now - changedAt >= Config::DEBOUNCE_MS && level != stable) {
            stable = level;
            if (level == LOW) pressed();
        }
    }
};

Button help{NodeConfig::HELP, HIGH, HIGH, 0};
Button resolve{NodeConfig::RESOLVE, HIGH, HIGH, 0};

void setup() {
    Serial.begin(115200);
    pinMode(NodeConfig::HELP, INPUT_PULLUP);
    pinMode(NodeConfig::RESOLVE, INPUT_PULLUP);
    pinMode(NodeConfig::RED, OUTPUT);
    pinMode(NodeConfig::GREEN, OUTPUT);
    digitalWrite(NodeConfig::RED, LOW);
    digitalWrite(NodeConfig::GREEN, LOW);

    if (!security.begin(HMAC_KEY_HEX) || !requests.begin("andam-req", false)) {
        Serial.println("ANDAM signing key or flash initialization failed");
        return;
    }

    // Start from the V3 request counter if this board is upgraded in place.
    const uint64_t stored = requests.getULong64("request-state", 0);
    request = RequestState::restore(stored);
    helpCount = stored ? static_cast<uint32_t>(stored >> 8) : requests.getUInt("req", 0);
    net.begin(WIFI_SSID, WIFI_PASS);
    mqtt.begin(NodeConfig::ID, NodeConfig::CMD, onCommand);
    initialized = true;
}

void loop() {
    if (!initialized) return;
    net.loop();
    const bool ready = net.connected() && net.epoch();
    mqtt.loop(ready);
    const bool connected = ready && mqtt.connected();
    const unsigned long now = millis();
    help.check(now, onHelp);
    resolve.check(now, onResolve);

    const auto phase = request.phase();
    const bool waitingToSave = phase == RequestPhase::WaitingForSave;
    const bool waitingToResolve = phase == RequestPhase::WaitingForResolution;
    const bool waiting = waitingToSave || waitingToResolve;
    if (!connected) {
        reportNow = true;
    } else if (reportNow || now - lastSend >= (waiting ? RETRY_MS : Config::HEARTBEAT_MS)) {
        const char* event = waitingToSave ? "help_request" : waitingToResolve ? "resolve" : "heartbeat";
        if (!publishCount(event)) Serial.println("Could not publish ANDAM count");
        lastSend = now;
        reportNow = false;
    }

    // A resolution in flight still shows an open ticket until the backend confirms it.
    digitalWrite(NodeConfig::RED, connected &&
        (phase == RequestPhase::Saved || waitingToResolve || (waitingToSave && (now / 200) % 2)));
    if (now - confirmedAt >= CONFIRMATION_MS) showConfirmation = false;
    digitalWrite(NodeConfig::GREEN, connected && phase == RequestPhase::Saved && showConfirmation);
}
