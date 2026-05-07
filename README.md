# 🚗 Car Black Box

![Platform](https://img.shields.io/badge/Platform-PIC16F877A-blue)
![Board](https://img.shields.io/badge/Board-PICGENIOS-informational)
![Simulator](https://img.shields.io/badge/Simulator-PICSIMSLAB-blueviolet)
![Language](https://img.shields.io/badge/Language-C%20%28XC8%29-orange)
![Build](https://github.com/<your-username>/car-black-box/actions/workflows/build.yml/badge.svg)
![Status](https://img.shields.io/badge/Status-In%20Development-yellow)
![License](https://img.shields.io/badge/License-MIT-green)

A embedded systems project that simulates the core functionality of an automotive black box (Event Data Recorder) on a **PIC16F877A** microcontroller, running on the **PICGENIOS** development board inside **PICSIMSLAB**. The system logs real-time speed, gear state, crash events, and timestamps — mirroring the essential behaviour found in production EDR systems.

---

## 🎬 Demo

> 📽️ **Recording instructions:** Open the project in PICSIMSLAB, run the simulation, and use a screen-capture tool (e.g. ScreenToGif on Windows, Peek on Linux) to record a short clip of the LCD dashboard updating. Export as a GIF, save it to `docs/demo.gif`, and replace the placeholder below.

<!-- Once recorded, replace this block with:
![Car Black Box Demo](docs/demo.gif)
-->

```
┌────────────────┐
│ TIME  EV   SD  │   ← LINE 1 labels
│ 09:41:03 GN 72 │   ← LINE 2 live values (time / gear / speed %)
└────────────────┘
```

*Above: expected CLCD output at rest. GIF coming soon.*

---

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Pin Configuration](#pin-configuration)
- [File Structure](#file-structure)
- [Module Descriptions](#module-descriptions)
- [Architecture](#architecture)
- [Getting Started](#getting-started)
- [Milestones](#milestones)
- [Known Limitations](#known-limitations)
- [What I Learned](#what-i-learned)
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
- **PIN-protected access menu** with lockout and idle timeout

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
| Login Screen | 4-digit PIN with ISR-driven blink cursor |
| Idle Timeout | Auto-return to dashboard after 30 s |
| Access Lockout | Device locks after 4 failed PIN attempts |
| State Machine | Non-blocking event-driven architecture |

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
│   ├── workflows/
│   │   └── build.yml           # XC8 CI pipeline
│   ├── ISSUE_TEMPLATE/
│   │   ├── bug_report.md
│   │   └── feature_request.md
│   ├── PULL_REQUEST_TEMPLATE.md
│   └── labels.yml              # Issue label definitions
└── src/
    ├── main.c                  # Entry point, state-machine loop
    ├── main_config.h           # Global config, includes, forward declarations
    ├── timer.c / .h            # Timer0 (10 ms) + Timer1 (100 ms) ISR driver
    ├── dashboard.c             # CLCD dashboard: time, speed, events
    ├── state.c / .h            # Application state machine
    ├── login.c / .h            # PIN login screen
    ├── clcd.c / .h             # HD44780 Character LCD driver
    ├── uart.c / .h             # UART serial driver
    ├── i2c.c  / .h             # I²C (MSSP) master driver
    ├── ds1307.c / .h           # DS1307 RTC driver
    ├── adc.c  / .h             # ADC driver
    └── digital_keypad.c / .h  # 6-switch keypad driver
```

---

## Module Descriptions

### `timer` — Hardware Timer ISR Driver
Configures Timer0 (8-bit, 1:256 prescaler, ~10 ms tick) and Timer1 (16-bit, 1:8 prescaler, ~100 ms tick). The single ISR entry point services both overflow flags. Application modules read `blink_tick` and `timeout_tick` directly — no software loop counters anywhere in the codebase.

### `clcd` — Character LCD Driver
Drives a HD44780-compatible 16×2 LCD in 8-bit parallel mode. Exposes `clcd_print()`, `clcd_putch()`, and `clcd_clear()`. Initialisation follows the HD44780 recommended power-on sequence.

### `uart` — Serial Driver
Configures the USART for asynchronous TX/RX at a caller-specified baud rate using the FOSC constant. Provides `uart_putchar()`, `uart_getchar()`, and `uart_puts()`.
> ⚠️ Functions are prefixed `uart_` to avoid conflicts with the C standard library.

### `i2c` — I²C Master Driver
Implements I²C start/stop/repeat-start, byte write, and byte read primitives using the PIC16F877A MSSP module. `i2c_wait_for_idle()` is `static` inside `i2c.c` and not exposed in the header.

### `ds1307` — RTC Driver
Reads and writes individual DS1307 registers over I²C. `init_rtc()` clears the Clock Halt (CH) bit on startup to ensure the oscillator is running.

### `adc` — ADC Driver
Configures the 10-bit ADC with Fosc/8 clock, right-justified result. `read_adc(channel)` selects the channel, starts the conversion, and returns the 10-bit result.

### `digital_keypad` — Keypad Driver
Polls PORTB for 6 active-low switches. Supports `LEVEL` (continuous) and `EDGE` (single-fire on press) trigger modes with software debounce.

### `dashboard` — CLCD Dashboard
Composes the live dashboard from the above modules: timestamp on LINE1/LINE2, gear/event state, and scaled speed value. Uses a `prev_speed` cache to skip LCD writes when the value has not changed.

### `login` — PIN Login Screen
4-digit binary PIN entry (SW4 = '0', SW5 = '1'). Blink cursor driven by `blink_tick` (Timer0). 30-second idle timeout driven by `timeout_tick` (Timer1). Locks the device after 4 failed attempts.

### `state` — Application State Machine
Single source of truth for the active screen. `set_status()` clears the LCD before transitioning, ensuring a clean canvas for every new state.

---

## Architecture

```
                         ┌──────────────────────────────┐
                         │          main.c              │
                         │  key = read_digital_keypad() │  ← Single poll per cycle
                         │  switch(get_status())        │
                         └───┬───────────┬──────────────┘
                             │           │
                    ┌────────▼──┐   ┌────▼────────┐
                    │ DASHBOARD │   │   LOGIN     │  (+ MENU, LOGS, SET_TIME …)
                    │dashboard.c│   │  login.c    │
                    └─────┬─────┘   └──────┬──────┘
                          │                │
            ┌─────────────▼────────────────▼──────────────┐
            │              blackbox_drivers               │
            │  clcd  │  uart  │  i2c  │  ds1307  │  adc  │
            └─────────────────────────────────────────────┘
                          ▲
            ┌─────────────┴─────────────┐
            │           timer.c         │
            │  Timer0 ISR → blink_tick  │
            │  Timer1 ISR → timeout_tick│
            └───────────────────────────┘
```

---

## Getting Started

### Prerequisites
- MPLAB X IDE ≥ 6.x
- XC8 Compiler ≥ 2.x
- PICkit 3/4 or compatible programmer

### Build & Flash
1. Clone the repository.
2. Open MPLAB X and create a new project targeting **PIC16F877A @ 20 MHz**.
3. Add all `.c` files in `src/` to the project.
4. Set the XC8 optimization level to `-O1` or higher.
5. Build and program via your PICkit.

### Serial Monitor
Connect a USB-UART bridge to RC6/RC7 and open a terminal at **9600 8N1** to observe debug output.

---

## Milestones

| Milestone | Scope | Status |
|---|---|---|
| **v1.0 — Core** | Stable state machine, login, dashboard, RTC, ADC, Timer ISR | 🔄 In Progress |
| **v1.1 — EEPROM** | Persistent crash/gear event logging to internal EEPROM | 📋 Planned |
| **v1.2 — Menu** | Full menu implementation: VIEW\_LOGS, SET\_TIME, CHANGE\_PASSWORD | 📋 Planned |

Issues are tagged with milestone labels (`v1.0 Core`, `v1.1 EEPROM`, `v1.2 Menu`). See `.github/labels.yml` for the full label set.

---

## Known Limitations

- No persistent storage: events are not written to non-volatile memory (EEPROM/Flash) — planned for v1.1.
- Single-channel ADC: only one analogue input (speed) is currently sampled.
- No date register support in RTC: only HH:MM:SS is read from the DS1307.
- Crash flag is not latched across power cycles.
- Menu state handlers are stubbed — planned for v1.2.

---

## What I Learned

This section maps every significant bug caught during development to the underlying embedded systems concept it exposed.

### 1. One Source of Truth for Hardware Constants
**Bug:** `#define FOSC 20000000` was duplicated in `uart.h` and `i2c.h`, causing macro-redefinition warnings.
**Concept:** In embedded C, global hardware properties (oscillator frequency, crystal value) belong in a single top-level config header. Any peripheral that defines them independently creates a silent maintenance hazard — change one and the other drifts.

### 2. Namespace Hygiene in C
**Bug:** UART functions were named `getchar`, `putchar`, and `puts`, clashing with `<stdio.h>` reserved identifiers and causing linker errors.
**Concept:** C has no namespaces. The MISRA-C convention of module-prefixed names (`uart_putchar`, `i2c_read`) is the only reliable way to prevent collisions with the standard library and other drivers.

### 3. `static` Linkage and Header Files
**Bug:** A `static void i2c_wait_for_idle()` declaration appeared in the public header, giving every translation unit its own copy of the function.
**Concept:** `static` in C means *internal linkage* — the function exists only within its translation unit. Declaring it in a header defeats this entirely. Private helpers belong only in the `.c` file.

### 4. ADC Startup Sequencing
**Bug:** `GO = 1` (start conversion) was placed inside `initADC()`, before `ADON = 1` (power on the ADC module).
**Concept:** Peripheral initialisation functions configure — they do not trigger operations. Embedded datasheets specify strict startup sequences (acquisition time, module enable order) that must be followed or the hardware returns undefined results.

### 5. I²C ACK/NACK Protocol
**Bug:** `i2c_read(0)` (send ACK) was called after a single-byte read from the DS1307. The master must send NACK after the *last* byte to signal the slave to stop transmitting.
**Concept:** The I²C specification (and the DS1307 datasheet §5.0) are explicit: ACK = "send me another byte"; NACK = "I'm done". Sending ACK on a single-byte read holds the bus and corrupts the next transaction. Always verify argument semantics against the callee's documented contract.

### 6. Timing in Embedded C — Never Use Empty Loops
**Bug:** `for(unsigned char i = 0; i < 50; i++);` was used as a 1.5 ms delay after LCD Clear. At `-O2` the loop is eliminated by the compiler, sending the next command before the HD44780 controller finishes.
**Concept:** Empty delay loops are not portable timing mechanisms — their duration depends entirely on the optimisation level. `__delay_ms()` in XC8 uses the `_XTAL_FREQ` constant to generate compiler-intrinsic delays that are guaranteed to be optimisation-resistant.

### 7. Unsigned Types for Array Indices
**Bug:** A `static char` (signed on XC8 by default) was used as a gear index. Decrementing below 0 produced -1 — a valid negative array subscript causing an out-of-bounds memory read.
**Concept:** Array indices must always be unsigned. A signed index that wraps below zero silently reads from memory before the array. `unsigned char` or `uint8_t` prevents this class of bug entirely.

### 8. Non-Blocking State Machines
**Bug:** `verify_password()` recursively called `show_login_screen()` on failure, and used `while(1);` traps on success/lockout, permanently halting the main polling loop.
**Concept:** Embedded event loops require every function to return promptly. Recursion and blocking traps starve the loop of CPU time, preventing keypad polling, LCD updates, and interrupt servicing. The correct pattern is: update flags, execute hardware delays only when unavoidable, return immediately.

### 9. Context-Dependent Input Routing
**Bug:** An `if(key == SW4)` block at the top of `while(1)` acted as a global override. `SW4` triggered a state transition even when the user was already on the Login screen trying to type a digit.
**Concept:** In a state machine, the *meaning* of an input depends entirely on the active state. Input-routing logic belongs strictly inside its originating state case — never at global scope.

### 10. Hardware Timers vs. Software Loop Counters
**Bug (design flaw):** The blink cursor and idle timeout used raw `counter++` variables whose real-world duration depended on loop iteration speed — unquantifiable and untestable.
**Concept:** Any timing requirement in an embedded system should be met with a hardware timer. Timer0 and Timer1 on the PIC16F877A run independently of the CPU and fire ISRs at precise, crystal-accurate intervals. Software counters are a crutch that breaks the moment loop speed changes.

---

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting pull requests.

---

## License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.