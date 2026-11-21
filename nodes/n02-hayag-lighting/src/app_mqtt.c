#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "app_mqtt.h"
#include "app_config.h"
#include "app_light_logic.h"
#include "secrets.h"

#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "HAYAG_MQTT";

static esp_mqtt_client_handle_t mqtt_client = NULL;
static volatile bool mqtt_connected = false;

static bool window_has_previous = false;
static bool desk_has_previous = false;
static float previous_window_lux = 0.0f;
static float previous_desk_lux = 0.0f;
static int64_t window_last_publish_ms = 0;
static int64_t desk_last_publish_ms = 0;

static void publish_status(const char *status)
{
    char payload[96];

    snprintf(
        payload,
        sizeof(payload),
        "{\"node\":\"%s\",\"status\":\"%s\"}",
        HAYAG_NODE_ID,
        status
    );

    esp_mqtt_client_publish(
        mqtt_client,
        HAYAG_MQTT_STATUS_TOPIC,
        payload,
        0,
        1,
        1
    );
}

static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data
)
{
    (void)handler_args;
    (void)base;
    (void)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            mqtt_connected = true;
            ESP_LOGI(TAG, "MQTT connected");
            publish_status("online");
            break;

        case MQTT_EVENT_DISCONNECTED:
            mqtt_connected = false;
            ESP_LOGW(TAG, "MQTT disconnected");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT connection error");
            break;

        default:
            break;
    }
}

esp_err_t app_mqtt_start(void)
{
    if (mqtt_client != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_mqtt_client_config_t mqtt_config = {
        .broker = {
            .address = {
                .uri = HAYAG_MQTT_BROKER_URI
            }
        },
        .credentials = {
            .client_id = "bca188-hayag-n02"
        },
        .session = {
            .last_will = {
                .topic = HAYAG_MQTT_STATUS_TOPIC,
                .msg = "{\"node\":\"n02\",\"status\":\"offline\"}",
                .msg_len = 0,
                .qos = 1,
                .retain = 1
            }
        }
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_config);

    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client initialization failed");
        return ESP_FAIL;
    }

    esp_err_t result = esp_mqtt_client_register_event(
        mqtt_client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    );

    if (result != ESP_OK) {
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        return result;
    }

    result = esp_mqtt_client_start(mqtt_client);

    if (result != ESP_OK) {
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        return result;
    }

    return ESP_OK;
}

bool app_mqtt_is_connected(void)
{
    return mqtt_connected;
}

static esp_err_t publish_metric(
    const char *topic,
    const char *metric,
    float lux
)
{
    char payload[192];

    int64_t uptime_seconds = esp_timer_get_time() / 1000000;

    snprintf(
        payload,
        sizeof(payload),
        "{\"node\":\"%s\",\"metric\":\"%s\",\"value\":%.2f,\"unit\":\"lx\",\"uptime\":%lld}",
        HAYAG_NODE_ID,
        metric,
        lux,
        (long long)uptime_seconds
    );

    int message_id = esp_mqtt_client_publish(
        mqtt_client,
        topic,
        payload,
        0,
        1,
        0
    );

    if (message_id < 0) {
        ESP_LOGE(TAG, "Publish failed for %s", topic);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Published %s: %.2f lx", metric, lux);
    return ESP_OK;
}

esp_err_t app_mqtt_publish_reading(const app_light_reading_t *reading)
{
    if (reading == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!mqtt_connected || mqtt_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    int64_t now = esp_timer_get_time() / 1000;
    esp_err_t final_result = ESP_OK;

    if (reading->window_valid) {
        unsigned long elapsed = (unsigned long)(
            now - window_last_publish_ms
        );

        bool should_publish = app_light_should_publish(
            reading->window_lux,
            previous_window_lux,
            window_has_previous,
            elapsed,
            HAYAG_LUX_CHANGE_THRESHOLD,
            HAYAG_HEARTBEAT_MS
        );

        if (should_publish) {
            final_result = publish_metric(
                HAYAG_WINDOW_TOPIC,
                "window-lux",
                reading->window_lux
            );

            if (final_result == ESP_OK) {
                previous_window_lux = reading->window_lux;
                window_has_previous = true;
                window_last_publish_ms = now;
            }
        }
    }

    if (reading->desk_valid) {
        unsigned long elapsed = (unsigned long)(
            now - desk_last_publish_ms
        );

        bool should_publish = app_light_should_publish(
            reading->desk_lux,
            previous_desk_lux,
            desk_has_previous,
            elapsed,
            HAYAG_LUX_CHANGE_THRESHOLD,
            HAYAG_HEARTBEAT_MS
        );

        if (should_publish) {
            esp_err_t result = publish_metric(
                HAYAG_DESK_TOPIC,
                "desk-lux",
                reading->desk_lux
            );

            if (result == ESP_OK) {
                previous_desk_lux = reading->desk_lux;
                desk_has_previous = true;
                desk_last_publish_ms = now;
            } else {
                final_result = result;
            }
        }
    }

    return final_result;
}
