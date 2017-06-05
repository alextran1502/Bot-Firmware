#pragma once

#include <stdint.h>

#define NUM_ANALOG_SENSORS  8

struct analog_telemetry {
    uint32_t values[NUM_ANALOG_SENSORS];
	uint32_t counter;
};

void Analog_Init(uint32_t sysclock_hz, struct analog_telemetry *state_out);
void Analog_SeqIrq(void);
