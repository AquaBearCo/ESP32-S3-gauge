#include "board_exio.h"

#include "esp_check.h"
#include "esp_log.h"

#include "board_i2c.h"

static const char *TAG = "board_exio";

enum {
    EXIO_REG_OUTPUT = 0x01,
    EXIO_REG_CONFIG = 0x03,
};

static bool s_initialized = false;
static uint8_t s_output_state = 0;

static esp_err_t exio_write_reg(uint8_t reg, uint8_t value)
{
    return board_i2c_write_reg(BOARD_EXIO_I2C_ADDR, reg, &value, 1);
}

esp_err_t board_exio_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    // 0 = output, 1 = input. Keep all pins as outputs for board control lines.
    ESP_RETURN_ON_ERROR(exio_write_reg(EXIO_REG_CONFIG, 0x00), TAG, "failed to set EXIO direction");

    s_output_state = 0;
    s_output_state |= (1U << (BOARD_EXIO_LCD_RST - 1));
    s_output_state |= (1U << (BOARD_EXIO_TOUCH_RST - 1));
    s_output_state |= (1U << (BOARD_EXIO_LCD_CS - 1));
    s_output_state |= (1U << (BOARD_EXIO_LCD_BK_EN - 1));
    ESP_RETURN_ON_ERROR(exio_write_reg(EXIO_REG_OUTPUT, s_output_state), TAG, "failed to set EXIO state");

    s_initialized = true;
    ESP_LOGI(TAG, "EXIO initialized");
    return ESP_OK;
}

esp_err_t board_exio_set_pin(uint8_t pin, bool level)
{
    if (pin < 1 || pin > 8) {
        return ESP_ERR_INVALID_ARG;
    }
    if (level) {
        s_output_state |= (uint8_t)(1U << (pin - 1));
    } else {
        s_output_state &= (uint8_t)~(1U << (pin - 1));
    }
    return exio_write_reg(EXIO_REG_OUTPUT, s_output_state);
}
