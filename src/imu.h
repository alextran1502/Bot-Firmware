#pragma once

#include <stdint.h>
#include "protocol_defs.h"

void IMU_Init(uint32_t sysclock_hz, struct imu_telemetry *state_out);
void IMU_Poll(void);
void IMU_I2CIrq(void);
