#pragma once
#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "Config.h"
#include "secrets.h"

class MqttBase {
public:
    MqttBase() : mqtt(plain) {}

    void begin(const char* node, const char* commandTopic,
               void (*callback)(char*, uint8_t*, unsigned int)) {
        clientId = String("andam-") + node + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
        statusTopic = String("bca188/blocka/andam/") + node + "/status";
        subTopic = commandTopic;
#if USE_TLS
        secure.setCACert(BROKER_CA_PEM);
        mqtt.setClient(secure);
#endif
        mqtt.setServer(BROKER_HOST, BROKER_PORT);
        mqtt.setKeepAlive(Config::BROKER_KEEPALIVE_SECONDS);
        mqtt.setSocketTimeout(2);
        mqtt.setBufferSize(768);
        mqtt.setCallback(callback);
    }

    void loop(bool ready) {
        if (!ready) {
            if (mqtt.connected()) {
                // Closing the socket without MQTT DISCONNECT lets the broker send the Last Will.
#if USE_TLS
                secure.stop();
#else
                plain.stop();
#endif
            }
            return;
        }

        if (!mqtt.connected() && millis() - lastTry >= Config::MQTT_RETRY_MS) {
            lastTry = millis();
            if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS, statusTopic.c_str(),
                             1, true, "offline")) {
                if (mqtt.subscribe(subTopic, 1)) {
                    mqtt.publish(statusTopic.c_str(), "online", true);
                } else {
                    mqtt.disconnect();
                }
            } else {
                Serial.printf("MQTT connection failed (%d)\n", mqtt.state());
            }
        }
        if (mqtt.connected()) mqtt.loop();
    }

    bool connected() { return mqtt.connected(); }

    bool publish(const char* topic, const String& payload) {
        return mqtt.connected() && mqtt.publish(topic, payload.c_str(), false);
    }

private:
    WiFiClient plain;
    WiFiClientSecure secure;
    PubSubClient mqtt;
    String clientId, statusTopic;
    const char* subTopic = nullptr;
    unsigned long lastTry = 0;
};
