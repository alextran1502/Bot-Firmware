#pragma once

#include <stdint.h>
#include <stdbool.h>

// Main control loop speed and Systick rate
#define BOT_TICK_HZ				200

// UDP packet header byte
#define BOT_MSG_LOOPBACK		0x10		// Entire packet is looped back to controller
#define BOT_MSG_GIMBAL          0x11        // Raw gimbal serial data
#define BOT_MSG_FLYER_SENSORS   0x12        // struct bot_flyer_sensors
#define BOT_MSG_WINCH_STATUS    0x13        // struct bot_winch_status
#define BOT_MSG_WINCH_COMMAND   0x14        // struct bot_winch_command
#define BOT_MSG_LEDS			0x15        // struct bot_leds

void BotProto_Init(void);
void BotProto_SendTelemetry(void);			// Called in lwIP context
