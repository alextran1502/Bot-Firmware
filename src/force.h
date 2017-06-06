#pragma once

#include <stdint.h>
#include "protocol_defs.h"

void Force_Init(uint32_t sysclock_hz, struct force_telemetry *state_out);
