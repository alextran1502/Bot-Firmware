#pragma once

#include <stdint.h>
#include <stdbool.h>

// Main control loop speed and Systick rate
#define BOT_TICK_HZ             250

// UDP packet header byte
#define BOT_MSG_LOOPBACK        0x20        // copy data
#define BOT_MSG_GIMBAL          0x01        // fygimbal protocol data
#define BOT_MSG_FLYER_SENSORS   0x02        // struct flyer_sensors
#define BOT_MSG_WINCH_STATUS    0x03        // struct winch_status
#define BOT_MSG_WINCH_COMMAND   0x04        // struct winch_command
#define BOT_MSG_LEDS            0x05        // apa102 data, 32 bits/pixel

struct xband_telemetry {
    uint32_t edge_count;
    uint32_t speed_measure;
    uint32_t measure_count;
};

#define NUM_LIDAR_SENSORS  4
struct lidar_telemetry {
    uint32_t ranges[NUM_LIDAR_SENSORS];
    uint32_t counters[NUM_LIDAR_SENSORS];
};

#define NUM_ANALOG_SENSORS  8
struct analog_telemetry {
    uint32_t values[NUM_ANALOG_SENSORS];
    uint32_t counter;
};

struct imu_telemetry {
    uint8_t bno055[48];
    uint32_t counter;
};

struct flyer_sensors {
    struct xband_telemetry xband;
    struct lidar_telemetry lidar;
    struct analog_telemetry analog;
    struct imu_telemetry imu;
};

struct force_telemetry {
    int32_t measure;            // Uncalibrated, (+) = increasing tension
    float filtered;             // Same units, just low-pass filtered prior to limit testing
    uint32_t counter;
};

struct force_command {
    float filter_param;         // IIR filter parameter in range [0,1] for force sensor, 0=slow 1=fast
    float neg_motion_min;       // Uncalibrated load cell units, no negative motion below
    float pos_motion_max;       // Uncalibrated load cell units, no positive motion above this filtered force value
    float lockout_below;        // Uncalibrated load cell units, no motion at all below
    float lockout_above;        // Uncalibrated load cell units, no motion at all above
};

struct pid_gains {
    float gain_p;               // PWM strength proportional to position error
    float gain_i;               // PWM strength proportional to integral of position error
    float gain_d;               // PWM gain proportional to velocity error
    float d_filter_param;       // IIR filter parameter in range [0,1] for velocity error, 0=slow 1=fast
    float i_decay_param;        // Exponential decay for the integral parameter, 0=slow 1=fast
};

struct winch_command {
    struct force_command force;
    struct pid_gains pid;
    int32_t position;
};

struct winch_sensors {
    struct force_telemetry force;
    int32_t position;           // Integrated position in encoder units, from hardware
    float velocity;             // Calculated instantaneous velocity on each tick, in position units per second
};

struct winch_pwm {
    float total;                // PWM calculated by the PID loop, clamped to [-1, 1]
    float p;                    // Just the contribution from proportional gain
    float i;                    // Just the contribution from integral gain
    float d;                    // Just the contribution from derivative gain
    int16_t quant;              // PWM state after quantizing into clock ticks
    int16_t enabled;            // Is the H-bridge enabled? Can be turned off by halt conditions.
};

struct winch_motor_control {
    struct winch_pwm pwm;
    int32_t position_err;       // Instantaneous position error
    float pos_err_integral;     // Accumulated integral of the position error, reset by halt watchdog
    float vel_err_inst;         // Instantaneous velocity error
    float vel_err_filtered;     // Low-pass-filtered velocity error
};

struct winch_status {
    uint32_t command_counter;
    uint32_t tick_counter;
    struct winch_command command;
    struct winch_sensors sensors;
    struct winch_motor_control motor;
};
