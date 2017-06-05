#pragma once

#include <stdint.h>
#include "force.h"

struct pbuf;

struct winch_status {
    struct force_telemetry force;
    uint32_t counter;
    int32_t position;
    int32_t velocity;
    int32_t accel;
};

void Winch_Init(uint32_t sysclock_hz);
void Winch_Command(struct pbuf *p);             // lwIP context
const struct winch_status* Winch_GetStatus();
void Winch_QEIIrq(void);
