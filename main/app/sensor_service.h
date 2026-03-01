#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "gauge_metrics.h"

esp_err_t sensor_service_init(void);
bool sensor_service_get_snapshot(gauge_metrics_t *out_metrics);
esp_err_t sensor_service_calibrate_level_zero(void);
