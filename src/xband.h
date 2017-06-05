#pragma once

#include <stdint.h>

struct xband_telemetry {
	uint32_t pulse_count;
};

void XBand_Init(uint32_t sysclock_hz, struct xband_telemetry *state_out);
