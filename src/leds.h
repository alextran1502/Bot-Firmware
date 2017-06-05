#pragma once

#include <stdint.h>

struct pbuf;

void LEDs_Init(uint32_t sysclock_hz);
void LEDs_Command(struct pbuf *p);    		// lwIP context
