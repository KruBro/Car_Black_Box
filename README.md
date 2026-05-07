# 🚗 Car Black Box

![Platform](https://img.shields.io/badge/Platform-PIC16F877A-blue)
![Board](https://img.shields.io/badge/Board-PICGENIOS-informational)
![Simulator](https://img.shields.io/badge/Simulator-PICSIMSLAB-blueviolet)
![Language](https://img.shields.io/badge/Language-C%20%28XC8%29-orange)
![Status](https://img.shields.io/badge/Status-In%20Development-yellow)
![License](https://img.shields.io/badge/License-MIT-green)

A embedded systems project that simulates the core functionality of an automotive black box (Event Data Recorder) on a **PIC16F877A** microcontroller, running on the **PICGENIOS** development board inside **PICSIMSLAB**. The system logs real-time speed, gear state, crash events, and timestamps — mirroring the essential behaviour found in production EDR systems.

---

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Pin Configuration](#pin-configuration)
- [File Structure](#file-structure)
- [Module Descriptions](#module-descriptions)
- [Getting Started](#getting-started)
- [Known Limitations](#known-limitations)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

Modern vehicles are equipped with Event Data Recorders (EDRs) that capture pre- and post-crash data. This project implements a simplified version of such a system as a learning exercise, using off-the-shelf peripherals wired to a PIC16F877A MCU. The project covers:

- **Real-time clock** via DS1307 over I²C
- **Speed sensing** via ADC (potentiometer simulating a speed sensor)
- **Gear + crash event logging** via a 6-switch digital keypad
- **Live dashboard** on a 16×2 Character LCD
- **UART serial output** for PC-side monitoring/logging

---

## Features

| Feature | Detail |
|---|---|
| Timestamp | Hours : Minutes : Seconds from DS1307 RTC |
| Speed Display | 0–100 % ADC reading on CLCD |
| Gear State | Reverse, Neutral, G1–G4 via keypad |
| Crash Event | Latching SW1 crash flag, locks gear input |
| UART Debug | 9600 baud serial output |
| CLCD Dashboard | 16×2 live display updated every loop |

---

## Hardware Requirements

| Component | Part Number / Spec |
|---|---|
| Microcontroller | PIC16F877A |
| Development Board | PICGENIOS |
| Simulation Environment | PICSIMSLAB |
| Crystal Oscillator | 20 MHz |
| RTC Module | DS1307 + 32.768 kHz crystal |
| Display | 16×2 HD44780 Character LCD |
| Speed Sensor (sim) | 10 kΩ potentiometer on AN0 |
| Input | 6-switch active-low keypad on PORTB |
| Communication | RS-232 / USB-UART bridge at 9600 baud |

---

## Pin Configuration

```
PIC16F877A (PICGENIOS board)
├── PORTD [RD0–RD7]  ──►  CLCD Data Bus (D0–D7)
├── RE1              ──►  CLCD EN
├── RE2              ──►  CLCD RS
├── RC3 (SCL)        ──►  DS1307 SCL  (I²C)
├── RC4 (SDA)        ──►  DS1307 SDA  (I²C)
├── RC6 (TX)         ──►  UART TX
├── RC7 (RX)         ──►  UART RX
├── RB0–RB5          ──►  SW1–SW6 (active-low, pull-up via PORTB)
└── AN0 (RA0)        ──►  Speed potentiometer
```

---

## File Structure

```
Car_Black_Box/
├── README.md
├── LOG.md
├── CONTRIBUTING.md
├── CODE_OF_CONDUCT.md
├── SECURITY.md
├── .github/
│   ├── ISSUE_TEMPLATE/
│   │   ├── bug_report.md
│   │   └── feature_request.md
│   └── PULL_REQUEST_TEMPLATE.md
└── src/
    ├── main.c              # Entry point, peripheral initialisation
    ├── main_config.h       # Global config, includes, forward declarations
    ├── dashboard.c         # CLCD dashboard: time, speed, events
    ├── clcd.c / .h         # HD44780 Character LCD driver
    ├── uart.c / .h         # UART serial driver
    ├── i2c.c  / .h         # I²C (MSSP) master driver
    ├── ds1307.c / .h       # DS1307 RTC driver
    ├── adc.c  / .h         # ADC driver
    └── digital_keypad.c/.h # 6-switch keypad driver
```

---

## Module Descriptions

### `clcd` — Character LCD Driver
Drives a HD44780-compatible 16×2 LCD in 8-bit parallel mode. Exposes `clcd_print()`, `clcd_putch()`, and `clcd_clear()`. Initialisation follows the HD44780 recommended power-on sequence.

### `uart` — Serial Driver
Configures the USART for asynchronous TX/RX at a caller-specified baud rate using the FOSC constant. Provides `uart_putchar()`, `uart_getchar()`, and `uart_puts()`.
> ⚠️ Functions are prefixed `uart_` to avoid conflicts with the C standard library. Note: the PIC16F877A has a standard USART, not the Enhanced EUSART found on PIC18 devices — register names are identical so no code changes are needed.

### `i2c` — I²C Master Driver
Implements I²C start/stop/repeat-start, byte write, and byte read primitives using the PIC16F877A MSSP module. `i2c_wait_for_idle()` is internal and not exposed in the header.

### `ds1307` — RTC Driver
Reads and writes individual DS1307 registers over I²C. `init_rtc()` clears the Clock Halt (CH) bit on startup to ensure the oscillator is running.

### `adc` — ADC Driver
Configures the 10-bit ADC with Fosc/8 clock, right-justified result. `read_adc(channel)` selects the channel, starts the conversion, and returns the 10-bit result.

### `digital_keypad` — Keypad Driver
Polls PORTB for 6 active-low switches. Supports `LEVEL` (continuous) and `EDGE` (single-fire on press) trigger modes with software debounce.

### `dashboard` — CLCD Dashboard
Composes the live dashboard from the above modules: timestamp on LINE1/LINE2, gear/event state, and scaled speed value.

---

## Getting Started

### Prerequisites
- MPLAB X IDE ≥ 6.x
- XC8 Compiler ≥ 2.x
- PICkit 3/4 or compatible programmer

### Build & Flash
1. Clone the repository.
2. Open MPLAB X and create a new project targeting **PIC18F4580 @ 20 MHz**.
3. Add all `.c` files in `src/` to the project.
4. Set the XC8 optimization level to `-O1` or higher.
5. Build and program via your PICkit.

### Serial Monitor
Connect a USB-UART bridge to RC6/RC7 and open a terminal at **9600 8N1** to observe debug output.

---

## Known Limitations

- No persistent storage: events are not written to non-volatile memory (EEPROM/Flash).
- Single-channel ADC: only one analogue input (speed) is currently sampled.
- No date register support in RTC: only HH:MM:SS is read from the DS1307.
- Crash flag is not latched across power cycles.

---

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting pull requests.

---

## License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.