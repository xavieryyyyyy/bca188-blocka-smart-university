#ifndef HAYAG_APP_WIFI_H
#define HAYAG_APP_WIFI_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t app_wifi_start(void);
bool app_wifi_is_connected(void);

#endif
