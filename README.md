# 🚗 Car Black Box

![Platform](https://img.shields.io/badge/Platform-PIC16F877A-blue)
![Board](https://img.shields.io/badge/Board-PICGENIOS-informational)
![Simulator](https://img.shields.io/badge/Simulator-PICSIMSLAB-blueviolet)
![Language](https://img.shields.io/badge/Language-C%20%28XC8%29-orange)
![Status](https://img.shields.io/badge/Status-In%20Development-yellow)
![License](https://img.shields.io/badge/License-MIT-green)

A embedded systems project that simulates the core functionality of an automotive black box (Event Data Recorder) on a **PIC16F877A** microcontroller, running on the **PICGENIOS** development board inside **PICSIMSLAB**. The system logs real-time speed, gear state, crash events, and timestamps to an AT24C04 EEPROM over I²C.

---

## 🎬 Demo

```
┌────────────────┐
│ TIME  EV   SD  │   ← LINE 1 labels
│ 09:41:03 GN 72 │   ← LINE 2 live values (time / gear / speed %)
└────────────────┘
```

---

## 📋 Table of Contents

- [Overview](#overview)
- [Hardware Requirements](#hardware-requirements)
- [Pin Configuration](#pin-configuration)
- [File Structure](#file-structure)
- [Architecture](#architecture)
- [Module Descriptions](#module-descriptions)
- [Serial Monitor Reference](#serial-monitor-reference)
- [Getting Started](#getting-started)
- [What I Learned](#what-i-learned)
- [Known Limitations](#known-limitations)
- [License](#license)

---

## Overview

Modern vehicles carry Event Data Recorders (EDRs) that capture pre- and post-crash data. This project implements a simplified version on a PIC16F877A. The system covers:

- **Real-time clock** via DS1307 over I²C
- **Speed sensing** via ADC (potentiometer simulating a speed sensor)
- **Gear + crash event logging** via a 6-switch digital keypad
- **Live dashboard** on a 16×2 Character LCD
- **UART serial output** for real-time monitoring of every event
- **PIN-protected access menu** with lockout and idle timeout
- **Circular EEPROM log** — 10 entries, oldest overwritten, atomic page writes

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
| Storage | AT24C04 EEPROM on I²C bus |
| Communication | RS-232 / USB-UART bridge at 9600 baud |

---

## Pin Configuration

```
PIC16F877A (PICGENIOS board)
├── PORTD [RD0–RD7]  ──►  CLCD Data Bus (D0–D7)
├── RE1              ──►  CLCD EN
├── RE2              ──►  CLCD RS
├── RC3 (SCL)        ──►  DS1307 + AT24C04 SCL  (I²C)
├── RC4 (SDA)        ──►  DS1307 + AT24C04 SDA  (I²C)
├── RC6 (TX)         ──►  UART TX → PC serial monitor
├── RC7 (RX)         ──►  UART RX (future use)
├── RB0              ──►  SW1 (crash / menu back)
├── RB1              ──►  SW2 (gear up / menu down)
├── RB2              ──►  SW3 (gear down / menu up)
├── RB3              ──►  SW4 (enter login / digit '0' / select)
├── RB4              ──►  SW5 (digit '1')
└── AN0 (RA0)        ──►  Speed potentiometer
```

---

## File Structure

```
Car_Black_Box/
├── README.md
├── LOG.md
└── src/
    ├── main.c              Entry point, 4-phase event-driven loop
    ├── main_config.h       Global config, SYSTEM_STATE, all includes
    ├── events.h / .c       EVENT enum + translate_key() — hardware→app bridge
    ├── state.h / .c        Application state machine (DASHBOARD…CHANGE_PASSWORD)
    ├── timer.h / .c        Timer0 (10 ms) + Timer1 (100 ms) ISR driver
    ├── dashboard.h / .c    Dashboard update (hardware→sys) and render (sys→LCD)
    ├── login.h / .c        PIN login — update/render split, LOGIN_PHASE enum
    ├── menu.h / .c         Scrollable menu — update/render split
    ├── view_logs.h / .c    Log viewer (CLCD) and downloader (UART)
    ├── eeprom.h / .c       AT24C04 circular log driver — 10-slot, atomic page write
    └── blackbox_drivers.h  Unified peripheral API (UART, I²C, CLCD, DS1307, keypad, ADC)
    └── blackbox_drivers.c  Peripheral driver implementations
```

---

## Architecture

### Design Model

The firmware follows a strict embedded design model: **Inputs → State Machine → Outputs → Storage**.

```
                    ┌──────────────────────────────────────────┐
                    │              main.c while(1)              │
                    │                                           │
                    │  1. READ    key = read_digital_keypad()   │
                    │             evt = translate_key(key)      │
                    │                                           │
                    │  2. UPDATE  switch(state) {               │
                    │               DASHBOARD: dashboard_update │
                    │               LOGIN:     login_update     │
                    │               MENU:      menu_update      │
                    │               VIEW_LOGS: view_logs_update │
                    │             }                             │
                    │                                           │
                    │  3. RENDER  switch(state) {               │
                    │               DASHBOARD: dashboard_render │
                    │               LOGIN:     login_render     │
                    │               MENU:      menu_render      │
                    │               VIEW_LOGS: view_logs_render │
                    │             }                             │
                    │                                           │
                    │  4. STORAGE if(sys.log_pending)           │
                    │               eeprom_write_log()          │
                    └──────────────────────────────────────────┘
```

### Core Rules

**Rule 1 — Update never writes the LCD. Render never reads hardware.**
`dashboard_update()` calls `ds1307_i2c_read()` and `read_adc()`. It writes `sys`. It never calls `clcd_print()`. `dashboard_render()` reads `sys` and calls `clcd_print()`. It never calls a hardware driver. This separation makes both functions individually testable and prevents hidden side effects.

**Rule 2 — One event per cycle.**
The keypad is polled exactly once in step 1. The raw byte is immediately translated to an `EVENT` enum value. All modules downstream receive the `EVENT` — never the raw keypad byte. This means no module can misinterpret which physical key was pressed.

**Rule 3 — Single authoritative state (`sys`).**
All vehicle data (time, gear, speed, flags) lives in one `SYSTEM_STATE sys` struct defined in `main.c`. No module holds a private copy of vehicle data. When `dashboard_update()` reads a new speed from the ADC, it writes to `sys.speed`. When `dashboard_render()` needs the speed, it reads `sys.speed`. There is one truth.

**Rule 4 — EEPROM writes happen in step 4, never inside a module.**
`dashboard_update()` sets `sys.log_pending = 1`. The actual `eeprom_write_log()` call happens at the bottom of the main loop. This keeps the EEPROM call one level shallower in the call stack — critical on the PIC16F877A's 8-level hardware stack.

**Rule 5 — ISR only increments counters.**
The Timer0/Timer1 ISR increments `blink_tick` and `timeout_tick` and returns. All decisions based on those counters are made in `login_update()` running in the main loop. This makes the ISR's behaviour predictable and prevents race conditions.

### Event Flow

```
Physical keypress
      │
      ▼
read_digital_keypad(EDGE)    → raw unsigned char (SW1..SW6 mask)
      │
      ▼
translate_key()              → EVENT enum value
      │
      ▼
active_screen_update(EVENT)  → modifies SYSTEM_STATE or module statics
      │
      ▼
active_screen_render()       → reads state, writes LCD
      │
      ▼
eeprom_write_log()           → if sys.log_pending (step 4)
```

### Call Stack Depth (PIC16F877A: 8-level limit)

| Deepest chain | Depth from main | + ISR | Total |
|---|---|---|---|
| `eeprom_write_log → eeprom_write_byte → i2c_start → i2c_wait_for_idle` | 5 | 1 | **6** ✅ |
| `dashboard_update → ds1307_i2c_read → i2c_start → i2c_wait_for_idle` | 5 | 1 | **6** ✅ |
| `dashboard_render → clcd_print → clcd_write` | 4 | 1 | **5** ✅ |

The three inlining decisions that make this safe: `flush_pending_logs()` removed, `write_slot()` merged into `eeprom_write_log()`, `ack_poll()` inlined.

---

## Module Descriptions

### `events` — Hardware-to-Application Bridge
`translate_key(unsigned char key)` is the single point where raw hardware codes become semantic application events. Every module that reacts to input takes an `EVENT`; none takes a raw keypad byte. This makes the meaning of every key context-independent at the driver level and context-specific at the module level.

### `main_config` — Global Configuration
Defines hardware constants (`FOSC`, `_XTAL_FREQ`), the `SYSTEM_STATE` struct, `LOG_ENTRY` struct, `GEAR_STATE` enum, and event flags. Includes all module headers in dependency order. Nothing else should define hardware constants.

### `timer` — Hardware Timer ISR
Configures Timer0 (8-bit, 1:256, ≈10 ms overflow → `blink_tick`) and Timer1 (16-bit, 1:8, ≈100 ms overflow → `timeout_tick`). The ISR increments both counters and returns immediately — no LCD writes, no EEPROM writes, no state decisions. Modules read the counters and make decisions in the main loop.

### `dashboard` — Live Vehicle Display
Two strictly separate functions:
- `dashboard_update(EVENT)` reads RTC (I²C), ADC, and processes gear/crash keys. Writes `sys`. Never calls `clcd_print()`.
- `dashboard_render()` reads `sys` and draws time, gear, and speed on the LCD. Has a `prev_speed` cache to skip writes when speed hasn't changed. Never calls a hardware driver.

### `login` — PIN Entry Screen
Uses a `LOGIN_PHASE` enum (`ENTERING`, `FAIL`, `SUCCESS`, `LOCKED`) as the internal state machine. `login_update(EVENT)` advances the phase — processes digits, runs `pass_match()`, decrements `attempts_left`. `login_render()` displays based on the current phase. The 2-second failure-feedback delay is in `render()` because it is display behaviour: it fires once per `PHASE_FAIL` entry, then resets the phase to `ENTERING`. Entry cooldown is 500 ms based on `timeout_tick` (hardware Timer1), not loop iterations.

### `menu` — Scrollable Selection Menu
`menu_update(EVENT)` moves a `selection` index (0–4) and calls `set_status()` on SW4. `menu_render()` draws the selected item and the item below it. Redraws only when `selection != last_drawn`. `last_drawn` is a cache sentinel (`0xFF`); it is **never** used as an array index (see LOG.md — `last_i` OOB bug).

### `view_logs` — EEPROM Log Viewer and Downloader
`view_logs_update(EVENT)` moves a `log_index` and sets `needs_draw = 1`. `view_logs_render()` reads the EEPROM entry at `log_index` only when `needs_draw` is set, then clears it. Gear bytes from EEPROM are raw `GEAR_STATE` indices — `gear_label()` translates them to display strings (`"GR"`, `"GN"`, `"G1"`…) at every read site. `download_logs()` dumps all entries over UART in one pass then auto-transitions to MENU.

### `eeprom` — Circular Log Driver
10-slot circular buffer. `head` = next write slot, `count` = valid entries. Writes use AT24C04 **page write** (11 bytes in one I²C transaction) — atomic: either all 11 bytes commit or none do. `ack_poll()` is inlined. `write_slot()` is merged into `eeprom_write_log()`. Both inlining decisions were made to reduce call stack depth on the PIC16F877A.

### `blackbox_drivers` — Peripheral API
Unified driver for UART, I²C MSSP master, HD44780 CLCD, DS1307 RTC, 6-switch keypad, and 10-bit ADC. `uart_getchar()` is non-blocking — returns 0 if no byte is available. `uart_data_ready()` returns `RCIF` for poll-before-read. `i2c_wait_for_idle()` is `static` inside the `.c` file — not exposed in the header.

### `state` — Application State Machine
`set_status(STATE)` calls `clcd_clear()`, logs `[STATE] name\n` over UART, and updates `current_state`. `get_status()` returns it. Every screen transition is visible in the serial monitor.

---

## Serial Monitor Reference

Connect a USB-UART bridge to RC6/RC7. Open a terminal at **9600 8N1**. Every significant event produces a tagged line:

| Prefix | Meaning |
|---|---|
| `[STATE] DASHBOARD` | State machine transitioned to that screen |
| `[DASH] GEAR: G2` | Gear changed on dashboard |
| `[DASH] CRASH` | SW1 pressed — crash latched |
| `[DASH] IGNITION ON` | First gear/crash key pressed |
| `[LOGIN] digit 2` | Second digit entered on login screen |
| `[LOGIN] FAIL - 3 left` | Wrong PIN, 3 attempts remaining |
| `[LOGIN] TIMEOUT` | Idle 5 s — returned to dashboard |
| `[LOGIN] ACCESS GRANTED` | Correct PIN |
| `[LOGIN] LOCKED` | All attempts exhausted |
| `[MENU] SELECT: VIEW LOG` | Menu item selected |
| `[MENU] BACK` | Returned to dashboard from menu |
| `[LOGS] NEXT / PREV` | Scrolled log entries |
| `[LOGS] CLEARED` | All EEPROM log entries erased |
| `[DOWNLOAD] …` | UART log dump in progress |
| `[LOG] EEPROM OK` | Log entry saved successfully |
| `[LOG] EEPROM FAIL` | I²C write failed |

Download format:
```
=== CAR BLACK BOX LOG ===
## HH:MM:SS GEAR SPD
01 09:41:03 GN 072
02 09:43:17 G2 055
03 09:43:19 CR 055
=========================
```

---

## Getting Started

### Prerequisites
- MPLAB X IDE ≥ 6.x
- XC8 Compiler ≥ 2.x
- PICkit 3/4 or compatible programmer (or PICSIMSLAB for simulation)

### Build & Flash
1. Clone the repository.
2. Open MPLAB X and create a new project targeting **PIC16F877A @ 20 MHz**.
3. Add all `.c` files in `src/` to the project (including `events.c`).
4. Build and program via PICkit, or load into PICSIMSLAB.

### Serial Monitor
Connect a USB-UART bridge to RC6 (TX) / RC7 (RX). Open a terminal at **9600 8N1**.

---

## What I Learned

### 1. One Source of Truth for Hardware Constants
**Bug:** `#define FOSC 20000000` duplicated in `uart.h` and `i2c.h`.
**Concept:** Hardware constants belong in one top-level config header. Any peripheral that defines them creates a silent maintenance hazard.

### 2. Namespace Hygiene in C
**Bug:** UART functions named `getchar`, `putchar`, `puts` — reserved by `<stdio.h>`.
**Concept:** C has no namespaces. Module-prefixed names (`uart_putchar`, `i2c_read`) are the only reliable way to prevent collisions with the standard library.

### 3. `static` Linkage and Header Files
**Bug:** `static void i2c_wait_for_idle()` declared in the public header.
**Concept:** `static` means internal linkage — declaring it in a header defeats the purpose and gives every translation unit its own copy.

### 4. ADC Startup Sequencing
**Bug:** `GO = 1` placed before `ADON = 1` inside `initADC()`.
**Concept:** Initialisation functions configure — they do not trigger operations. Embedded datasheets specify strict startup sequences.

### 5. I²C ACK/NACK Protocol
**Bug:** `i2c_read(0)` (ACK) called after a single-byte DS1307 read.
**Concept:** The master sends NACK after the last byte to signal the slave to stop. ACK means "send me another byte." Always verify argument semantics against the callee's documented contract.

### 6. Never Use Empty Loops for Timing
**Bug:** `for(i=0;i<50;i++);` used as 1.52 ms delay in `clcd_clear()`. Eliminated at `-O2`.
**Concept:** `__delay_ms()` in XC8 uses `_XTAL_FREQ` to generate compiler-intrinsic delays that survive optimisation. Empty loops are not portable timing.

### 7. Unsigned Types for Array Indices
**Bug:** `static char i` (signed) used as gear index — wraps to -1 below zero.
**Concept:** Array indices must always be unsigned. A signed index that wraps below zero silently reads memory before the array.

### 8. Non-Blocking State Machines
**Bug:** `verify_password()` recursively called `show_login_screen()` on failure.
**Concept:** Embedded event loops require every function to return promptly. Recursion and blocking traps starve the loop of CPU time, preventing keypad polling and LCD updates.

### 9. Context-Dependent Input Routing
**Bug:** `if(key == SW4)` at the top of `while(1)` triggered LOGIN transition even when already on the Login screen.
**Concept:** The meaning of an input depends entirely on the active state. Input routing belongs strictly inside the originating state's case block, never at global scope.

### 10. Hardware Timers vs. Software Loop Counters
**Bug:** Blink cursor and idle timeout used raw `counter++` variables whose real duration depended on loop speed.
**Concept:** Hardware timers run independently of the CPU and fire at crystal-accurate intervals. Software loop counters break the moment loop speed changes.

### 11. PIC16F877A Hardware Stack Overflow (Intermittent Reboot)
**Bug:** Call chain from main to `i2c_wait_for_idle` was 8 levels deep. Timer ISR pushed depth to 9. Stack pointer wrapped, corrupting the return address of `main()`. The reboot was intermittent because it only occurred when a timer interrupt fired during the ~5 ms EEPROM ACK-poll window.
**Concept:** On fixed-hardware-stack MCUs, every function call has a measurable cost. Audit call depth against the limit. Reduce depth by inlining small intermediate functions rather than adding abstraction.

### 12. Early Return Killing a Render Block (Dead Code)
**Bug:** In `display_event()`, an early `return` when `FLAG_CRASH` was set meant the `clcd_print("C ", ...)` render block below it was never reached. The crash display was dead code.
**Concept:** When a function has multiple exit points, trace every path to confirm all side effects are reached. Render functions should have one exit point and one render path for each state.

### 13. Gear Label Corruption (Black Box Data Integrity)
**Bug:** EEPROM stored gear as raw index (`'0'`=GR, `'1'`=GN…). The viewer printed `"G"` + raw digit, making GN display as `"G1"` (First Gear) and GR as `"G0"`. This would mislead forensic analysis.
**Concept:** Any time data is stored in a compact representation, a dedicated decode function must be used at every read site. Printing the raw storage value is always wrong.

### 14. Loop-Speed-Dependent Cooldown Was Instantaneous
**Bug:** `entry_cooldown < 50` incremented once per loop iteration. After the ISR refactor made the loop faster, 50 iterations completed in under 0.2 ms — effectively no cooldown.
**Concept:** Any timing requirement, even a debounce cooldown, must be based on a hardware timer. Loop counters produce timing that silently changes whenever the loop body changes.

### 15. EEPROM Partial-Write Record Corruption
**Bug:** 11 separate byte writes. If a power fault occurred on byte 6, the remaining bytes kept the stale data from a previous record. `head`/`count` were only updated on success, but the partial record sat permanently in that slot.
**Concept:** When multiple writes must be atomic, use the hardware's native batch mechanism. I²C page write commits all 11 bytes or none.

### 16. Blocking `uart_getchar()` Was a Latent System Freeze
**Bug:** `while (!RCIF);` — permanent block waiting for a UART byte. Never called in production, but waiting like a landmine for anyone implementing SET_TIME via serial.
**Concept:** Driver functions must be non-blocking. Blocking behaviour is composed explicitly in the caller. Provide `uart_data_ready()` for poll-before-read.

### 17. Sentinel Value Used as Array Index (Menu OOB Read)
**Bug:** `last_i` initialised to `0xFF` as a "force redraw" sentinel. The SW4 handler called `menu_states[last_i]` — an out-of-bounds read into arbitrary data memory.
**Concept:** Sentinel values must never be dereferenced as array indices. Keep the live selection index and the cache-invalidation sentinel in separate variables with distinct roles.

### 18. Separation of UPDATE and RENDER
**Concept (design principle):** The biggest architectural improvement in this codebase. `_update()` reads hardware and mutates state. `_render()` reads state and writes the display. Neither does the other's job. This makes both functions individually testable, prevents hidden side effects, and makes the system's data flow explicit and auditable.

### 19. Single Authoritative State (SYSTEM_STATE)
**Concept (design principle):** Before this refactor, vehicle state was scattered — gear lived as a `static` inside `display_event()`, speed lived inside `current_log`, time fields were re-read inside a render function. `SYSTEM_STATE sys` is one struct that holds all vehicle data. One owner (dashboard_update writes it). One reader per field. No hidden copies.

### 20. Event Abstraction Over Raw Keycodes
**Concept (design principle):** Passing raw keypad bytes (`SW1 = 0x3E`) into every module is a coupling between hardware and application logic. `translate_key()` is the single conversion point. Modules receive `EVENT_SW1` — a semantic label. When hardware changes (different keypad, different pin), only `translate_key()` needs updating.

---

## Known Limitations

- No date register: only HH:MM:SS is read from DS1307. Date requires DS1307 registers 0x03–0x06.
- Crash flag not latched across power cycles. A reboot clears `sys.flags`.
- SET_TIME and CHANGE_PASSWORD are stubs — they log a message and return to MENU.
- Single ADC channel: only speed (AN0) is sampled.
- Login password is hardcoded as `"1111"`. CHANGE_PASSWORD is not yet implemented.

---

## License

MIT License — see [LICENSE](LICENSE) for details.