#pragma once

#include <stdint.h>
#include <stdbool.h>

struct pbuf;

void BotProto_Init(void);
void BotProto_SendCopy(uint8_t type, const void *data, uint32_t len);   // Called in lwIP context
void BotProto_Send(struct pbuf *p);                                     // Called in lwIP context
