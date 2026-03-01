#pragma once

#include "esp_err.h"

typedef struct {
    float pressure_hpa;
    float temperature_c;
} lps22hb_sample_t;

esp_err_t lps22hb_init(void);
esp_err_t lps22hb_read_sample(lps22hb_sample_t *out_sample);
