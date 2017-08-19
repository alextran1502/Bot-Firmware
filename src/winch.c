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

// Modulation frequency
#define MOTOR_PWM_HZ        25000

// Watchdog for incoming commands; ramp motor to zero when controller disappears
#define MAX_TICKS_SINCE_LAST_COMMAND    2

static uint32_t motor_pwm_period;
static struct winch_status winchstat;

static void winch_set_motor_enable(bool en)
{
    MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, en ? GPIO_PIN_1 : 0);
}

static void winch_force_sensor_callback(int32_t measure)
{
    // Called from the force sensor's ISR when it completes a measurement.
    // We run a low-pass filter and store both the original and filtered data.

    // The control host can do its own filtering on the raw data obviously,
    // but this filtering is built-in so we have a canonical low-pass-filtered
    // signal to use for out-of-range detection.

    // Single pole IIR filter. Let's use that hardware floating point!
    // Assuming 80 Hz sampling rate, and frequencies of interest < 4 Hz or so.

    static float state;
    float param = winchstat.command.force_filter_param;

    if (param > 0.0f && param < 1.0f) {
        state += (1.0f - param) * ((float)measure - state);
    } else {
        state = (float)measure;
    }

    winchstat.sensors.force.measure = measure;
    winchstat.sensors.force.filtered = state;
    winchstat.sensors.force.counter++;
}

void Winch_Init(uint32_t sysclock_hz)
{
    // Drive the Enable signal low for now, we start up the motor after !winch_wdt_check_halt()
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);
    winch_set_motor_enable(false);

    // Force feedback via the external strain gauge ADC chip and its driver
    Force_Init(sysclock_hz, &winch_force_sensor_callback);

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
    MAP_PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    MAP_PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, motor_pwm_period);
    MAP_PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT | PWM_OUT_3_BIT, false);
    MAP_PWMGenEnable(PWM0_BASE, PWM_GEN_1);
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
    MAP_QEIIntClear(QEI0_BASE, QEI_INTTIMER);
    float velocity = (float)(position - winchstat.sensors.position) * BOT_TICK_HZ;
    winchstat.sensors.position = position;
    winchstat.sensors.velocity = velocity;

    // Local copy of status from global data
    float force = winchstat.sensors.force.filtered;
    struct winch_command command = winchstat.command;

    // Determine whether there was a new command since the last tick,
    // and keep count of how many ticks since the last command.
    static uint32_t last_command_counter = 0;
    static uint32_t ticks_without_new_command = 0;
    uint32_t next_command_counter = winchstat.command_counter;
    if (last_command_counter != next_command_counter) {
        last_command_counter = next_command_counter;
        ticks_without_new_command = 0;
    } else {
        ticks_without_new_command++;
    }

    // The central controller gives us a target velocity that drives a PID loop
    float velocity_target = command.velocity_target;

    // If we haven't seen a new command recently, ramp the motor to zero.
    if (ticks_without_new_command > MAX_TICKS_SINCE_LAST_COMMAND) {
        velocity_target = 0.0f;
    }

    // Enforce limits on filtered force sensor readings
    if (force > command.force_max && velocity_target > 0.0f) {
        velocity_target = 0.0f;
    }
    if (force < command.force_min && velocity_target < 0.0f) {
        velocity_target = 0.0f;
    }

    // Linear velocity ramping (acceleration limit)
    float target_accel = velocity_target - winchstat.motor.ramp_velocity;
    float rate_per_tick = command.accel_rate / BOT_TICK_HZ;
    if (target_accel > rate_per_tick) target_accel = rate_per_tick;
    if (target_accel < -rate_per_tick) target_accel = -rate_per_tick;
    winchstat.motor.ramp_velocity += target_accel;

    // xxx more control loop goes here
    int32_t pwm = winchstat.motor.ramp_velocity;
    winchstat.motor.pwm = pwm;

    if (pwm > 0) {
        MAP_PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, false);
        MAP_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, pwm);
        MAP_PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, true);
    } else if (pwm < 0) {
        MAP_PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, false);
        MAP_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, -pwm);
        MAP_PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);
    } else {
        MAP_PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT | PWM_OUT_2_BIT, false);
    }
    winch_set_motor_enable(true);
    winchstat.tick_counter++;
}
