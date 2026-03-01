#include "board_i2c.h"

#include <stdbool.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "board_i2c";
static bool s_initialized = false;
static i2c_master_bus_handle_t s_i2c_bus = NULL;

typedef struct {
    uint8_t addr;
    i2c_master_dev_handle_t handle;
} i2c_dev_slot_t;

static i2c_dev_slot_t s_dev_slots[8] = {0};

static esp_err_t get_dev_handle(uint8_t device_addr, i2c_master_dev_handle_t *out_handle)
{
    if (out_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized || s_i2c_bus == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    for (size_t i = 0; i < (sizeof(s_dev_slots) / sizeof(s_dev_slots[0])); i++) {
        if (s_dev_slots[i].handle != NULL && s_dev_slots[i].addr == device_addr) {
            *out_handle = s_dev_slots[i].handle;
            return ESP_OK;
        }
    }

    i2c_master_dev_handle_t new_dev = NULL;
    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = device_addr,
        .scl_speed_hz = BOARD_I2C_FREQ_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_i2c_bus, &dev_cfg, &new_dev), TAG, "add device failed");

    for (size_t i = 0; i < (sizeof(s_dev_slots) / sizeof(s_dev_slots[0])); i++) {
        if (s_dev_slots[i].handle == NULL) {
            s_dev_slots[i].addr = device_addr;
            s_dev_slots[i].handle = new_dev;
            *out_handle = new_dev;
            return ESP_OK;
        }
    }

    // No slot left, clean up the just-created device handle.
    i2c_master_bus_rm_device(new_dev);
    return ESP_ERR_NO_MEM;
}

esp_err_t board_i2c_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    const i2c_master_bus_config_t cfg = {
        .i2c_port = BOARD_I2C_PORT,
        .sda_io_num = BOARD_I2C_SDA_GPIO,
        .scl_io_num = BOARD_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &s_i2c_bus));
    s_initialized = true;
    ESP_LOGI(TAG, "I2C initialized on SDA=%d SCL=%d", BOARD_I2C_SDA_GPIO, BOARD_I2C_SCL_GPIO);
    return ESP_OK;
}

esp_err_t board_i2c_write(uint8_t device_addr, const uint8_t *data, size_t len)
{
    i2c_master_dev_handle_t dev = NULL;
    ESP_RETURN_ON_ERROR(get_dev_handle(device_addr, &dev), TAG, "get dev handle failed");
    return i2c_master_transmit(dev, data, len, BOARD_I2C_TIMEOUT_MS);
}

esp_err_t board_i2c_read(uint8_t device_addr, uint8_t *data, size_t len)
{
    i2c_master_dev_handle_t dev = NULL;
    ESP_RETURN_ON_ERROR(get_dev_handle(device_addr, &dev), TAG, "get dev handle failed");
    return i2c_master_receive(dev, data, len, BOARD_I2C_TIMEOUT_MS);
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
    i2c_master_dev_handle_t dev = NULL;
    ESP_RETURN_ON_ERROR(get_dev_handle(device_addr, &dev), TAG, "get dev handle failed");
    return i2c_master_transmit_receive(dev, &reg, 1, data, len, BOARD_I2C_TIMEOUT_MS);
}
