# Nolicom

Nolicom is a two-device wireless communication indicator based on the ESP32 and ESP-NOW.

When one button is pressed, the paired device activates a buzzer and LEDs.  
The signal remains active as long as the button is held.  
Both devices use deep sleep to minimize power consumption.

Some assemble photos can be found on my personal website: [https://thiering.org/projects/nolicom](https://thiering.org/projects/nolicom)

## Hardware

- ESP32-WROOM-32
- 74HC595 shift register
- 2-digit 7-segment display (common anode)
- Passive piezo buzzer
- 3 LEDs (green, yellow, red)
- Push button
- 10k potentiometer
- Li-Ion battery
- TP4056 charging module
- MT3608 boost converter

## Communication

- ESP-NOW (peer-to-peer)
- No router required
- Low latency
- Low power usage
