#include "board_i2c.h"

#include <stdbool.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "board_i2c";
static bool s_initialized = false;

esp_err_t board_i2c_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    const i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BOARD_I2C_SDA_GPIO,
        .scl_io_num = BOARD_I2C_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = BOARD_I2C_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_param_config(BOARD_I2C_PORT, &cfg));
    ESP_ERROR_CHECK(i2c_driver_install(BOARD_I2C_PORT, cfg.mode, 0, 0, 0));
    s_initialized = true;
    ESP_LOGI(TAG, "I2C initialized on SDA=%d SCL=%d", BOARD_I2C_SDA_GPIO, BOARD_I2C_SCL_GPIO);
    return ESP_OK;
}

esp_err_t board_i2c_write(uint8_t device_addr, const uint8_t *data, size_t len)
{
    return i2c_master_write_to_device(
        BOARD_I2C_PORT,
        device_addr,
        data,
        len,
        pdMS_TO_TICKS(BOARD_I2C_TIMEOUT_MS));
}

esp_err_t board_i2c_read(uint8_t device_addr, uint8_t *data, size_t len)
{
    return i2c_master_read_from_device(
        BOARD_I2C_PORT,
        device_addr,
        data,
        len,
        pdMS_TO_TICKS(BOARD_I2C_TIMEOUT_MS));
}

esp_err_t board_i2c_write_reg(uint8_t device_addr, uint8_t reg, const uint8_t *data, size_t len)
{
    uint8_t tmp[32];
    if (len + 1 > sizeof(tmp)) {
        return ESP_ERR_INVALID_SIZE;
    }
    tmp[0] = reg;
    for (size_t i = 0; i < len; i++) {
        tmp[i + 1] = data[i];
    }
    return board_i2c_write(device_addr, tmp, len + 1);
}

esp_err_t board_i2c_read_reg(uint8_t device_addr, uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(
        BOARD_I2C_PORT,
        device_addr,
        &reg,
        1,
        data,
        len,
        pdMS_TO_TICKS(BOARD_I2C_TIMEOUT_MS));
}
