![Static Badge](https://img.shields.io/badge/MCU-ATSAME51-green "MCU:ATSAME51")
![Static Badge](https://img.shields.io/badge/IDE-MPLAB_X_V6.20-green "IDE:MPLAB_X_V6.20")
![Static Badge](https://img.shields.io/badge/BOARD-Igloo64-green "BOARD:Igloo64")

# SAME51RGBVideo #

Firmware for Atmel/Microchip ATSAME51J20A microcontroller (ARM) on Igloo64 prototyping board.
The board has nine big LEDs, but this code does not use them.
It generates line and frame sync pulses at CCIR System I rates on GPIO pins.
As yet, it only generates rudimentary video.
256x240 resolution, not unlike video game consoles of the 1980s.

All video generation will take place in the TC3 timer ISR,
which is triggered every 20ms (50Hz).
Game logic and/or video RAM updates must occur within about 1ms after the
frame sync pulse begins (i.e. during frame blanking).

## Connections ##

| Signal | SAM E51 pin | Name    | Notes                                            |
|--------|-------------|---------|--------------------------------------------------|
| VIDEO  | 1-4, 13-16  | PA00-07 | 8-bit RGB digital video                          |
| VSYNC  |     5       | PB04    | Vertical or frame sync, 50Hz                     |
| HSYNC  |     6       | PB05    | Horizontal or line sync, 15.625kHz               |
| TxD    |     17      | PA08    | Transmits 'F' at 9600 baud during frame blanking |
| RxD    |     18      | PA09    | Unused at present                                |

GPIO pin assignments are by physical proximity on the PCB.

## Compiling ##

Code written in MPLAB X V6.20 and compiled with 'xc32' compiler V4.60.


