#include "app_sensor.h"
#include "app_config.h"

#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "HAYAG_BH1750";

static esp_err_t bh1750_command(uint8_t address, uint8_t command)
{
    return i2c_master_write_to_device(
        HAYAG_I2C_PORT,
        address,
        &command,
        1,
        pdMS_TO_TICKS(1000)
    );
}

static esp_err_t bh1750_init(uint8_t address)
{
    esp_err_t result = bh1750_command(address, 0x01);

    if (result != ESP_OK) {
        return result;
    }

    result = bh1750_command(address, 0x10);

    if (result != ESP_OK) {
        return result;
    }

    vTaskDelay(pdMS_TO_TICKS(180));
    return ESP_OK;
}

static esp_err_t bh1750_read(uint8_t address, float *lux)
{
    if (lux == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2] = {0};

    esp_err_t result = i2c_master_read_from_device(
        HAYAG_I2C_PORT,
        address,
        data,
        sizeof(data),
        pdMS_TO_TICKS(1000)
    );

    if (result != ESP_OK) {
        return result;
    }

    uint16_t raw = ((uint16_t)data[0] << 8) | data[1];
    *lux = (float)raw / 1.2f;

    return ESP_OK;
}

esp_err_t app_sensor_init(void)
{
    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = HAYAG_I2C_SDA,
        .scl_io_num = HAYAG_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = HAYAG_I2C_CLOCK_HZ
    };

    esp_err_t result = i2c_param_config(HAYAG_I2C_PORT, &config);

    if (result != ESP_OK) {
        return result;
    }

    result = i2c_driver_install(
        HAYAG_I2C_PORT,
        I2C_MODE_MASTER,
        0,
        0,
        0
    );

    if (result != ESP_OK) {
        return result;
    }

    result = bh1750_init(HAYAG_BH1750_WINDOW_ADDR);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Window BH1750 initialization failed");
        return result;
    }

    result = bh1750_init(HAYAG_BH1750_DESK_ADDR);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Desk BH1750 initialization failed");
        return result;
    }

    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << HAYAG_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    result = gpio_config(&led_config);

    if (result != ESP_OK) {
        return result;
    }

    gpio_set_level(HAYAG_LED_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(HAYAG_LED_GPIO, 0);

    ESP_LOGI(TAG, "Both BH1750 sensors initialized");
    ESP_LOGI(TAG, "Window address: 0x23, desk address: 0x5C");
    ESP_LOGI(TAG, "Test LED initialized on GPIO %d", HAYAG_LED_GPIO);

    return ESP_OK;
}

esp_err_t app_sensor_read(app_light_reading_t *reading)
{
    if (reading == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    reading->window_valid =
        bh1750_read(HAYAG_BH1750_WINDOW_ADDR, &reading->window_lux) == ESP_OK;

    reading->desk_valid =
        bh1750_read(HAYAG_BH1750_DESK_ADDR, &reading->desk_lux) == ESP_OK;

    if (!reading->window_valid) {
        ESP_LOGW(TAG, "Window BH1750 reading failed");
    }

    if (!reading->desk_valid) {
        ESP_LOGW(TAG, "Desk BH1750 reading failed");
    }

    if (!reading->window_valid && !reading->desk_valid) {
        return ESP_FAIL;
    }

    if (reading->window_valid) {
        ESP_LOGI(TAG, "Window-side: %.2f lx", reading->window_lux);
    }

    if (reading->desk_valid) {
        ESP_LOGI(TAG, "Desk-surface: %.2f lx", reading->desk_lux);
    }

    return ESP_OK;
}
