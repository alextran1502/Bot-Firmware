#pragma once

#include <stdint.h>

struct xband_telemetry {
	uint32_t edge_count;
	uint32_t speed_measure;
	uint32_t measure_count;
};

void XBand_Init(uint32_t sysclock_hz, struct xband_telemetry *state_out);
void XBand_TimerIrq(void);