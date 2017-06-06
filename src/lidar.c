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
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "settings.h"
#include "lidar.h"

static struct lidar_telemetry *lidar_buffer;


void Lidar_Init(uint32_t sysclock_hz, struct lidar_telemetry *state_out)
{
    lidar_buffer = state_out;

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);

    // Enable all sensors by driving the 1k resistor outputs low
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_5 | GPIO_PIN_3);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, GPIO_PIN_1);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTM_BASE, GPIO_PIN_1);
    MAP_GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_5 | GPIO_PIN_3, 0);
    MAP_GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_1, 0);
    MAP_GPIOPinWrite(GPIO_PORTM_BASE, GPIO_PIN_1, 0);

    // Use timers 3 and 5 to capture four PWM signals
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER5);
    MAP_GPIOPinConfigure(GPIO_PM2_T3CCP0);
    MAP_GPIOPinConfigure(GPIO_PA7_T3CCP1);
    MAP_GPIOPinConfigure(GPIO_PM6_T5CCP0);
    MAP_GPIOPinConfigure(GPIO_PM7_T5CCP1);
    MAP_GPIOPinTypeTimer(GPIO_PORTA_BASE, GPIO_PIN_7);
    MAP_GPIOPinTypeTimer(GPIO_PORTM_BASE, GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7);

    MAP_TimerConfigure(TIMER3_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME | TIMER_CFG_B_CAP_TIME);
    MAP_TimerClockSourceSet(TIMER3_BASE, TIMER_CLOCK_SYSTEM);
    MAP_TimerControlEvent(TIMER3_BASE, TIMER_BOTH, TIMER_EVENT_BOTH_EDGES);
    MAP_TimerPrescaleSet(TIMER3_BASE, TIMER_BOTH, 0xff);
    MAP_TimerIntEnable(TIMER3_BASE, TIMER_CAPA_EVENT | TIMER_CAPB_EVENT);

    MAP_TimerConfigure(TIMER5_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME | TIMER_CFG_B_CAP_TIME);
    MAP_TimerClockSourceSet(TIMER5_BASE, TIMER_CLOCK_SYSTEM);
    MAP_TimerControlEvent(TIMER5_BASE, TIMER_BOTH, TIMER_EVENT_BOTH_EDGES);
    MAP_TimerPrescaleSet(TIMER5_BASE, TIMER_BOTH, 0xff);
    MAP_TimerIntEnable(TIMER5_BASE, TIMER_CAPA_EVENT | TIMER_CAPB_EVENT);

    MAP_TimerEnable(TIMER3_BASE, TIMER_BOTH);
    MAP_TimerEnable(TIMER5_BASE, TIMER_BOTH);

    MAP_IntEnable(INT_TIMER3A);
    MAP_IntEnable(INT_TIMER3B);
    MAP_IntEnable(INT_TIMER5A);
    MAP_IntEnable(INT_TIMER5B);
}

void Lidar_Timer3AIrq(void)
{
    static uint32_t rising_edge;
    uint32_t value = MAP_TimerValueGet(TIMER3_BASE, TIMER_A);
    MAP_TimerIntClear(TIMER3_BASE, TIMER_CAPA_EVENT);
    if (MAP_GPIOPinRead(GPIO_PORTM_BASE, GPIO_PIN_2)) {
        rising_edge = value;
    } else {
        lidar_buffer->ranges[0] = 0xffffff & (rising_edge - value);
        lidar_buffer->counters[0]++;
    }
}

void Lidar_Timer3BIrq(void)
{
    static uint32_t rising_edge;
    uint32_t value = MAP_TimerValueGet(TIMER3_BASE, TIMER_B);
    MAP_TimerIntClear(TIMER3_BASE, TIMER_CAPB_EVENT);
    if (MAP_GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_7)) {
        rising_edge = value;
    } else {
        lidar_buffer->ranges[1] = 0xffffff & (rising_edge - value);
        lidar_buffer->counters[1]++;
    }
}

void Lidar_Timer5AIrq(void)
{
    static uint32_t rising_edge;
    uint32_t value = MAP_TimerValueGet(TIMER5_BASE, TIMER_A);
    MAP_TimerIntClear(TIMER5_BASE, TIMER_CAPA_EVENT);
    if (MAP_GPIOPinRead(GPIO_PORTM_BASE, GPIO_PIN_6)) {
        rising_edge = value;
    } else {
        lidar_buffer->ranges[2] = 0xffffff & (rising_edge - value);
        lidar_buffer->counters[2]++;
    }
}

void Lidar_Timer5BIrq(void)
{
    static uint32_t rising_edge;
    uint32_t value = MAP_TimerValueGet(TIMER5_BASE, TIMER_B);
    MAP_TimerIntClear(TIMER5_BASE, TIMER_CAPB_EVENT);
    if (MAP_GPIOPinRead(GPIO_PORTM_BASE, GPIO_PIN_7)) {
        rising_edge = value;
    } else {
        lidar_buffer->ranges[3] = 0xffffff & (rising_edge - value);
        lidar_buffer->counters[3]++;
    }
}
