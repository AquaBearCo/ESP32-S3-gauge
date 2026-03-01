#pragma once

#include "esp_err.h"

typedef struct {
    float ax_g;
    float ay_g;
    float az_g;
    float gx_dps;
    float gy_dps;
    float gz_dps;
} qmi8658_sample_t;

esp_err_t qmi8658_init(void);
esp_err_t qmi8658_read_sample(qmi8658_sample_t *out_sample);
