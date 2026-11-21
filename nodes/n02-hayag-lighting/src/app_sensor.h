#ifndef HAYAG_APP_SENSOR_H
#define HAYAG_APP_SENSOR_H

#include <stdbool.h>
#include "esp_err.h"

typedef struct {
    float window_lux;
    float desk_lux;
    bool window_valid;
    bool desk_valid;
} app_light_reading_t;

esp_err_t app_sensor_init(void);
esp_err_t app_sensor_read(app_light_reading_t *reading);

#endif
