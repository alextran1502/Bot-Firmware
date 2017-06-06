#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/watchdog.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "drivers/pinout.h"
#include "settings.h"
#include "botproto.h"
#include "gimbal.h"
#include "imu.h"
#include "leds.h"
#include "winch.h"
#include "xband.h"
#include "analog.h"
#include "lidar.h"
#include "protocol_defs.h"

struct flyer_sensors flyer_sensor_buffer;


static void periodic_status(void)
{
    const uint32_t interval = BOT_TICK_HZ;
    static uint32_t counter = 0;
    counter++;
    if (counter >= interval) {
        counter = 0;
        if (!console_is_interactive) {
            uint32_t ip = lwIPLocalIPAddrGet();

            UARTprintf("IP=%d.%d.%d.%d RX=%x TX=%x\n",
                (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff,
                lwip_stats.link.recv, lwip_stats.link.xmit);
        }
    }
}

static void tick_watchdog(void)
{
    // At minimum, resetting the watchdog requires a stream of systicks.
    // If telemetry is enabled and we've seen the network connection active
    // at all in the past, additionally require some data to be transmitted
    // successfully before we'll pat the dog.

    uint16_t xmit = lwip_stats.link.xmit;
    static uint16_t last_xmit = 0;
    bool seen_xmit = false;
    bool telemetry_disabled = !!(settings.debug_flags & DBGF_NO_TELEMETRY);
    static bool ever_seen_xmit = false;

    if (xmit != last_xmit) {
        if (last_xmit != 0) {
            seen_xmit = true;
            ever_seen_xmit = true;
        }
        last_xmit = xmit;
    }

    if (telemetry_disabled || !ever_seen_xmit || seen_xmit) {
        MAP_WatchdogIntClear(WATCHDOG0_BASE);
    }
}

void lwIPHostTimerHandler(void)
{
    periodic_status();
    Gimbal_Poll();
    XBand_Poll();

    if (!(settings.debug_flags & DBGF_NO_TELEMETRY)) {
        // Send out telemetry, depending on the robot options

        if (settings.bot_options & BOT_HAS_FLYER_SENSORS) {
            BotProto_SendCopy(BOT_MSG_FLYER_SENSORS, &flyer_sensor_buffer, sizeof flyer_sensor_buffer);
        }
        if (settings.bot_options & BOT_HAS_WINCH) {
            BotProto_SendCopy(BOT_MSG_WINCH_STATUS, Winch_GetStatus(), sizeof(struct winch_status));
        }
    }
}

void systick_isr(void)
{
    // Advance lwIP time, asynchronously wake up the ethernet ISR
    lwIPTimer(1000 / BOT_TICK_HZ);

    // Check network stats, maybe pat watchdog
    tick_watchdog();
}

int main(void)
{
    MAP_SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);
    uint32_t sysclock_hz = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

    PinoutSet(true, false);

    UARTStdioConfig(0, 115200, sysclock_hz);
    UARTprintf("\n\n====\n"
               "Tuco Flyer Firmware starting!\n\n");
    UARTFlushTx(false);

    Settings_Init();

    if (settings.bot_options & BOT_HAS_GIMBAL) {
        Gimbal_Init(sysclock_hz);
    }

    if (settings.bot_options & BOT_HAS_WINCH) {
        Winch_Init(sysclock_hz);
    }

    if (settings.bot_options & BOT_HAS_LEDS) {
        LEDs_Init(sysclock_hz);
    }

    if (settings.bot_options & BOT_HAS_FLYER_SENSORS) {
        XBand_Init(sysclock_hz, &flyer_sensor_buffer.xband);
        IMU_Init(sysclock_hz, &flyer_sensor_buffer.imu);
        Lidar_Init(sysclock_hz, &flyer_sensor_buffer.lidar);
        Analog_Init(sysclock_hz, &flyer_sensor_buffer.analog);
    }

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);
    MAP_WatchdogReloadSet(WATCHDOG0_BASE, sysclock_hz / (BOT_TICK_HZ / 4));
    MAP_WatchdogResetEnable(WATCHDOG0_BASE);
    MAP_WatchdogEnable(WATCHDOG0_BASE);

    MAP_SysTickPeriodSet(sysclock_hz / BOT_TICK_HZ);
    MAP_SysTickEnable();
    MAP_SysTickIntEnable();

    MAP_IntPrioritySet(INT_EMAC0,      0xC0);
    MAP_IntPrioritySet(FAULT_SYSTICK,  0x80);

    lwIPInit(sysclock_hz, mac_address,
        settings.ip_addr, settings.ip_netmask, settings.ip_gateway,
        IPADDR_USE_STATIC);

    BotProto_Init();
    Settings_Console();
}
