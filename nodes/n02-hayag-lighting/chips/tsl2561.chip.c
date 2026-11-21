#include "wokwi-api.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t light_level;
    uint8_t registers[16];
    uint8_t reg_pointer;
    bool expecting_register;
    bool word_mode;
    bool powered;
} tsl2561_state_t;

static void update_light_data(tsl2561_state_t *chip) {
    uint32_t level = attr_read(chip->light_level);

    uint16_t channel0 = (uint16_t)(level * 10);
    uint16_t channel1 = (uint16_t)(level * 2);

    chip->registers[0x0C] = channel0 & 0xFF;
    chip->registers[0x0D] = (channel0 >> 8) & 0xFF;
    chip->registers[0x0E] = channel1 & 0xFF;
    chip->registers[0x0F] = (channel1 >> 8) & 0xFF;
}

static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
    tsl2561_state_t *chip = user_data;

    if (address != 0x39) {
        return false;
    }

    if (!read) {
        chip->expecting_register = true;
    }

    return true;
}

static uint8_t on_i2c_read(void *user_data) {
    tsl2561_state_t *chip = user_data;

    update_light_data(chip);

    if (!chip->powered) {
        return 0;
    }

    uint8_t value = chip->registers[chip->reg_pointer & 0x0F];

    if (chip->word_mode) {
        chip->reg_pointer = (chip->reg_pointer + 1) & 0x0F;
    }

    return value;
}

static bool on_i2c_write(void *user_data, uint8_t data) {
    tsl2561_state_t *chip = user_data;

    if (chip->expecting_register) {
        chip->word_mode = (data & 0x20) != 0;
        chip->reg_pointer = data & 0x0F;
        chip->expecting_register = false;
        return true;
    }

    uint8_t reg = chip->reg_pointer & 0x0F;
    chip->registers[reg] = data;

    if (reg == 0x00) {
        chip->powered = (data & 0x03) == 0x03;
    }

    if (chip->word_mode) {
        chip->reg_pointer = (chip->reg_pointer + 1) & 0x0F;
    }

    return true;
}

void chip_init(void) {
    static tsl2561_state_t chip = {0};

    chip.light_level = attr_init("lightLevel", 500);
    chip.registers[0x0A] = 0x50;
    chip.expecting_register = true;

    const i2c_config_t config = {
        .address = 0x39,
        .scl = pin_init("SCL", INPUT_PULLUP),
        .sda = pin_init("SDA", INPUT_PULLUP),
        .connect = on_i2c_connect,
        .read = on_i2c_read,
        .write = on_i2c_write,
        .user_data = &chip,
    };

    i2c_init(&config);

    printf("HAYAG virtual TSL2561 initialized at 0x39\n");
}
