#include "app_light_logic.h"

bool app_light_should_publish(
    float current_lux,
    float previous_lux,
    bool has_previous,
    unsigned long elapsed_ms,
    float change_threshold,
    unsigned long heartbeat_ms
)
{
    if (!has_previous) {
        return true;
    }

    float difference = current_lux - previous_lux;

    if (difference < 0.0f) {
        difference = -difference;
    }

    if (difference >= change_threshold) {
        return true;
    }

    return elapsed_ms >= heartbeat_ms;
}
