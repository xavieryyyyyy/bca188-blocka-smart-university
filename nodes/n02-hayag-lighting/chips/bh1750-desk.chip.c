#include "wokwi-api.h"
#include <stdint.h>
#include <stdbool.h>

#ifndef BH1750_ADDRESS
#define BH1750_ADDRESS 0x5C
#endif

typedef struct {
    uint32_t light;
    uint8_t index;
    bool powered;
    bool measuring;
} bh1750_t;

static bh1750_t chip;

static bool on_connect(void *data, uint32_t address, bool read) {
    bh1750_t *s = data;
    if (address != BH1750_ADDRESS) return false;
    if (read) s->index = 0;
    return true;
}

static bool on_write(void *data, uint8_t cmd) {
    bh1750_t *s = data;
    if (cmd == 0x00) {
        s->powered = false;
        s->measuring = false;
    } else if (cmd == 0x01 || cmd == 0x07) {
        s->powered = true;
    } else if (cmd == 0x10 || cmd == 0x11 || cmd == 0x13 ||
               cmd == 0x20 || cmd == 0x21 || cmd == 0x23) {
        s->powered = true;
        s->measuring = true;
        s->index = 0;
    }
    return true;
}

static uint8_t on_read(void *data) {
    bh1750_t *s = data;
    if (!s->powered || !s->measuring) return 0;

    float lux = attr_read_float(s->light);
    if (lux < 0) lux = 0;

    uint32_t raw = (uint32_t)(lux * 1.2f);
    if (raw > 65535) raw = 65535;

    uint8_t result = s->index == 0
        ? (uint8_t)(raw >> 8)
        : (uint8_t)(raw & 0xFF);

    s->index = (s->index + 1) & 1;
    return result;
}

void chip_init(void) {
    chip.light = attr_init_float("lux", 500.0f);
    chip.index = 0;
    chip.powered = false;
    chip.measuring = false;

    const i2c_config_t config = {
        .user_data = &chip,
        .address = BH1750_ADDRESS,
        .scl = pin_init("SCL", INPUT_PULLUP),
        .sda = pin_init("SDA", INPUT_PULLUP),
        .connect = on_connect,
        .read = on_read,
        .write = on_write,
    };

    i2c_init(&config);
}
