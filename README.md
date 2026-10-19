# SCAU
# Self Contained Atmosphere Unit

A standalone indoor air monitor built on an ESP32. It measures temperature, humidity and air quality and shows them on a small round display. The display only turns on while someone is nearby.

## Features

- Temperature and humidity from a DHT22, read every 2.5 s
- Air-quality reading from an MQ135, shown on the display as **Good** / **Medium** / **Bad**
- A buzzer that sounds when the air quality reading is high/Bad
- A PIR motion sensor that wakes the display; it goes dark 15 s after the last motion
- Live sensor readings logged over serial
- The onboard LED blinks as a heartbeat to show the firmware is running

## Hardware

| Part | Purpose |
|---|---|
| ESP32 dev board (`esp32dev`) | Microcontroller |
| DHT22 | Temperature and humidity |
| MQ135 | Air quality (analog) |
| PIR motion sensor | Wakes the display |
| GC9A01 1.28" round TFT (240×240, SPI) | Display |
| Buzzer | Air-quality alarm |
| 2× 10 kΩ resistors | Voltage divider on the MQ135 analog output |

## Wiring

| Signal | ESP32 GPIO |
|---|---|
| DHT22 data | 27 |
| PIR output | 26 |
| MQ135 AO (through the divider) | 34 |
| Buzzer | 25 |
| TFT CS | 5 |
| TFT DC | 16 |
| TFT RST | 17 |
| TFT SCK | 18 |
| TFT MOSI (SDA) | 23 |
| Heartbeat LED | 2 (onboard) |

**MQ135 divider:** the module's AO pin can output up to 5 V, which is more than the ESP32's ADC can safely take. AO feeds a 10 kΩ / 10 kΩ divider, and the midpoint goes to GPIO 34. That halves the voltage, so the firmware multiplies the reading by 2 to get the sensor's actual output voltage.

## Air-quality thresholds

These use the raw 12-bit ADC value (0–4095) from GPIO 34:

| Raw value | Display | Buzzer |
|---|---|---|
| < 1000 | Good (green) | off |
| 1000–1199 | Medium (yellow) | off |
| 1200–1999 | Medium (yellow) | **on** |
| ≥ 2000 | Bad (red) | on |

The thresholds haven't been calibrated, so they show relative changes rather than real gas concentrations. The MQ135 also needs time to warm up after power-on before its readings settle.

## Building and flashing

This is a [PlatformIO](https://platformio.org/) project. The libraries are listed in `platformio.ini`, and PlatformIO downloads them automatically:

- `adafruit/DHT sensor library`
- `adafruit/Adafruit Unified Sensor`
- `moononournation/GFX Library for Arduino` (pinned to 1.4.9)

```sh
pio run -t upload      # build and flash
pio device monitor     # serial monitor at 115200 baud
```

## Serial output

Example:

```
ESP32 is still alive!
DHT22 -> Temp: 22.4 C  Hum: 48.1 %
MQ135 -> raw: 812  voltage: 1.31 V
PIR -> motion detected
```

## Known quirks

- The buzzer turns on at raw 1200, which falls inside the "Medium" band. Because of this, it can sound while the display still says Medium.
- When the display goes "dark", the firmware only fills the screen with black. The backlight stays on.
- The screen is fully redrawn every 500 ms, so it may flicker a little.
