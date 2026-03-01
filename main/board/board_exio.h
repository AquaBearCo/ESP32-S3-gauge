#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define BOARD_EXIO_I2C_ADDR          0x20

#define BOARD_EXIO_LCD_RST           1
#define BOARD_EXIO_TOUCH_RST         2
#define BOARD_EXIO_LCD_CS            3
#define BOARD_EXIO_LCD_BK_EN         4
#define BOARD_EXIO_BUZZER            5

esp_err_t board_exio_init(void);
esp_err_t board_exio_set_pin(uint8_t pin, bool level);
