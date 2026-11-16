#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

#define I2C0_SDA_PIN 16
#define I2C0_SCL_PIN 17
#define I2C_BITRATE_HZ (100 * 1000)
#define DATA_ARR_LEN 3
#define WR_SLEEP_TIME_MS 5

bool write_byte(i2c_inst_t *i2c, uint8_t device_addr, uint16_t w_addr, uint8_t w_data);
void setup_i2c(i2c_inst_t *i2c, uint bitrate, uint8_t sda, uint8_t scl);