#pragma once

#include <stdint.h>

#define NUM_LIDAR_SENSORS  4

struct lidar_telemetry {
    uint32_t ranges[NUM_LIDAR_SENSORS];
    uint32_t counters[NUM_LIDAR_SENSORS];
};

void Lidar_Init(uint32_t sysclock_hz, struct lidar_telemetry *state_out);
