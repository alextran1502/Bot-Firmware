#include <stdint.h>
#include <string.h>
#include "driverlib/fpu.h"
#include "driverlib/rom_map.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "utils/lwiplib.h"
#include "utils/uartstdio.h"
#include "gimbal.h"
#include "analog.h"
#include "winch.h"
#include "xband.h"

extern uint32_t _etext;
extern uint32_t _data;
extern uint32_t _edata;
extern uint32_t _bss;
extern uint32_t _ebss;

uint32_t stack_segment[512];

extern int main(void);
extern void systick_isr(void);
extern void UARTStdioIntHandler(void);

void startup(void);
void fault_nmi(void);
void fault_hard(void);
void fault_mpu(void);
void fault_bus(void);
void fault_usage(void);
void unimplemented_isr(void);

typedef void (* ivt_t)(void);
__attribute__ ((section(".isr_vector"))) const ivt_t ivt[] = {
    (ivt_t) ((uintptr_t)stack_segment + sizeof stack_segment),
    startup,
    fault_nmi,                      // FAULT_NMI
    fault_hard,                     // FAULT_HARD
    fault_mpu,                      // FAULT_MPU
    fault_bus,                      // FAULT_BUS
    fault_usage,                    // FAULT_USAGE
    unimplemented_isr,              // reserved
    unimplemented_isr,              // reserved
    unimplemented_isr,              // reserved
    unimplemented_isr,              // reserved
    unimplemented_isr,              // FAULT_SVCALL
    unimplemented_isr,              // FAULT_DEBUG
    unimplemented_isr,              // reserved
    unimplemented_isr,              // FAULT_PENDSV
    systick_isr,                    // FAULT_SYSTICK
    unimplemented_isr,              // INT_GPIOA_TM4C123
    unimplemented_isr,              // INT_GPIOB_TM4C123
    unimplemented_isr,              // INT_GPIOC_TM4C123
    unimplemented_isr,              // INT_GPIOD_TM4C123
    unimplemented_isr,              // INT_GPIOE_TM4C123
    UARTStdioIntHandler,            // INT_UART0_TM4C123
    unimplemented_isr,              // INT_UART1_TM4C123
    unimplemented_isr,              // INT_SSI0_TM4C123
    unimplemented_isr,              // INT_I2C0_TM4C123
    unimplemented_isr,              // INT_PWM0_FAULT_TM4C123
    unimplemented_isr,              // INT_PWM0_0_TM4C123
    unimplemented_isr,              // INT_PWM0_1_TM4C123
    unimplemented_isr,              // INT_PWM0_2_TM4C123
    Winch_QEIIrq,                   // INT_QEI0_TM4C123
    Analog_SeqIrq,                  // INT_ADC0SS0_TM4C123
    unimplemented_isr,              // INT_ADC0SS1_TM4C123
    unimplemented_isr,              // INT_ADC0SS2_TM4C123
    unimplemented_isr,              // INT_ADC0SS3_TM4C123
    unimplemented_isr,              // INT_WATCHDOG_TM4C123
    unimplemented_isr,              // INT_TIMER0A_TM4C123
    XBand_TimerIrq,                 // INT_TIMER0B_TM4C123
    unimplemented_isr,              // INT_TIMER1A_TM4C123
    unimplemented_isr,              // INT_TIMER1B_TM4C123
    unimplemented_isr,              // INT_TIMER2A_TM4C123
    unimplemented_isr,              // INT_TIMER2B_TM4C123
    unimplemented_isr,              // INT_COMP0_TM4C123
    unimplemented_isr,              // INT_COMP1_TM4C123
    unimplemented_isr,              // INT_COMP2_TM4C123
    unimplemented_isr,              // INT_SYSCTL_TM4C123
    unimplemented_isr,              // INT_FLASH_TM4C123
    unimplemented_isr,              // INT_GPIOF_TM4C123
    unimplemented_isr,              // INT_GPIOG_TM4C123
    unimplemented_isr,              // INT_GPIOH_TM4C123
    unimplemented_isr,              // INT_UART2_TM4C123
    unimplemented_isr,              // INT_SSI1_TM4C123
    unimplemented_isr,              // INT_TIMER3A_TM4C123
    unimplemented_isr,              // INT_TIMER3B_TM4C123
    unimplemented_isr,              // INT_I2C1_TM4C123
    unimplemented_isr,              // INT_CAN0_TM4C123
    unimplemented_isr,              // INT_CAN1_TM4C123
    lwIPEthernetIntHandler,         // Ethernet
    unimplemented_isr,              // INT_HIBERNATE_TM4C123
    unimplemented_isr,              // INT_USB0_TM4C123
    unimplemented_isr,              // INT_PWM0_3_TM4C123
    unimplemented_isr,              // INT_UDMA_TM4C123
    unimplemented_isr,              // INT_UDMAERR_TM4C123
    unimplemented_isr,              // INT_ADC1SS0_TM4C123
    unimplemented_isr,              // INT_ADC1SS1_TM4C123
    unimplemented_isr,              // INT_ADC1SS2_TM4C123
    unimplemented_isr,              // INT_ADC1SS3_TM4C123
    unimplemented_isr,              // exti 0
    unimplemented_isr,              // INT_GPIOJ_TM4C123
    unimplemented_isr,              // INT_GPIOK_TM4C123
    unimplemented_isr,              // INT_GPIOL_TM4C123
    unimplemented_isr,              // INT_SSI2_TM4C123
    unimplemented_isr,              // INT_SSI3_TM4C123
    unimplemented_isr,              // INT_UART3_TM4C123
    unimplemented_isr,              // INT_UART4_TM4C123
    unimplemented_isr,              // INT_UART5_TM4C123
    unimplemented_isr,              // INT_UART6_TM4C123
    Gimbal_UartIrq,                 // INT_UART7_TM4C123
    unimplemented_isr,              // INT_I2C2_TM4C123
    unimplemented_isr,              // INT_I2C3_TM4C123
    unimplemented_isr,              // INT_TIMER4A_TM4C123
    unimplemented_isr,              // INT_TIMER4B_TM4C123
    unimplemented_isr,              // INT_TIMER5A_TM4C123
    unimplemented_isr,              // INT_TIMER5B_TM4C123
};

void unimplemented_isr()
{
    UARTprintf("ERROR: Unimplemented ISR\n");
    while (1);
}

void fault_nmi(void)
{
    UARTprintf("FAULT: NMI\n");
    while (1);
}

void fault_hard(void)
{
    UARTprintf("FAULT: HARD\n");
    while (1);
}

void fault_mpu(void)
{
    UARTprintf("FAULT: MPU\n");
    while (1);
}

void fault_bus(void)
{
    UARTprintf("FAULT: BUS\n");
    while (1);
}

void fault_usage(void)
{
    UARTprintf("FAULT: USAGE\n");
    while (1);
}

#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line)
{
    UARTprintf("ERROR at %s:%d\n", pcFilename, ui32Line);
    while (1);
}
#endif

void startup()
{
    memcpy(&_data, &_etext, (uint32_t)&_edata - (uint32_t)&_data);
    memset(&_bss, 0, (uint32_t)&_ebss - (uint32_t)&_bss);
    MAP_FPUEnable();
    main();
    while (1);
}
