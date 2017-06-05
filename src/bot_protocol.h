#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lidar.h"
#include "analog.h"

struct pbuf;

// Main control loop speed and Systick rate
#define BOT_TICK_HZ             200

// UDP packet header byte
#define BOT_MSG_LOOPBACK        0x20        // Entire packet is looped back to controller
#define BOT_MSG_GIMBAL          0x01        // Raw gimbal serial data
#define BOT_MSG_FLYER_SENSORS   0x02        // struct bot_flyer_sensors
#define BOT_MSG_WINCH_STATUS    0x03        // struct bot_winch_status
#define BOT_MSG_WINCH_COMMAND   0x04        // struct bot_winch_command
#define BOT_MSG_LEDS            0x05        // struct bot_leds

struct flyer_sensors {
    struct lidar_telemetry lidar;
    struct analog_telemetry analog;
};

void BotProto_Init(void);
void BotProto_SendCopy(uint8_t type, void *data, uint32_t len);     // Called in lwIP context
void BotProto_Send(struct pbuf *p);                                 // Called in lwIP context
