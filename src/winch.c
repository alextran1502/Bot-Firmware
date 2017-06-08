#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "inc/hw_uart.h"
#include "inc/hw_nvic.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/qei.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "utils/lwiplib.h"
#include "settings.h"
#include "winch.h"
#include "force.h"

static struct winch_status winchstat;

void Winch_Init(uint32_t sysclock_hz)
{
    // Force feedback via the external strain gauge ADC chip and its driver
    Force_Init(sysclock_hz, &winchstat.sensors.force);

    // Quadrature encoder tracks position and velocity in hardware
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_QEI0);
    MAP_QEIEnable(QEI0_BASE);
    MAP_QEIConfigure(QEI0_BASE, QEI_CONFIG_CAPTURE_A_B | QEI_CONFIG_NO_RESET |
        QEI_CONFIG_QUADRATURE | QEI_CONFIG_NO_SWAP, 0xFFFFFFFF);
    MAP_QEIVelocityEnable(QEI0_BASE);
    MAP_QEIVelocityConfigure(QEI0_BASE, QEI_VELDIV_1, sysclock_hz / BOT_TICK_HZ);
    MAP_QEIIntEnable(QEI0_BASE, QEI_INTTIMER);
    MAP_IntEnable(INT_QEI0);
    MAP_GPIOPinConfigure(GPIO_PL1_PHA0);
    MAP_GPIOPinConfigure(GPIO_PL2_PHB0);
    MAP_GPIOPinTypeQEI(GPIO_PORTL_BASE, GPIO_PIN_1 | GPIO_PIN_2);
}

const struct winch_status* Winch_GetStatus()
{
    return &winchstat;
}

void Winch_Command(struct pbuf *p)
{
    if (p->len >= sizeof winchstat.command) {
        memcpy(&winchstat.command, p->payload, sizeof winchstat.command);
        winchstat.command_counter++;
    }
}

static bool winch_wdt_check_halt(void)
{
    // Check if it's okay to run the motors or if we should decelerate rapidly.
    // Things to look at:
    //    is force in range (soft stop)
    //    is force sensor working
    //    is command stream working

    return false;
}

void Winch_QEIIrq()
{
    int32_t position = MAP_QEIPositionGet(QEI0_BASE);
    int32_t velocity = MAP_QEIVelocityGet(QEI0_BASE);
    int32_t accel = velocity - winchstat.sensors.velocity;
    MAP_QEIIntClear(QEI0_BASE, QEI_INTTIMER);

    winchstat.sensors.position = position;
    winchstat.sensors.velocity = velocity;
    winchstat.sensors.accel = accel;

    bool is_halting = winch_wdt_check_halt();
    int32_t velocity_target = is_halting ? 0 : winchstat.command.velocity_target;

    winchstat.tick_counter++;
}
