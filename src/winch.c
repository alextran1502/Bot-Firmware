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
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "settings.h"
#include "winch.h"
#include "force.h"

static struct winch_status winchstat;


void Winch_Init(uint32_t sysclock_hz)
{
	Force_Init(sysclock_hz, &winchstat.force);
}

const struct winch_status* Winch_GetStatus()
{
	return &winchstat;
}

void Winch_Command(struct pbuf *p)
{
}
