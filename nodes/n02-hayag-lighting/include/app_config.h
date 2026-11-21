#ifndef HAYAG_APP_CONFIG_H
#define HAYAG_APP_CONFIG_H

#include "driver/i2c.h"

#define HAYAG_NODE_ID "n02"

#define HAYAG_I2C_PORT I2C_NUM_0
#define HAYAG_I2C_SDA 21
#define HAYAG_I2C_SCL 22
#define HAYAG_I2C_CLOCK_HZ 100000

#define HAYAG_BH1750_WINDOW_ADDR 0x23
#define HAYAG_BH1750_DESK_ADDR   0x5C

#define HAYAG_LED_GPIO 25

#define HAYAG_SAMPLE_INTERVAL_MS 5000
#define HAYAG_HEARTBEAT_MS 45000
#define HAYAG_LUX_CHANGE_THRESHOLD 1.0f

#define HAYAG_WINDOW_TOPIC "bca188/blocka/hayag/n02/window-lux"
#define HAYAG_DESK_TOPIC   "bca188/blocka/hayag/n02/desk-lux"
#define HAYAG_MQTT_STATUS_TOPIC "bca188/blocka/hayag/n02/status"

#endif
