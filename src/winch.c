#include <math.h>
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

// Watchdog for the HX711 force sensor and its associated IRQ, also want to make
// sure it isn't running in slow mode (10 Hz) but is more like 80 Hz.
#define MAX_TICKS_SINCE_FORCE_READING   (BOT_TICK_HZ / 25)

// Detect position sensor jam, motor drive failure, or anything else that would
// cause PWM to be above a threshold while motion is below for a while.
#define JAM_PWM_THRESHOLD               0.3
#define JAM_VELOCITY_THRESHOLD          800.0
#define JAM_DURATION_THRESHOLD          (BOT_TICK_HZ / 10)

// If we've been commanding the motor to zero velocity for this many ticks and the
// motor output still isn't zero, disable the motor driver. This is another safeguard
// against a runaway motor in case the control loop parameters are bad and we're
// hitting another limit, plus it lets us put the brakes on more quickly if we must.
#define MOTOR_HALT_LATENCY_TICKS        (BOT_TICK_HZ / 3)

// How long to keep halted after a halt/jam condition
#define MOTOR_HALT_RESTART_DELAY        (BOT_TICK_HZ / 2)


static uint32_t motor_pwm_period;
static struct winch_status winchstat;

static void winch_set_motor_enable(bool en)
{
    winchstat.motor.enabled = en;
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
        state += param * ((float)measure - state);
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

static void winch_pwm_output_set(float pwm)
{
    // Final stored PWM state is clamped to [-1, 1]
    pwm = pwm > -1.0f ? pwm : -1.0f;
    pwm = pwm < 1.0f ? pwm : 1.0f;
    winchstat.motor.pwm = pwm;

    // Convert to number of clock ticks
    int32_t pwm_quant = motor_pwm_period * pwm;
    winchstat.motor.pwm_quant = pwm_quant;

    // Drive one or the other H-bridge leg according to sign
    if (pwm_quant > 0) {
        MAP_PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, false);
        MAP_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, pwm_quant);
        MAP_PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, true);
    } else if (pwm_quant < 0) {
        MAP_PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, false);
        MAP_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, -pwm_quant);
        MAP_PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);
    } else {
        MAP_PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT | PWM_OUT_2_BIT, false);
    }

    // Enable motor driver for the first time only once we get here.
    // Every subsequent time, this is redundant.
    if (pwm_quant != 0) {
        winch_set_motor_enable(true);
    }
}

static void winch_pwm_halt()
{
    // Disable PWM, disable H-bridge
    MAP_PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT | PWM_OUT_2_BIT, false);
    winch_set_motor_enable(false);

    // Reset control loop state
    winchstat.motor.pwm = 0.0f;
    winchstat.motor.pwm_quant = 0;
    winchstat.motor.ramp_velocity = 0.0f;
    winchstat.motor.vel_err_diff = 0.0f;
    winchstat.motor.vel_err_integral = 0.0f;
}

static bool winch_command_timeout_is_expired()
{
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

    return ticks_without_new_command > MAX_TICKS_SINCE_LAST_COMMAND;
}

static bool winch_force_timeout_is_expired()
{
    // Keep track of how long it's been since the last force measure,
    // so we can halt if the force sensor isn't working. For this purpose
    // we also avoid resetting the timer any time we notice the last
    // reading was zero, since we may get a stream of zero readings
    // if the clock line is good but data comes loose.

    static uint32_t last_force_counter = 0;
    static uint32_t ticks_without_force_update = 0;
    int32_t latest_measure = winchstat.sensors.force.measure;
    uint32_t next_force_counter = winchstat.sensors.force.counter;

    if (next_force_counter != last_force_counter && latest_measure != 0) {
        last_force_counter = next_force_counter;
        ticks_without_force_update = 0;
    } else {
        ticks_without_force_update++;
    }

    return ticks_without_force_update > MAX_TICKS_SINCE_FORCE_READING;
}

static bool is_velocity_near_zero(float v)
{
    return fabsf(v) < 1e-3;
}

static bool winch_force_out_of_range()
{
    float velocity_target = winchstat.command.velocity_target;
    float force = winchstat.sensors.force.filtered;

    return (force > winchstat.command.force_max && velocity_target > 0.0f) ||
           (force < winchstat.command.force_min && velocity_target < 0.0f);
}

