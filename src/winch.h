#pragma once

#include <stdint.h>
#include "protocol_defs.h"

struct pbuf;

void Winch_Init(uint32_t sysclock_hz);
void Winch_Command(struct pbuf *p);             // lwIP context
const struct winch_status* Winch_GetStatus();
void Winch_QEIIrq(void);
