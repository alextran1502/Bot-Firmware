#pragma once

#include <stdbool.h>
#include <stdint.h>

#define BOT_HAS_GIMBAL          (1 << 0)
#define BOT_HAS_WINCH           (1 << 1)
#define BOT_HAS_FLYER_SENSORS   (1 << 2)
#define BOT_HAS_LEDS            (1 << 3)

#define DBGF_NO_TELEMETRY       (1 << 0)


struct settings_t {
    uint32_t bot_options;       // BOT_HAS_*
    uint32_t debug_flags;       // DBGF_*
    uint32_t udp_port;
    uint32_t ip_controller;
    uint32_t ip_addr;
    uint32_t ip_netmask;
    uint32_t ip_gateway;
};

extern struct settings_t settings;
extern uint8_t mac_address[8];
extern volatile bool console_is_interactive;

void Settings_Init();
void Settings_Console();
