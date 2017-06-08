#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "utils/uartstdio.h"
#include "drivers/pinout.h"
#include "settings.h"
#include "analog.h"

#define ADC_BASE        ADC0_BASE
#define ADC_SEQINT      INT_ADC0SS0
#define ADC_PERIPH      SYSCTL_PERIPH_ADC0

static struct analog_telemetry *analog_buffer;

static const uint32_t analog_channels[NUM_ANALOG_SENSORS] =
{
    ADC_CTL_CH0, ADC_CTL_CH1, ADC_CTL_CH2, ADC_CTL_CH3,         // PE3 .. PE0
    ADC_CTL_CH16, ADC_CTL_CH17, ADC_CTL_CH18, ADC_CTL_CH19      // PK0 .. PK3
};

void Analog_Init(uint32_t sysclock_hz, struct analog_telemetry *state_out)
{
    analog_buffer = state_out;

    MAP_SysCtlPeripheralEnable(ADC_PERIPH);
    MAP_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0);
    MAP_GPIOPinTypeADC(GPIO_PORTK_BASE, GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0);
    MAP_ADCReferenceSet(ADC_BASE, ADC_REF_INT);
    ADCClockConfigSet(ADC_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_HALF, 64);
    MAP_ADCHardwareOversampleConfigure(ADC_BASE, 64);
    MAP_ADCSequenceConfigure(ADC_BASE, 0, ADC_TRIGGER_ALWAYS, 0);

    for (unsigned seq = 0; seq < NUM_ANALOG_SENSORS; seq++) {
        uint32_t flags = seq == (NUM_ANALOG_SENSORS - 1) ? ADC_CTL_IE | ADC_CTL_END : 0;
        MAP_ADCSequenceStepConfigure(ADC_BASE, 0, seq, analog_channels[seq] | flags);
    }

    MAP_ADCSequenceEnable(ADC_BASE, 0);
    MAP_ADCSequenceDataGet(ADC_BASE, 0, analog_buffer->values);
    MAP_ADCIntClear(ADC_BASE, 0);
    MAP_ADCIntEnable(ADC_BASE, 0);
    MAP_IntEnable(ADC_SEQINT);
}

void Analog_SeqIrq(void)
{
    MAP_ADCIntClear(ADC_BASE, 0);
    MAP_ADCSequenceDataGet(ADC_BASE, 0, analog_buffer->values);
    analog_buffer->counter++;
}
