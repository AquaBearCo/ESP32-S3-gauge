#include "lps22hb.h"

#include <stdbool.h>
#include <stdint.h>

#include "esp_check.h"
#include "esp_log.h"

#include "board/board_i2c.h"

static const char *TAG = "lps22hb";

#define LPS22HB_ADDR                0x5C

#define LPS22HB_REG_WHO_AM_I        0x0F
#define LPS22HB_REG_CTRL1           0x10
#define LPS22HB_REG_CTRL2           0x11
#define LPS22HB_REG_PRESS_OUT_XL    0x28

#define LPS22HB_WHO_AM_I_VALUE      0xB1

static bool s_initialized = false;

esp_err_t lps22hb_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    uint8_t whoami = 0;
    esp_err_t err = board_i2c_read_reg(LPS22HB_ADDR, LPS22HB_REG_WHO_AM_I, &whoami, 1);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "LPS22HB not detected: %s", esp_err_to_name(err));
        return err;
    }
    if (whoami != LPS22HB_WHO_AM_I_VALUE) {
        ESP_LOGW(TAG, "Unexpected WHO_AM_I: 0x%02X", whoami);
    }

    const uint8_t ctrl1 = 0x32; // ODR=25Hz, BDU=1
    const uint8_t ctrl2 = 0x10; // IF_ADD_INC=1
    ESP_RETURN_ON_ERROR(board_i2c_write_reg(LPS22HB_ADDR, LPS22HB_REG_CTRL1, &ctrl1, 1), TAG, "CTRL1 write failed");
    ESP_RETURN_ON_ERROR(board_i2c_write_reg(LPS22HB_ADDR, LPS22HB_REG_CTRL2, &ctrl2, 1), TAG, "CTRL2 write failed");

    s_initialized = true;
    ESP_LOGI(TAG, "LPS22HB online");
    return ESP_OK;
}

esp_err_t lps22hb_read_sample(lps22hb_sample_t *out_sample)
{
    if (out_sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t raw[5] = {0};
    ESP_RETURN_ON_ERROR(board_i2c_read_reg(LPS22HB_ADDR, (LPS22HB_REG_PRESS_OUT_XL | 0x80), raw, sizeof(raw)), TAG, "sample read failed");

    int32_t press_raw = (int32_t)((raw[2] << 16) | (raw[1] << 8) | raw[0]);
    if ((press_raw & 0x00800000) != 0) {
        press_raw |= 0xFF000000;
    }
    int16_t temp_raw = (int16_t)((raw[4] << 8) | raw[3]);

    out_sample->pressure_hpa = (float)press_raw / 4096.0f;
    out_sample->temperature_c = (float)temp_raw / 100.0f;

    if (out_sample->pressure_hpa < 250.0f || out_sample->pressure_hpa > 1300.0f) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}
