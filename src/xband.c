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
#include "driverlib/timer.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "settings.h"
#include "xband.h"

#define MEASUREMENT_HZ  100
#define TIM_BASE        TIMER0_BASE
#define TIM_INT         INT_TIMER0B
#define TIM_PERIPH      SYSCTL_PERIPH_TIMER0

static struct xband_telemetry *xband_buffer;

void XBand_Init(uint32_t sysclock_hz, struct xband_telemetry *state_out)
{
	xband_buffer = state_out;

	// Timer A counts pulses, B times the measurement interval
	// MAP_SysCtlPeripheralEnable(TIM_BASE);
	// MAP_TimerConfigure(TIM_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_COUNT_UP | TIMER_CFG_B_PERIODIC);
	// MAP_TimerControlEvent(TIM_BASE, TIMER_A, TIMER_EVENT_BOTH_EDGES);
	// MAP_TimerClockSourceSet(TIM_BASE, TIMER_CLOCK_SYSTEM);
	// MAP_TimerPrescaleSet(TIM_BASE, TIMER_A, 0);
	// MAP_TimerPrescaleSet(TIM_BASE, TIMER_B, 255);
	// MAP_TimerLoadSet(TIM_BASE, TIMER_B, sysclock_hz / (256 * MEASUREMENT_HZ));
//	MAP_TimerEnable(TIM_BASE, TIMER_BOTH);
//	MAP_TimerIntEnable(TIM_BASE, TIMER_TIMB_TIMEOUT);
//	MAP_IntEnable(TIM_INT);
//	MAP_GPIOPinConfigure(GPIO_PL4_T0CCP0);
//	MAP_GPIOPinTypeTimer(GPIO_PORTL_BASE, GPIO_PIN_4);
}

void XBand_TimerIrq(void)
{
	/*
	static uint16_t last_count16 = 0;
	uint16_t count16 = MAP_TimerValueGet(TIM_BASE, TIMER_A);
	uint16_t diff16 = count16 - last_count16;
	last_count16 = count16;

	xband_buffer->edge_count += diff16;
	xband_buffer->speed_measure = diff16;
	xband_buffer->measure_count++;
*/
	MAP_TimerIntClear(TIM_BASE, TIMER_TIMB_TIMEOUT);
}