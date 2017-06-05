#pragma once

#include <stdint.h>

struct force_telemetry {
	uint32_t measure;
	uint32_t counter;
};

void Force_Init(uint32_t sysclock_hz, struct force_telemetry *state_out);
