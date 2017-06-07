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
#include "driverlib/i2c.h"
#include "sensorlib/i2cm_drv.h"
#include "utils/uartstdio.h"
#include "settings.h"
#include "imu.h"

static struct imu_telemetry *imu_buffer;
static tI2CMInstance imu_i2c;
static uint16_t imu_setup_progress = 0;
static uint16_t imu_setup_delay = 0;
static bool imu_busy = false;

#define IMU_I2C_ADDR             0x28
#define IMU_I2C_FIRST_DATA_REG   0x08
#define IMU_I2C_LAST_DATA_REG    0x35

static const uint8_t imu_setup[] = {
    // log2(Delay), Register, Value
    7, 0x3D, 0x00,  // [OPR_MODE] = CONFIGMODE
    2, 0x3F, 0x20,  // [SYS_TRIGGER] = RESET
    7, 0x3F, 0x00,  // [SYS_TRIGGER] = (Placeholder for ext 32 kHz crystal, not working)
    5, 0x3E, 0x00,  // [PWR_MODE] = NORMAL
    2, 0x07, 0x00,  // [PAGE_ID] = 0
    2, 0x3D, 0x0C,  // [OPR_MODE] = MODE_NDOF
};


void IMU_Init(uint32_t sysclock_hz, struct imu_telemetry *state_out)
{
    // Try to unstick the bus by forcing SCL and SDA high briefly at startup
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_2 | GPIO_PIN_3);
    MAP_GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_5 | GPIO_PIN_3, GPIO_PIN_5 | GPIO_PIN_3);

    imu_buffer = state_out;
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
    MAP_GPIOPinConfigure(GPIO_PB3_I2C0SDA);
    MAP_GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    I2CMInit(&imu_i2c, I2C0_BASE, INT_I2C0, 0xff, 0xff, sysclock_hz);
    MAP_GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    MAP_GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);
}

void IMU_I2CIrq(void)
{
    I2CMIntHandler(&imu_i2c);
}

static void imu_poll_complete(void *userdata, uint_fast8_t i2c_status)
{
    imu_busy = false;
    if (i2c_status == I2CM_STATUS_SUCCESS) {
        imu_buffer->counter++;
    } else {
        // Try resetting
        imu_setup_progress = 0;
    }
}

void IMU_Poll(void)
{
    if (!imu_buffer) {
        return;
    }

    if (imu_setup_progress < sizeof imu_setup) {
        // Asynchronous setup

        uint8_t step_delay = imu_setup[imu_setup_progress];
        const uint8_t *step_msg = &imu_setup[imu_setup_progress + 1];

        if (imu_setup_delay < (1 << step_delay)) {
            imu_setup_delay++;
        } else {
            imu_setup_delay = 0;
            if (I2CMWrite(&imu_i2c, IMU_I2C_ADDR, step_msg, 2, 0, 0)) {
                imu_setup_progress += 3;
            } else {
                imu_setup_progress = 0;
            }
        }

    } else if (!imu_busy) {
        // Read

        imu_busy = true;
        static const uint8_t write_buf[1] = { IMU_I2C_FIRST_DATA_REG };
        I2CMRead(&imu_i2c, IMU_I2C_ADDR, write_buf, sizeof write_buf,
            (uint8_t*) imu_buffer, IMU_I2C_LAST_DATA_REG + 1 - IMU_I2C_FIRST_DATA_REG,
            imu_poll_complete, 0);
    }
}
