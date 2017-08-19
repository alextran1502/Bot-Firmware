#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "inc/hw_ssi.h"
#include "inc/hw_udma.h"
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
#include "utils/lwiplib.h"
#include "settings.h"
#include "force.h"

// The HX711 can select channel and gain by varying the number of clock pulses.
// Note that the sample rate can also be configured, using a dedicated pin.
#define HX_CHA_X128         25
#define HX_CHB_X32          26
#define HX_CHA_X64          27

#define HX_SSI_BASE         SSI3_BASE
#define HX_SSI_INT          INT_SSI3
#define HX_CLOCK_HZ         1000000
#define HX_NUM_CLOCKS       HX_CHA_X128
#define HX_SSI_PERIPH       SYSCTL_PERIPH_SSI3
#define HX_GPIO_BASE        GPIO_PORTQ_BASE
#define HX_GPIO_INT         INT_GPIOQ3
#define HX_GPIO_INT_PIN     GPIO_INT_PIN_3

static force_callback_t force_callback;
static enum hx_state_t {
    S_WAIT_FOR_DATA_LOW = 0,
    S_READ_FIRST_16_BITS,
    S_READ_REMAINDER
} hx_state;

static void hxssi_data_width(uint32_t width)
{
    // Hardware can transfer between 4 and 16 bits, inclusive.
    HWREG(HX_SSI_BASE + SSI_O_CR0) = (HWREG(HX_SSI_BASE + SSI_O_CR0) & ~0xF) | (width - 1);
}

static void hx_transition_to_state(enum hx_state_t next)
{
    switch (next) {

        case S_WAIT_FOR_DATA_LOW:
            MAP_GPIOIntClear(HX_GPIO_BASE, HX_GPIO_INT_PIN);
            MAP_IntEnable(HX_GPIO_INT);
            break;

        case S_READ_FIRST_16_BITS:
            MAP_IntDisable(HX_GPIO_INT);
            hxssi_data_width(16);
            MAP_SSIDataPutNonBlocking(HX_SSI_BASE, 0);
            break;

        case S_READ_REMAINDER:
            MAP_IntDisable(HX_GPIO_INT);
            hxssi_data_width(HX_NUM_CLOCKS - 16);
            MAP_SSIDataPutNonBlocking(HX_SSI_BASE, 0);
            break;

    }
    hx_state = next;
}

static void hx_store_result(uint32_t first_16_bits, uint32_t remainder)
{
    // SPI register is 16 bits wide, and results are right-justified.
    // The size of the second transfer depends on the configured number of clock pulses.
    // Reassemble the result and sign-extend to 32 bits.
    uint32_t left_justified_remainder = remainder << (32 - (HX_NUM_CLOCKS - 16));
    uint32_t combined24 = 0xffffff & ((first_16_bits << 8) | (left_justified_remainder >> 24));
    uint32_t signbit = (combined24 & 0x800000) ? 0xff000000 : 0;
    int32_t measure = (int32_t) (combined24 | signbit);
    force_callback(measure);

}

void Force_Init(uint32_t sysclock_hz, force_callback_t callback)
{
    force_callback = callback;
    MAP_SysCtlPeripheralEnable(HX_SSI_PERIPH);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
    MAP_SSIConfigSetExpClk(HX_SSI_BASE, sysclock_hz, SSI_FRF_MOTO_MODE_1, SSI_MODE_MASTER, HX_CLOCK_HZ, 8);
    MAP_SSIEnable(HX_SSI_BASE);
    MAP_SSIIntEnable(HX_SSI_BASE, SSI_TXEOT);
    MAP_GPIOPinConfigure(GPIO_PQ0_SSI3CLK);
    MAP_GPIOPinConfigure(GPIO_PQ3_SSI3XDAT1);
    MAP_GPIOPinTypeSSI(GPIO_PORTQ_BASE, GPIO_PIN_3 | GPIO_PIN_0);

    MAP_GPIOIntTypeSet(HX_GPIO_BASE, HX_GPIO_INT_PIN, GPIO_LOW_LEVEL | GPIO_DISCRETE_INT);
    MAP_GPIOIntClear(HX_GPIO_BASE, HX_GPIO_INT_PIN);
    MAP_GPIOIntEnable(HX_GPIO_BASE, HX_GPIO_INT_PIN);

    MAP_IntEnable(HX_SSI_INT);
    hx_transition_to_state(S_WAIT_FOR_DATA_LOW);
}

void Force_SPIIrq(void)
{
    // SPI End-of-Transfer interrupt
    static uint32_t first_16_bits = 0;
    uint32_t remainder = 0;

    if (hx_state == S_READ_FIRST_16_BITS) {
        MAP_SSIDataGetNonBlocking(HX_SSI_BASE, &first_16_bits);
        hx_transition_to_state(S_READ_REMAINDER);
    } else {
        MAP_SSIDataGetNonBlocking(HX_SSI_BASE, &remainder);
        hx_store_result(first_16_bits, remainder);
        hx_transition_to_state(S_WAIT_FOR_DATA_LOW);
    }
    MAP_SSIIntClear(HX_SSI_BASE, SSI_TXEOT);
}

void Force_DataPinIrq(void)
{
    // Interrupts when data is LOW, only enabled in S_WAIT_FOR_DATA_LOW
    // Make sure the pin is still low, to ignore old interrupts.
    if (!MAP_GPIOPinRead(HX_GPIO_BASE, HX_GPIO_INT_PIN)) {
        hx_transition_to_state(S_READ_FIRST_16_BITS);
    }
    MAP_GPIOIntClear(HX_GPIO_BASE, GPIO_INT_PIN_3);
}
