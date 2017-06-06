#pragma once

#include <stdint.h>
#include "protocol_defs.h"

void XBand_Init(uint32_t sysclock_hz, struct xband_telemetry *state_out);
void XBand_TimerIrq(void);
void XBand_Poll(void);			// lwIP context
