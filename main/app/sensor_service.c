#include "sensor_service.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "sensors/lps22hb.h"
#include "sensors/qmi8658.h"

static const char *TAG = "sensor_service";

#define DEG_PER_RAD                  57.2957795f
#define G_TO_MPS2                    9.80665f

static SemaphoreHandle_t s_metrics_lock = NULL;
static gauge_metrics_t s_metrics;

static bool s_qmi_available = false;
static bool s_baro_available = false;

static float s_roll_deg = 0.0f;
static float s_pitch_deg = 0.0f;
static float s_yaw_deg = 0.0f;
static float s_roll_zero_deg = 0.0f;
static float s_pitch_zero_deg = 0.0f;
static float s_speed_mps = 0.0f;
static float s_vertical_speed_mps = 0.0f;

static bool s_have_altitude = false;
static float s_prev_altitude_m = 0.0f;
static float s_min_altitude_m = 0.0f;
static float s_max_altitude_m = 0.0f;
static float s_sea_level_hpa = 1013.2f;

static void sensor_task(void *arg)
{
    (void)arg;
    int64_t last_us = esp_timer_get_time();
    TickType_t wake = xTaskGetTickCount();

    while (true) {
        int64_t now_us = esp_timer_get_time();
        float dt = (float)(now_us - last_us) / 1000000.0f;
        if (dt <= 0.0f) {
            dt = (float)CONFIG_GAUGE_SENSOR_PERIOD_MS / 1000.0f;
        }
        last_us = now_us;

        bool imu_ok = false;
        bool baro_ok = false;
        qmi8658_sample_t imu = {0};
        lps22hb_sample_t baro = {0};

        if (s_qmi_available && qmi8658_read_sample(&imu) == ESP_OK) {
            imu_ok = true;

            const float roll_acc = atan2f(imu.ay_g, imu.az_g) * DEG_PER_RAD;
            const float pitch_acc = atan2f(-imu.ax_g, sqrtf((imu.ay_g * imu.ay_g) + (imu.az_g * imu.az_g))) * DEG_PER_RAD;
            const float alpha = 0.98f;

            s_roll_deg = alpha * (s_roll_deg + (imu.gx_dps * dt)) + ((1.0f - alpha) * roll_acc);
            s_pitch_deg = alpha * (s_pitch_deg + (imu.gy_dps * dt)) + ((1.0f - alpha) * pitch_acc);
            s_yaw_deg += imu.gz_dps * dt;
            if (s_yaw_deg > 180.0f) {
                s_yaw_deg -= 360.0f;
            } else if (s_yaw_deg < -180.0f) {
                s_yaw_deg += 360.0f;
            }

            // Toy speed estimate from gravity-compensated X acceleration.
            const float grav_x_g = -sinf(s_pitch_deg / DEG_PER_RAD);
            const float lin_x_mps2 = (imu.ax_g - grav_x_g) * G_TO_MPS2;
            s_speed_mps += lin_x_mps2 * dt;
            if (s_speed_mps < 0.0f) {
                s_speed_mps = 0.0f;
            }

            const float damping = (float)CONFIG_GAUGE_SPEED_DAMPING_PERMIL / 1000.0f;
            s_speed_mps *= damping;
            if (s_speed_mps < 0.05f) {
                s_speed_mps = 0.0f;
            }
        }

        if (s_baro_available && lps22hb_read_sample(&baro) == ESP_OK) {
            baro_ok = true;

            if (CONFIG_GAUGE_ENABLE_AUTO_BASELINE && !s_have_altitude) {
                s_sea_level_hpa = baro.pressure_hpa;
            }

            const float altitude_m = 44330.0f * (1.0f - powf(baro.pressure_hpa / s_sea_level_hpa, 0.19029495f));
            if (s_have_altitude) {
                const float raw_vs = (altitude_m - s_prev_altitude_m) / dt;
                s_vertical_speed_mps = (0.85f * s_vertical_speed_mps) + (0.15f * raw_vs);
                if (altitude_m < s_min_altitude_m) {
                    s_min_altitude_m = altitude_m;
                }
                if (altitude_m > s_max_altitude_m) {
                    s_max_altitude_m = altitude_m;
                }
            } else {
                s_min_altitude_m = altitude_m;
                s_max_altitude_m = altitude_m;
                s_vertical_speed_mps = 0.0f;
                s_have_altitude = true;
            }
            s_prev_altitude_m = altitude_m;
        }

        xSemaphoreTake(s_metrics_lock, portMAX_DELAY);
        s_metrics.samples++;
        s_metrics.imu_ok = imu_ok;
        s_metrics.baro_ok = baro_ok;
        s_metrics.roll_deg = s_roll_deg - s_roll_zero_deg;
        s_metrics.pitch_deg = s_pitch_deg - s_pitch_zero_deg;
        s_metrics.yaw_deg = s_yaw_deg;
        s_metrics.speed_mps = s_speed_mps;
        if (s_speed_mps > s_metrics.max_speed_mps) {
            s_metrics.max_speed_mps = s_speed_mps;
        }
        if (baro_ok) {
            s_metrics.pressure_hpa = baro.pressure_hpa;
            s_metrics.altitude_m = s_prev_altitude_m;
            s_metrics.min_altitude_m = s_min_altitude_m;
            s_metrics.max_altitude_m = s_max_altitude_m;
            s_metrics.vertical_speed_mps = s_vertical_speed_mps;
        }
        xSemaphoreGive(s_metrics_lock);

        vTaskDelayUntil(&wake, pdMS_TO_TICKS(CONFIG_GAUGE_SENSOR_PERIOD_MS));
    }
}

esp_err_t sensor_service_init(void)
{
    memset(&s_metrics, 0, sizeof(s_metrics));
    s_metrics_lock = xSemaphoreCreateMutex();
    if (s_metrics_lock == NULL) {
        return ESP_ERR_NO_MEM;
    }

    s_sea_level_hpa = (float)CONFIG_GAUGE_SEA_LEVEL_HPA_X10 / 10.0f;

    s_qmi_available = (qmi8658_init() == ESP_OK);
    s_baro_available = (lps22hb_init() == ESP_OK);
    if (!s_qmi_available) {
        ESP_LOGW(TAG, "IMU unavailable");
    }
    if (!s_baro_available) {
        ESP_LOGW(TAG, "Barometer unavailable");
    }

    if (xTaskCreatePinnedToCore(sensor_task, "sensor_task", 4096, NULL, 4, NULL, 0) != pdPASS) {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "sensor task started");
    return ESP_OK;
}

bool sensor_service_get_snapshot(gauge_metrics_t *out_metrics)
{
    if (out_metrics == NULL || s_metrics_lock == NULL) {
        return false;
    }
    xSemaphoreTake(s_metrics_lock, portMAX_DELAY);
    *out_metrics = s_metrics;
    xSemaphoreGive(s_metrics_lock);
    return true;
}

esp_err_t sensor_service_calibrate_level_zero(void)
{
    if (s_metrics_lock == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(s_metrics_lock, portMAX_DELAY);
    s_roll_zero_deg = s_roll_deg;
    s_pitch_zero_deg = s_pitch_deg;
    xSemaphoreGive(s_metrics_lock);

    ESP_LOGI(TAG, "Level calibration set: roll_zero=%.2f pitch_zero=%.2f", s_roll_zero_deg, s_pitch_zero_deg);
    return ESP_OK;
}
