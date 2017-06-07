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
#include "driverlib/udma.h"
#include "driverlib/ssi.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "settings.h"
#include "leds.h"

#define LED_SSI_BASE        SSI2_BASE
#define LED_SSI_INT         INT_SSI2
#define LED_SSI_PERIPH      SYSCTL_PERIPH_SSI2
#define LED_UDMA         	UDMA_CH13_SSI2TX
#define LED_UDMA_CH         (LED_UDMA & 0xFFFF)


void LEDs_Init(uint32_t sysclock_hz)
{
	MAP_SysCtlPeripheralEnable(LED_SSI_PERIPH);
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
	MAP_GPIOPinConfigure(GPIO_PD3_SSI2CLK);
	MAP_GPIOPinConfigure(GPIO_PD1_SSI2XDAT0);
	MAP_GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_3 | GPIO_PIN_1);
	MAP_uDMAChannelAssign(LED_UDMA);
}

void LEDs_Command(struct pbuf *p)
{
}
