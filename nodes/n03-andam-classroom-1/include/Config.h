#pragma once
#include <Arduino.h>
namespace Config {
constexpr uint32_t DEBOUNCE_MS=50, WIFI_RETRY_MS=10000, MQTT_RETRY_MS=5000;
constexpr uint32_t HEARTBEAT_MS=45000, MAX_AGE_S=60, SEQ_BLOCK=32;
constexpr uint16_t BROKER_KEEPALIVE_SECONDS=30;
constexpr char NTP1[]="pool.ntp.org", NTP2[]="time.nist.gov", TZ[]="UTC0";
}
