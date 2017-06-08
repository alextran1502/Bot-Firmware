#pragma once

#include <stdint.h>
#include "protocol_defs.h"

void Force_Init(uint32_t sysclock_hz, struct force_telemetry *state_out);
void Force_SPIIrq(void);
void Force_DataPinIrq(void);
