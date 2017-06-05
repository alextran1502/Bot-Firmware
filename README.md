# Bot-Firmware

Firmware for Tuco Flyer bots. Each of the winches and the flyer itself contain a Tiva-C microcontroller with Ethernet. This common firmware codebase handles network configuration, processing incoming UDP commands, and broadcasting sensor readings over UDP.

## Peripherals

Bot     | Header    | Pin               | Device   | Function
------- | --------- | ----------------- | -------- | ---------------------------------
Flyer   | A1.3      | PC4 (*U7RX*)      | Gimbal   | Host to Gimbal Serial (*Pitch*)
Flyer   | A1.4      | PC5 (*U7TX*)      | Gimbal   | Gimbal to Host Serial (*Yaw*)
Flyer   | B1.6      | PE3 (*AIN0*)      | IR Prox  | Proximity sensor 0 analog input
Flyer   | B1.5      | PE2 (*AIN1*)      | IR Prox  | Proximity sensor 1 analog input
Flyer   | B1.4      | PE1 (*AIN2*)      | IR Prox  | Proximity sensor 2 analog input
Flyer   | B1.3      | PE0 (*AIN3*)      | IR Prox  | Proximity sensor 3 analog input
Flyer   | B2.5      | PK0 (*AIN16*)     | IR Prox  | Proximity sensor 4 analog input
Flyer   | B2.6      | PK1 (*AIN17*)     | IR Prox  | Proximity sensor 5 analog input
Flyer   | B2.7      | PK2 (*AIN18*)     | IR Prox  | Proximity sensor 6 analog input
Flyer   | B2.8      | PK3 (*AIN19*)     | IR Prox  | Proximity sensor 7 analog input
Flyer   |           |                   | Lidar    | Enable output 0 (via 1k resistor)
Flyer   |           |                   | Lidar    | Enable output 1 (via 1k resistor)
Flyer   |           |                   | Lidar    | Enable output 2 (via 1k resistor)
Flyer   |           |                   | Lidar    | Enable output 3 (via 1k resistor)
Flyer   |           |                   | Lidar    | PWM input 0
Flyer   |           |                   | Lidar    | PWM input 1
Flyer   |           |                   | Lidar    | PWM input 2
Flyer   |           |                   | Lidar    | PWM input 3
Flyer   |           |                   | Xband    | Motion pulse input
Flyer   | A1.9      | PB2 (*I2C0SCL*)   | BNO055   | I2C SCL
Flyer   | A1.1      | PB3 (*I2C0SDA*)   | BNO055   | I2C SDA
Any     |           |                   | APA102   | SPI CLK
Any     |           |                   | APA102   | SPI MOSI
Winch   |           |                   | HX711    | SPI CLK
Winch   |           |                   | HX711    | SPI MISO
Winch   | C1.8      | PL1 (*PhA0*)      | Encoder  | Quadrature Phase A input
Winch   | C1.9      | PL2 (*PhB0*)      | Encoder  | Quadrature Phase B input
Winch   | C1.1      | PF1               | H-Bridge | Motor Enable output
Winch   | C1.2      | PF2 (*M0PWM2*)    | H-Bridge | Motor PWM A output
Winch   | C1.3      | PF3 (*M0PWM3*)    | H-Bridge | Motor PWM B output

