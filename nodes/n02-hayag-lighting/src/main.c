#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_err.h"

#include "app_config.h"
#include "app_sensor.h"
#include "app_wifi.h"
#include "app_mqtt.h"

static const char *TAG = "HAYAG_N02";
static QueueHandle_t light_queue = NULL;

static void sensor_task(void *parameter)
{
    (void)parameter;

    app_light_reading_t reading;
    TickType_t last_wake_time = xTaskGetTickCount();

    while (1) {
        esp_err_t result = app_sensor_read(&reading);

        if (result == ESP_OK) {
            xQueueOverwrite(light_queue, &reading);
        } else {
            ESP_LOGE(TAG, "Both BH1750 readings failed");
        }

        vTaskDelayUntil(
            &last_wake_time,
            pdMS_TO_TICKS(HAYAG_SAMPLE_INTERVAL_MS)
        );
    }
}

static void mqtt_task(void *parameter)
{
    (void)parameter;

    app_light_reading_t reading;

    ESP_LOGI(TAG, "Waiting for Wi-Fi");

    while (!app_wifi_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Wi-Fi connected");

    while (app_mqtt_start() != ESP_OK) {
        ESP_LOGW(TAG, "MQTT startup failed. Retrying...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    while (1) {
        if (!app_mqtt_is_connected()) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (xQueueReceive(
                light_queue,
                &reading,
                pdMS_TO_TICKS(6000)
            ) == pdPASS) {

            esp_err_t result = app_mqtt_publish_reading(&reading);

            if (result != ESP_OK &&
                result != ESP_ERR_INVALID_STATE) {
                ESP_LOGW(TAG, "Telemetry publishing encountered an error");
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting HAYAG Node 2 lighting system");

    light_queue = xQueueCreate(1, sizeof(app_light_reading_t));

    if (light_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create FreeRTOS queue");
        return;
    }

    esp_err_t result = app_sensor_init();

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Sensor initialization failed: %s",
                 esp_err_to_name(result));
        return;
    }

    result = app_wifi_start();

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi initialization failed: %s",
                 esp_err_to_name(result));
        return;
    }

    if (xTaskCreate(
            sensor_task,
            "n02_sensor_task",
            4096,
            NULL,
            5,
            NULL
        ) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor task");
        return;
    }

    if (xTaskCreate(
            mqtt_task,
            "n02_mqtt_task",
            4096,
            NULL,
            4,
            NULL
        ) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create MQTT task");
        return;
    }

    ESP_LOGI(TAG, "HAYAG Node 2 tasks started");
}
