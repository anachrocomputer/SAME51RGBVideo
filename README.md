![Static Badge](https://img.shields.io/badge/MCU-ATSAME51-green "MCU:ATSAME51")
![Static Badge](https://img.shields.io/badge/IDE-MPLAB_X_V6.20-green "IDE:MPLAB_X_V6.20")
![Static Badge](https://img.shields.io/badge/BOARD-Igloo64-green "BOARD:Igloo64")

# SAME51RGBVideo #

Firmware for Atmel/Microchip ATSAME51J20A microcontroller (ARM) on Igloo64 prototyping board.
The board has nine big LEDs, and this code blinks them.
It also generates an analog staircase waveform on the 12-bit DAC,
VOUT0.

## Connections ##

| Signal | SAM E51 pin | Name |
|--------|-------------|------|
| D1     |     1       | PA00 |
| D2     |     60      | PB31 |
| D3     |     59      | PB30 |
| D4     |     51      | PA27 |
| D5     |     50      | PB23 |
| D6     |     46      | PA25 |
| D7     |     41      | PA20 |
| D8     |     37      | PA18 |
| D9     |     32      | PA15 |
| VOUT0  |     3       | PA02 |
| VOUT1  |     14      | PA05 |
| AIN6   |     15      | PA06 |
| AIN7   |     16      | PA07 |
| TxD    |     17      | PA08 |
| RxD    |     18      | PA09 |

GPIO pin assignments are by physical proximity on the PCB.
ADC pin assignments attempt to avoid clashes with SERCOM pins.

## Compiling ##

Code written in MPLAB X V6.20 and compiled with 'xc32' compiler V4.60.


