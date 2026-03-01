#pragma once

#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#define BOARD_I2C_PORT               I2C_NUM_0
#define BOARD_I2C_SDA_GPIO           15
#define BOARD_I2C_SCL_GPIO           7
#define BOARD_I2C_FREQ_HZ            400000
#define BOARD_I2C_TIMEOUT_MS         1000

esp_err_t board_i2c_init(void);
esp_err_t board_i2c_write(uint8_t device_addr, const uint8_t *data, size_t len);
esp_err_t board_i2c_read(uint8_t device_addr, uint8_t *data, size_t len);
esp_err_t board_i2c_write_reg(uint8_t device_addr, uint8_t reg, const uint8_t *data, size_t len);
esp_err_t board_i2c_read_reg(uint8_t device_addr, uint8_t reg, uint8_t *data, size_t len);
