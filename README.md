# Bot-Firmware

Firmware for Tuco Flyer bots. Each of the winches and the flyer itself contain a Tiva-C microcontroller with Ethernet. This common firmware codebase handles network configuration, processing incoming UDP commands, and broadcasting sensor readings over UDP. Each bot gets its own conditionally compiled firmware built from this tree.
