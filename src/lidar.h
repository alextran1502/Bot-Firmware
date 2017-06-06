#pragma once

#include <stdint.h>
#include "protocol_defs.h"

void Lidar_Init(uint32_t sysclock_hz, struct lidar_telemetry *state_out);

void Lidar_Timer3AIrq(void);
void Lidar_Timer3BIrq(void);
void Lidar_Timer5AIrq(void);
void Lidar_Timer5BIrq(void);
