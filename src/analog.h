#pragma once

#include <stdint.h>
#include "protocol_defs.h"

void Analog_Init(uint32_t sysclock_hz, struct analog_telemetry *state_out);
void Analog_SeqIrq(void);
