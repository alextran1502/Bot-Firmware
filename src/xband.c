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

#define MEASUREMENT_HZ  	100
#define TIM_BASE        	TIMER0_BASE
#define TIM_INT         	INT_TIMER0B
#define TIM_PERIPH      	SYSCTL_PERIPH_TIMER0
#define COUNTER_ROLLOVER   	0x1FF

static struct xband_telemetry *xband_buffer = 0;

void XBand_Init(uint32_t sysclock_hz, struct xband_telemetry *state_out)
{
	xband_buffer = state_out;

	MAP_SysCtlPeripheralEnable(TIM_PERIPH);
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
	MAP_GPIOPinConfigure(GPIO_PL4_T0CCP0);
	MAP_GPIOPinTypeTimer(GPIO_PORTL_BASE, GPIO_PIN_4);

	// Timer A counts pulses, B times the measurement interval
	MAP_TimerConfigure(TIM_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_COUNT | TIMER_CFG_B_PERIODIC);
	MAP_TimerClockSourceSet(TIM_BASE, TIMER_CLOCK_SYSTEM);
	MAP_TimerPrescaleSet(TIM_BASE, TIMER_A, 0);
	MAP_TimerPrescaleSet(TIM_BASE, TIMER_B, 255);
	MAP_TimerLoadSet(TIM_BASE, TIMER_A, COUNTER_ROLLOVER);
	MAP_TimerLoadSet(TIM_BASE, TIMER_B, sysclock_hz / (256 * MEASUREMENT_HZ));
	MAP_TimerControlEvent(TIM_BASE, TIMER_A, TIMER_EVENT_BOTH_EDGES);
	MAP_TimerIntEnable(TIM_BASE, TIMER_TIMB_TIMEOUT);
	MAP_TimerEnable(TIM_BASE, TIMER_BOTH);
	MAP_IntEnable(TIM_INT);
}

void XBand_Poll(void)
{
	// Update 32-bit edge count from the lwIP context, just prior to each telemetry packet.
	// This field is timed separately with respect to the Timer B measurement period, so each
	// packet will contain the most up-to-date count.

	if (!xband_buffer) {
		return;
	}

	static unsigned last_count16 = 0;
	unsigned count16 = MAP_TimerValueGet(TIM_BASE, TIMER_A);
	unsigned diff16 = (last_count16 - count16) & COUNTER_ROLLOVER;
	last_count16 = count16;

	xband_buffer->edge_count += diff16;
}

void XBand_TimerIrq(void)
{
	MAP_TimerIntClear(TIM_BASE, TIMER_TIMB_TIMEOUT);

	// Measure velocity (edges per measurement period) at a steady rate
	static unsigned last_count16 = 0;
	unsigned count16 = MAP_TimerValueGet(TIM_BASE, TIMER_A);
	unsigned diff16 = (last_count16 - count16) & COUNTER_ROLLOVER;
	last_count16 = count16;

	// Fixed point low pass filter
	const uint32_t prev_speed = xband_buffer->speed_measure;
	const uint32_t shift = 14;
	const uint32_t smooth = 0x3ff0;
	const uint32_t inv = (1 << shift) - smooth;
	const uint32_t next_speed = ((prev_speed * smooth) >> shift) + (diff16 * inv);
	xband_buffer->speed_measure = next_speed;
	xband_buffer->measure_count++;
}
