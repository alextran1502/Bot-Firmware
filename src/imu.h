#pragma once

#include <stdint.h>

struct imu_telemetry {
	int16_t accel[3];
	int16_t mag[3];
	int16_t gyro[3];
	int16_t euler[3];
	int16_t quat[4];
	int16_t linacc[3];
	int16_t gravity[3];
	uint8_t temperature;
	uint8_t calibration_stat;
	uint8_t reserved[2];
};

void IMU_Init(uint32_t sysclock_hz, struct imu_telemetry *state_out);
