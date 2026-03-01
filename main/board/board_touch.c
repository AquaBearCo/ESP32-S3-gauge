#include "board_touch.h"

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_exio.h"
#include "board_i2c.h"

static const char *TAG = "board_touch";

#define CST820_ADDR                 0x15
#define CST820_REG_GESTURE          0x01
#define CST820_REG_CHIP_ID          0xA7
#define CST820_REG_DISABLE_SLEEP    0xFE

#define BOARD_TOUCH_INT_GPIO        16
#define BOARD_TOUCH_MAX_X           479
#define BOARD_TOUCH_MAX_Y           479

static bool s_touch_ready = false;

esp_err_t board_touch_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BOARD_TOUCH_INT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ESP_ERROR_CHECK(board_exio_set_pin(BOARD_EXIO_TOUCH_RST, false));
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_ERROR_CHECK(board_exio_set_pin(BOARD_EXIO_TOUCH_RST, true));
    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t no_sleep = 0xFF;
    esp_err_t err = board_i2c_write_reg(CST820_ADDR, CST820_REG_DISABLE_SLEEP, &no_sleep, 1);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "touch sleep config failed: %s", esp_err_to_name(err));
    }

    uint8_t chip_id = 0;
    err = board_i2c_read_reg(CST820_ADDR, CST820_REG_CHIP_ID, &chip_id, 1);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "touch not detected: %s", esp_err_to_name(err));
        s_touch_ready = false;
        return err;
    }

    s_touch_ready = true;
    ESP_LOGI(TAG, "CST820 ready (chip=0x%02X)", chip_id);
    return ESP_OK;
}

void board_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;

    data->state = LV_INDEV_STATE_REL;

    if (!s_touch_ready) {
        return;
    }

    uint8_t buf[6] = {0};
    if (board_i2c_read_reg(CST820_ADDR, CST820_REG_GESTURE, buf, sizeof(buf)) != ESP_OK) {
        return;
    }

    const uint8_t points = buf[1] & 0x0F;
    if (points == 0) {
        return;
    }

    int x = ((buf[2] & 0x0F) << 8) | buf[3];
    int y = ((buf[4] & 0x0F) << 8) | buf[5];

    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (x > BOARD_TOUCH_MAX_X) {
        x = BOARD_TOUCH_MAX_X;
    }
    if (y > BOARD_TOUCH_MAX_Y) {
        y = BOARD_TOUCH_MAX_Y;
    }

    data->point.x = (lv_coord_t)x;
    data->point.y = (lv_coord_t)y;
    data->state = LV_INDEV_STATE_PR;
}
