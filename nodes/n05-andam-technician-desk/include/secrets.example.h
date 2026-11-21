#pragma once

// Copy this file to secrets.h and replace the placeholders locally.
#define WIFI_SSID "<2.4 GHz Wi-Fi network>"
#define WIFI_PASS "<Wi-Fi password>"
#define BROKER_HOST "<EMQX deployment hostname without a URL scheme>"
#define BROKER_PORT 8883
#define MQTT_USER "<this node's broker username>"
#define MQTT_PASS "<this node's broker password>"
#define HMAC_KEY_HEX "<this node's separate 64-character hexadecimal signing key>"
#define USE_TLS 1

// Paste the broker's trusted CA certificate in PEM format here.
static constexpr char BROKER_CA_PEM[] = R"PEM(
-----BEGIN CERTIFICATE-----
<CA certificate data>
-----END CERTIFICATE-----
)PEM";
