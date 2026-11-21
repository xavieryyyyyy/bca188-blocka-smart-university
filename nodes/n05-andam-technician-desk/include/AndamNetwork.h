
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "Config.h"

class AndamNetwork {
public:
    void begin(const char* s, const char* p) {
        ssid = s;
        pass = p;
        started = true;

        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        WiFi.begin(ssid, pass);

        lastAttempt = millis();
        Serial.println("[INFO] Starting Wi-Fi connection");
    }

    void loop() {
        if (!started) return;

        unsigned long now = millis();

        if (WiFi.status() != WL_CONNECTED) {
            reported = false;

            if (now - lastAttempt >= Config::WIFI_RETRY_MS) {
                lastAttempt = now;
                Serial.println("[INFO] Retrying Wi-Fi connection");
                WiFi.begin(ssid, pass);
            }
            return;
        }

        if (!reported) {
            reported = true;
            Serial.println("[INFO] Wi-Fi connected");
            Serial.print("[INFO] IP: ");
            Serial.println(WiFi.localIP());
        }

        time_t t = time(nullptr);

        if (t > 1700000000) {
            if (!timeSynced) {
                timeSynced = true;
                Serial.println("[SUCCESS] NTP TIME SYNCHRONIZED");
                Serial.println(ctime(&t));
            }
            return;
        }

        if (now - lastNtpAttempt >= 10000) {
            lastNtpAttempt = now;
            Serial.println("[INFO] Requesting NTP time...");
            configTzTime(Config::TZ, Config::NTP1, Config::NTP2);
        }
    }

    bool connected() const {
        return WiFi.status() == WL_CONNECTED;
    }

    uint32_t epoch() const {
        if (!connected()) return 0;

        time_t t = time(nullptr);
        return t > 1700000000 ? (uint32_t)t : 0;
    }

private:
    const char* ssid = nullptr;
    const char* pass = nullptr;

    bool started = false;
    bool reported = false;
    bool timeSynced = false;

    unsigned long lastAttempt = 0;
    unsigned long lastNtpAttempt = 0;
};
