#pragma once

#include "esp_err.h"
#include "lvgl.h"

esp_err_t board_touch_init(void);
void board_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data);
