# Bot-Firmware

Firmware for Tuco Flyer bots. Each of the winches and the flyer itself contain a Tiva-C microcontroller with Ethernet. This common firmware codebase handles network configuration, processing incoming UDP commands, and broadcasting sensor readings over UDP.

## Peripherals

Bot     | Pin               | Device   | Function
------- | ----------------- | -------- | ---------------------------------
Flyer   | PC4 (*U7RX*)      | Gimbal   | Host to Gimbal Serial (*Pitch*)
Flyer   | PC5 (*U7TX*)      | Gimbal   | Gimbal to Host Serial (*Yaw*)
Flyer   | PE3 (*AIN0*)      | IR Prox  | Proximity sensor 0 analog input
Flyer   | PE2 (*AIN1*)      | IR Prox  | Proximity sensor 1 analog input
Flyer   | PE1 (*AIN2*)      | IR Prox  | Proximity sensor 2 analog input
Flyer   | PE0 (*AIN3*)      | IR Prox  | Proximity sensor 3 analog input
Flyer   | PK0 (*AIN16*)     | IR Prox  | Proximity sensor 4 analog input
Flyer   | PK1 (*AIN17*)     | IR Prox  | Proximity sensor 5 analog input
Flyer   | PK2 (*AIN18*)     | IR Prox  | Proximity sensor 6 analog input
Flyer   | PK3 (*AIN19*)     | IR Prox  | Proximity sensor 7 analog input
Flyer   |                   | Lidar    | Enable output 0 (via 1k resistor)
Flyer   |                   | Lidar    | Enable output 1 (via 1k resistor)
Flyer   |                   | Lidar    | Enable output 2 (via 1k resistor)
Flyer   |                   | Lidar    | Enable output 3 (via 1k resistor)
Flyer   |                   | Lidar    | PWM input 0
Flyer   |                   | Lidar    | PWM input 1
Flyer   |                   | Lidar    | PWM input 2
Flyer   |                   | Lidar    | PWM input 3
Flyer   |                   | Xband    | Motion pulse input
Flyer   | PB2 (*I2C0SCL*)   | BNO055   | I2C SCL
Flyer   | PB3 (*I2C0SDA*)   | BNO055   | I2C SDA
Any     |                   | APA102   | SPI CLK
Any     |                   | APA102   | SPI MOSI
Winch   |                   | HX711    | SPI CLK
Winch   |                   | HX711    | SPI MISO
Winch   | PL1 (*PhA0*)      | Encoder  | Quadrature Phase A input
Winch   | PL2 (*PhB0*)      | Encoder  | Quadrature Phase B input
Winch   |                   | H-Bridge | Motor Enable output
Winch   |                   | H-Bridge | Motor PWM A output
Winch   |                   | H-Bridge | Motor PWM B output
