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
#include "sensorlib/i2cm_drv.h"
#include "utils/uartstdio.h"
#include "settings.h"
#include "imu.h"

static struct imu_telemetry *imu_buffer;
static tI2CMInstance imu_i2c;
static uint16_t imu_setup_progress = 0;

#define IMU_I2C_ADDR		0x28
#define IMU_I2C_FIRST_REG	0x08
#define IMU_I2C_LAST_REG    0x35

static const uint8_t imu_setup[] = {
	0x3D, 0x00,    // [OPR_MODE] = CONFIGMODE
	0x3F, 0x20,    // [SYS_TRIGGER] = RESET
	0x3E, 0x00,    // [PWR_MODE] = NORMAL
	0x07, 0x00,    // [PAGE_ID] = 0
	0x3F, 0x80,    // [SYS_TRIGGER] = CLK_SEL (External 32kHz crystal)
	0x3D, 0x0C,    // [OPR_MODE] = MODE_NDOF
};


void IMU_Init(uint32_t sysclock_hz, struct imu_telemetry *state_out)
{
	imu_buffer = state_out;
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	MAP_GPIOPinConfigure(GPIO_PB3_I2C0SDA);
	MAP_GPIOPinConfigure(GPIO_PB2_I2C0SCL);
	MAP_GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
	MAP_GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);
	I2CMInit(&imu_i2c, I2C0_BASE, INT_I2C0, 0xff, 0xff, sysclock_hz);
}

void IMU_I2CIrq(void)
{
	I2CMIntHandler(&imu_i2c);
}

static void imu_poll_complete(void *userdata, uint_fast8_t i2c_status)
{
	if (i2c_status == I2CM_STATUS_SUCCESS) {
		imu_buffer->counter++;
	} else {
		// Try re-initializing it next time
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

		if (I2CMWrite(&imu_i2c, IMU_I2C_ADDR, imu_setup + imu_setup_progress, 2, 0, 0)) {
			imu_setup_progress += 2;
		} else {
			imu_setup_progress = 0;
		}

	} else {
		// Read

		static const uint8_t write_buf[1] = { IMU_I2C_FIRST_REG };
		I2CMRead(&imu_i2c, IMU_I2C_ADDR, write_buf, sizeof write_buf,
			(uint8_t*) imu_buffer, IMU_I2C_LAST_REG + 1 - IMU_I2C_FIRST_REG,
			imu_poll_complete, 0);
	}
}
