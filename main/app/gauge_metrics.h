#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float roll_deg;
    float pitch_deg;
    float yaw_deg;

    float pressure_hpa;
    float altitude_m;
    float min_altitude_m;
    float max_altitude_m;
    float vertical_speed_mps;

    float speed_mps;
    float max_speed_mps;

    bool imu_ok;
    bool baro_ok;
    uint32_t samples;
} gauge_metrics_t;