static bool winch_needs_to_halt(float velocity_target) {
    // If we're trying to stop the motor but the control loop is sluggish or broken,
    // turn off the driver and reset all PID state and stay off until the command
    // becomes nonzero again.

    static uint32_t halt_tick_count = 0;
    if (winchstat.motor.pwm_quant && is_velocity_near_zero(velocity_target)) {
        halt_tick_count++;
    } else {
        halt_tick_count = 0;
    }
    return halt_tick_count >= MOTOR_HALT_LATENCY_TICKS;
}

static bool winch_jam_detected() {
    // If we're giving the motor power but it isn't moving (much), something
    // is probably really wrong and we should stop.

    static uint32_t jam_tick_count = 0;
    if (fabsf(winchstat.motor.pwm) > JAM_PWM_THRESHOLD &&
        fabsf(winchstat.sensors.velocity) < JAM_VELOCITY_THRESHOLD) {
        jam_tick_count++;
    } else {
        jam_tick_count = 0;
    }
    return jam_tick_count >= JAM_DURATION_THRESHOLD;
}

static void winch_motor_tick()
{
    // Target velocity from the controller; we might override it to zero if something's wrong
    float velocity_target = winchstat.command.velocity_target;

    if (winch_force_timeout_is_expired()) velocity_target = 0.0f;
    if (winch_command_timeout_is_expired()) velocity_target = 0.0f;
    if (winch_force_out_of_range()) velocity_target = 0.0f;

    // Linear velocity ramping (acceleration limit)
    float ramp_velocity = winchstat.motor.ramp_velocity;
    float target_accel = velocity_target - ramp_velocity;
    float rate_per_tick = winchstat.command.accel_rate / BOT_TICK_HZ;
    if (target_accel > rate_per_tick) target_accel = rate_per_tick;
    if (target_accel < -rate_per_tick) target_accel = -rate_per_tick;
    ramp_velocity += target_accel;
    winchstat.motor.ramp_velocity = ramp_velocity;

    // PID loop is based on velocity error
    float diff_filter_param = winchstat.command.diff_filter_param;
    float vel_err = ramp_velocity - winchstat.sensors.velocity;
    float vel_err_diff_unfiltered = (vel_err - winchstat.motor.vel_err) * BOT_TICK_HZ;
    float vel_err_diff = winchstat.motor.vel_err_diff;
    vel_err_diff = diff_filter_param * (vel_err_diff_unfiltered - vel_err_diff);
    float vel_err_integral = winchstat.motor.vel_err_integral + vel_err / BOT_TICK_HZ;
    winchstat.motor.vel_err = vel_err;
    winchstat.motor.vel_err_diff = vel_err_diff;
    winchstat.motor.vel_err_integral = vel_err_integral;

    // Error conditions that disable motor output
    bool is_jammed = winch_jam_detected();
    bool halting = winch_needs_to_halt(velocity_target);

    // Keep motor output disabled after an error for a short while
    static uint32_t halt_restart_timer = 0;
    if (is_jammed || halting) {
        halt_restart_timer = MOTOR_HALT_RESTART_DELAY;
    }
    if (halt_restart_timer) {
        halt_restart_timer--;
        winch_pwm_halt();

    } else {
        // Update PID loop
        float pwm = winchstat.motor.pwm;
        pwm += winchstat.command.pwm_gain_p * vel_err;
        pwm += winchstat.command.pwm_gain_i * vel_err_integral;
        pwm += winchstat.command.pwm_gain_d * vel_err_diff;
        winch_pwm_output_set(pwm);
    }
}

void Winch_QEIIrq()
{
    int32_t position = MAP_QEIPositionGet(QEI0_BASE);
    MAP_QEIIntClear(QEI0_BASE, QEI_INTTIMER);
    float velocity = (float)(position - winchstat.sensors.position) * BOT_TICK_HZ;
    winchstat.sensors.position = position;
    winchstat.sensors.velocity = velocity;

    winch_motor_tick();

    winchstat.tick_counter++;
}
