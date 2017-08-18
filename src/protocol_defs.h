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
    int32_t measure;
    int32_t filtered;
    uint32_t counter;
};

struct winch_command {
    int32_t velocity_target;
    uint32_t accel_max;
    int32_t force_min;
    int32_t force_max;
};

struct winch_sensors {
    struct force_telemetry force;
    int32_t position;
    int32_t velocity;
    int32_t accel;
};

struct winch_motor_control {
    int32_t pwm;
    int32_t ramp_velocity;
};

struct winch_status {
    uint32_t command_counter;
    uint32_t tick_counter;
    struct winch_command command;
    struct winch_sensors sensors;
    struct winch_motor_control motor;
};
