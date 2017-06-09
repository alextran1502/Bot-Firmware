#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "inc/hw_pwm.h"
#include "inc/hw_nvic.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/qei.h"
#include "driverlib/pwm.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "utils/lwiplib.h"
#include "settings.h"
#include "winch.h"
#include "force.h"

#define MOTOR_PWM_HZ    25000
static uint32_t motor_pwm_period;
static struct winch_status winchstat;

static void winch_set_motor_enable(bool en)
{
    MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, en ? GPIO_PIN_1 : 0);
}

static bool winch_wdt_check_halt(void)
{
    // Check if it's okay to run the motors or if we should halt and reset
    // Things to look at:
    //    is force in range (soft stop)
    //    is force sensor working
    //    is command stream working

    return false;
}

void Winch_Init(uint32_t sysclock_hz)
{
    // Drive the Enable signal low for now, we start up the motor after !winch_wdt_check_halt()
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);
    winch_set_motor_enable(false);

    // Force feedback via the external strain gauge ADC chip and its driver
    Force_Init(sysclock_hz, &winchstat.sensors.force);

    // Quadrature encoder tracks position and velocity in hardware, and generates a periodic interrupt
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_QEI0);
    MAP_QEIEnable(QEI0_BASE);
    MAP_QEIConfigure(QEI0_BASE, QEI_CONFIG_CAPTURE_A_B | QEI_CONFIG_NO_RESET |
        QEI_CONFIG_QUADRATURE | QEI_CONFIG_NO_SWAP, 0xFFFFFFFF);
    MAP_QEIVelocityEnable(QEI0_BASE);
    MAP_QEIVelocityConfigure(QEI0_BASE, QEI_VELDIV_1, sysclock_hz / BOT_TICK_HZ);
    MAP_QEIIntEnable(QEI0_BASE, QEI_INTTIMER);
    MAP_GPIOPinConfigure(GPIO_PL1_PHA0);
    MAP_GPIOPinConfigure(GPIO_PL2_PHB0);
    MAP_GPIOPinTypeQEI(GPIO_PORTL_BASE, GPIO_PIN_1 | GPIO_PIN_2);

    // Motion control PWM output
    motor_pwm_period = sysclock_hz / MOTOR_PWM_HZ;
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    MAP_PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_SYNC | PWM_GEN_MODE_DBG_STOP | PWM_GEN_MODE_GEN_SYNC_LOCAL);
    MAP_PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_SYNC | PWM_GEN_MODE_DBG_STOP | PWM_GEN_MODE_GEN_SYNC_LOCAL);
    MAP_PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT | PWM_OUT_3_BIT, true);
    MAP_PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, motor_pwm_period);
    MAP_PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, motor_pwm_period);
    MAP_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0);
    MAP_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 0);
    MAP_PWMSyncUpdate(PWM0_BASE, PWM_GEN_2_BIT | PWM_GEN_3_BIT);
    MAP_PWMGenEnable(PWM0_BASE, PWM_GEN_2);
    MAP_PWMGenEnable(PWM0_BASE, PWM_GEN_3);
    MAP_GPIOPinConfigure(GPIO_PF2_M0PWM2);
    MAP_GPIOPinConfigure(GPIO_PF3_M0PWM3);
    MAP_GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    // Start regular motion processing in the QEI interrupt
    MAP_IntEnable(INT_QEI0);
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

void Winch_QEIIrq()
{
    int32_t position = MAP_QEIPositionGet(QEI0_BASE);
    int32_t velocity = MAP_QEIVelocityGet(QEI0_BASE);
    int32_t accel = velocity - winchstat.sensors.velocity;
    MAP_QEIIntClear(QEI0_BASE, QEI_INTTIMER);
    winchstat.sensors.position = position;
    winchstat.sensors.velocity = velocity;
    winchstat.sensors.accel = accel;

    if (winch_wdt_check_halt()) {
        // Turn off H-bridge driver immediately, reset motor state
        winch_set_motor_enable(false);
        memset(&winchstat.motor, 0, sizeof winchstat.motor);
        winchstat.command.velocity_target = 0;

    } else {
        // Normal operation

        //xxx control loop goes here
        int32_t pwm = winchstat.command.velocity_target;

        MAP_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, pwm > 0 ?  pwm : 0);
        MAP_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, pwm < 0 ? -pwm : 0);
        MAP_PWMSyncUpdate(PWM0_BASE, PWM_GEN_2_BIT | PWM_GEN_3_BIT);

        winch_set_motor_enable(true);
    }

    winchstat.tick_counter++;
}
