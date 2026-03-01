#pragma once

#include <stdint.h>

#include "esp_err.h"

esp_err_t board_display_init(void);
esp_err_t board_lvgl_init(void);
void board_display_set_backlight(uint8_t percent);
