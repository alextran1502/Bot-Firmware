#pragma once

#include <stdint.h>
#include "protocol_defs.h"

typedef void (*force_callback_t)(int32_t measure);

void Force_Init(uint32_t sysclock_hz, force_callback_t callback);
void Force_SPIIrq(void);
void Force_DataPinIrq(void);