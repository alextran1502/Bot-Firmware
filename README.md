# Bot-Firmware

Firmware for Tuco Flyer bots. Each of the winches and the flyer itself contain a Tiva-C microcontroller with Ethernet. This common firmware codebase handles network configuration, processing incoming UDP commands, and broadcasting sensor readings over UDP.

## Peripherals

Bot     | Header    | Pin               | Device   | Function
------- | --------- | ----------------- | -------- | ---------------------------------
Any     | A1.7      | PD3 (*SSI2CLK*)   | APA102   | SPI CLK
Any     | D1.6      | PD1 (*SSI2XDAT0*) | APA102   | SPI MOSI
Flyer   | A1.1      | PB3 (*I2C0SDA*)   | BNO055   | I2C SDA
Flyer   | A1.3      | PC4 (*U7RX*)      | Gimbal   | Host to Gimbal Serial (*PITCH*)
Flyer   | A1.4      | PC5 (*U7TX*)      | Gimbal   | Gimbal to Host Serial (*YAW*)
Flyer   | A1.9      | PB2 (*I2C0SCL*)   | BNO055   | I2C SCL
Flyer   | B1.3      | PE0 (*AIN3*)      | IR Prox  | Proximity sensor 3 analog input
Flyer   | B1.4      | PE1 (*AIN2*)      | IR Prox  | Proximity sensor 2 analog input
Flyer   | B1.5      | PE2 (*AIN1*)      | IR Prox  | Proximity sensor 1 analog input
Flyer   | B1.6      | PE3 (*AIN0*)      | IR Prox  | Proximity sensor 0 analog input
Flyer   | B2.5      | PK0 (*AIN16*)     | IR Prox  | Proximity sensor 4 analog input
Flyer   | B2.6      | PK1 (*AIN17*)     | IR Prox  | Proximity sensor 5 analog input
Flyer   | B2.7      | PK2 (*AIN18*)     | IR Prox  | Proximity sensor 6 analog input
Flyer   | B2.8      | PK3 (*AIN19*)     | IR Prox  | Proximity sensor 7 analog input
Flyer   | C1.5      | PL4 (*T0CCP0*)    | Xband    | Doppler pulse input (R divider for 5V to 3V)
Flyer   | C2.5      | PM1               | Lidar    | Enable output 0 (via 1k resistor)
Flyer   | C2.6      | PM2 (*T3CCP0*)    | Lidar    | PWM input 0
Flyer   | D2.2      | PM7 (*T5CCP1*)    | Lidar    | PWM input 3
Flyer   | D2.3      | PP5               | Lidar    | Enable output 3 (via 1k resistor)
Flyer   | D2.4      | PA7 (*T3CCP1*)    | Lidar    | PWM input 1
Flyer   | D2.8      | PP3               | Lidar    | Enable output 1 (via 1k resistor)
Flyer   | D2.9      | PQ1               | Lidar    | Enable output 2 (via 1k resistor)
Flyer   | D2.10     | PM6 (*T5CCP0*)    | Lidar    | PWM input 2
Winch   | A2.7      | PQ0 (*SSI3CLK*)   | HX711    | SPI CLK
Winch   | C1.1      | PF1               | H-Bridge | Motor Enable output
Winch   | C1.2      | PF2 (*M0PWM2*)    | H-Bridge | Motor PWM A output
Winch   | C1.3      | PF3 (*M0PWM3*)    | H-Bridge | Motor PWM B output
Winch   | C1.8      | PL1 (*PhA0*)      | Encoder  | Quadrature Phase A input
Winch   | C1.9      | PL2 (*PhB0*)      | Encoder  | Quadrature Phase B input
Winch   | D2.7      | PQ3 (*SSI3XDAT1*) | HX711    | SPI MISO
