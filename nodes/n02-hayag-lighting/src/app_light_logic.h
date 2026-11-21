#ifndef HAYAG_APP_LIGHT_LOGIC_H
#define HAYAG_APP_LIGHT_LOGIC_H

#include <stdbool.h>

bool app_light_should_publish(
    float current_lux,
    float previous_lux,
    bool has_previous,
    unsigned long elapsed_ms,
    float change_threshold,
    unsigned long heartbeat_ms
);

#endif
