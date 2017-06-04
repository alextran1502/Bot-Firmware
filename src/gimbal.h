#pragma once

#include <stdint.h>
#include <stdbool.h>

struct pbuf;

// UART RX on PC4, UART TX on PC5
#define GIMBAL_BAUD             115200
#define GIMBAL_UART_BASE        UART7_BASE
#define GIMBAL_UART_INT         INT_UART7
#define GIMBAL_UART_PERIPH      SYSCTL_PERIPH_UART7

void Gimbal_Init(uint32_t sysclock_hz);
void Gimbal_TxQueue(struct pbuf *p);    // lwIP context
void Gimbal_Poll(void);                 // lwIP context
void Gimbal_UartIrq(void);
