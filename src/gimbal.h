#pragma once

#include <stdint.h>
#include <stdbool.h>

struct pbuf;

void Gimbal_Init(uint32_t sysclock_hz);
void Gimbal_TxQueue(struct pbuf *p);    // lwIP context
void Gimbal_Poll(void);                 // lwIP context
void Gimbal_UartIrq(void);
