#include "qmi8658.h"

#include <stdbool.h>
#include <stdint.h>

#include "esp_check.h"
#include "esp_log.h"

#include "board/board_i2c.h"

static const char *TAG = "qmi8658";

#define QMI8658_ADDR                 0x6B

#define QMI8658_REG_REVISION_ID      0x01
#define QMI8658_REG_CTRL1            0x02
#define QMI8658_REG_CTRL2            0x03
#define QMI8658_REG_CTRL3            0x04
#define QMI8658_REG_CTRL5            0x06
#define QMI8658_REG_CTRL7            0x08
#define QMI8658_REG_AX_L             0x35

#define QMI8658_ACC_RANGE_4G         1
#define QMI8658_GYRO_RANGE_512DPS    5
#define QMI8658_ODR_250HZ            5

static bool s_initialized = false;
static float s_acc_scale = 4.0f / 32768.0f;
static float s_gyro_scale = 512.0f / 32768.0f;

esp_err_t qmi8658_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    uint8_t rev = 0;
    esp_err_t err = board_i2c_read_reg(QMI8658_ADDR, QMI8658_REG_REVISION_ID, &rev, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "QMI8658 not detected: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t ctrl1 = 0;
    ESP_RETURN_ON_ERROR(board_i2c_read_reg(QMI8658_ADDR, QMI8658_REG_CTRL1, &ctrl1, 1), TAG, "read CTRL1 failed");
    ctrl1 &= 0xFE;      // enable 2MHz oscillator
    ctrl1 |= 0x40;      // auto increment
    ESP_RETURN_ON_ERROR(board_i2c_write_reg(QMI8658_ADDR, QMI8658_REG_CTRL1, &ctrl1, 1), TAG, "write CTRL1 failed");

    const uint8_t ctrl2 = (QMI8658_ACC_RANGE_4G << 4) | QMI8658_ODR_250HZ;
    const uint8_t ctrl3 = (QMI8658_GYRO_RANGE_512DPS << 4) | QMI8658_ODR_250HZ;
    const uint8_t ctrl5 = 0x11;      // enable LPFs
    const uint8_t ctrl7 = 0x43;      // enable accelerometer + gyro

    ESP_RETURN_ON_ERROR(board_i2c_write_reg(QMI8658_ADDR, QMI8658_REG_CTRL2, &ctrl2, 1), TAG, "write CTRL2 failed");
    ESP_RETURN_ON_ERROR(board_i2c_write_reg(QMI8658_ADDR, QMI8658_REG_CTRL3, &ctrl3, 1), TAG, "write CTRL3 failed");
    ESP_RETURN_ON_ERROR(board_i2c_write_reg(QMI8658_ADDR, QMI8658_REG_CTRL5, &ctrl5, 1), TAG, "write CTRL5 failed");
    ESP_RETURN_ON_ERROR(board_i2c_write_reg(QMI8658_ADDR, QMI8658_REG_CTRL7, &ctrl7, 1), TAG, "write CTRL7 failed");

    s_initialized = true;
    ESP_LOGI(TAG, "QMI8658 online (rev=0x%02X)", rev);
    return ESP_OK;
}

esp_err_t qmi8658_read_sample(qmi8658_sample_t *out_sample)
{
    if (out_sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t raw[12] = {0};
    ESP_RETURN_ON_ERROR(board_i2c_read_reg(QMI8658_ADDR, QMI8658_REG_AX_L, raw, sizeof(raw)), TAG, "sample read failed");

    int16_t ax = (int16_t)((raw[1] << 8) | raw[0]);
    int16_t ay = (int16_t)((raw[3] << 8) | raw[2]);
    int16_t az = (int16_t)((raw[5] << 8) | raw[4]);
    int16_t gx = (int16_t)((raw[7] << 8) | raw[6]);
    int16_t gy = (int16_t)((raw[9] << 8) | raw[8]);
    int16_t gz = (int16_t)((raw[11] << 8) | raw[10]);

    out_sample->ax_g = (float)ax * s_acc_scale;
    out_sample->ay_g = (float)ay * s_acc_scale;
    out_sample->az_g = (float)az * s_acc_scale;
    out_sample->gx_dps = (float)gx * s_gyro_scale;
    out_sample->gy_dps = (float)gy * s_gyro_scale;
    out_sample->gz_dps = (float)gz * s_gyro_scale;
    return ESP_OK;
}
