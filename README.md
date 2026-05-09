# 🚗 Car Black Box — PIC16F877A Event Data Recorder

![Platform](https://img.shields.io/badge/Platform-PIC16F877A-blue)
![Board](https://img.shields.io/badge/Board-PICGENIOS-informational)
![Simulator](https://img.shields.io/badge/Simulator-PICSIMSLAB-blueviolet)
![Language](https://img.shields.io/badge/Language-C%20%28XC8%29-orange)
![Optimisation](https://img.shields.io/badge/Optimisation--Os-brightgreen)
![Status](https://img.shields.io/badge/Status-Active-success)
![License](https://img.shields.io/badge/License-MIT-green)

A firmware project that implements the core functionality of an automotive
**Event Data Recorder (EDR / "black box")** on a PIC16F877A 8-bit
microcontroller. The system captures real-time speed, gear state, crash
events, and timestamps into a circular EEPROM log, protected behind a
PIN-authenticated menu with lockout and idle timeout.

---

## 🗺️ Project Mind Map

```
CAR BLACK BOX (PIC16F877A)
│
├── 🔩 HARDWARE LAYER
│   ├── CPU ─────────── PIC16F877A @ 20 MHz (HS oscillator)
│   ├── RTC ─────────── DS1307 (I²C, BCD registers)
│   ├── Storage ──────── AT24C04 EEPROM 512B (I²C, page-write)
│   ├── Display ──────── HD44780 16×2 CLCD (8-bit parallel, PORTD)
│   ├── Input ────────── 6-switch active-low keypad (PORTB, RB0–RB5)
│   ├── Speed sensor ─── 10 kΩ pot → AN0 (ADC, 10-bit right-justified)
│   └── Debug port ───── UART RC6/RC7 @ 9600 8N1
│
├── 🧠 FIRMWARE ARCHITECTURE
│   ├── Design pattern ── Event-driven state machine
│   ├── Main loop ─────── 4-phase: READ → UPDATE → RENDER → STORAGE
│   ├── Key principle ─── Update ≠ Render (strict separation)
│   ├── Event model ───── Raw keycode → EVENT enum (translate_key)
│   └── Global state ──── SYSTEM_STATE sys (single source of truth)
│
├── 📦 MODULE MAP
│   ├── main.c ────────── Entry point, master loop, EEPROM flush
│   ├── main_config.h ─── Types, constants, include chain
│   ├── events.h/.c ───── EVENT enum + translate_key()
│   ├── state.h/.c ────── State machine (set_status / get_status)
│   ├── timer.h/.c ────── Timer0 (10 ms) + Timer1 (100 ms) ISR
│   ├── blackbox_drivers  Peripheral API (UART/I²C/CLCD/RTC/ADC/Keypad)
│   ├── dashboard.h/.c ── Live display (update + render)
│   ├── login.h/.c ────── PIN auth (phases, lockout, timeout)
│   ├── menu.h/.c ─────── Scrollable 6-item menu
│   ├── view_logs.h/.c ── CLCD log viewer + UART downloader
│   ├── set_time.h/.c ─── Field-edit RTC set screen
│   ├── set_password.h/.c Dual-buffer PIN change screen
│   └── eeprom.h/.c ───── Circular 10-slot EEPROM log driver
│
├── 🔄 STATE MACHINE
│   ├── DASHBOARD (boot default)
│   │   └── SW4 → LOGIN (if not logged in) or MENU (if logged in)
│   ├── LOGIN
│   │   └── Correct PIN → MENU
│   ├── MENU
│   │   ├── VIEW LOG     → VIEW_LOGS
│   │   ├── CLEAR LOG    → CLEAR_LOGS (one-shot)
│   │   ├── DOWNLOAD LOG → DOWNLOAD_LOGS (one-shot)
│   │   ├── SET TIME     → SET_TIME
│   │   ├── CHANGE PASS  → CHANGE_PASSWORD
│   │   └── LOGOUT       → DASHBOARD
│   ├── VIEW_LOGS       (scroll log entries on CLCD)
│   ├── CLEAR_LOGS      (one-shot: eeprom_clear_log → MENU)
│   ├── DOWNLOAD_LOGS   (one-shot: UART dump → MENU)
│   ├── SET_TIME        (field-edit time → DS1307 → MENU)
│   └── CHANGE_PASSWORD (dual-buffer PIN → EEPROM → MENU)
│
├── ⏱️ TIMING SUBSYSTEM
│   ├── Timer0 ── 8-bit, 1:256 prescaler → overflow ~10 ms → blink_tick
│   │   └── Used by: login cursor blink, entry cooldown, set_time blink
│   ├── Timer1 ── 16-bit, 1:8 prescaler → overflow ~100 ms → timeout_tick
│   │   └── Used by: login idle timeout, menu inactivity, lockout countdown
│   └── ISR ───── Increments counters only. Zero LCD writes. Zero decisions.
│
├── 💾 DATA STORAGE (AT24C04 EEPROM)
│   ├── Layout
│   │   ├── 0x00  head  (1 byte — next write slot 0–9)
│   │   ├── 0x01  count (1 byte — valid entries 0–10)
│   │   ├── 0x02–0x72  10 × 11-byte log records
│   │   └── 0xF0–0xF3  4-byte PIN (ASCII '0'/'1')
│   ├── Record format: "HHMMSSGxSSS" (11 ASCII bytes, no null)
│   │   ├── HH  hours  (BCD-decoded to 2 ASCII digits)
│   │   ├── MM  minutes
│   │   ├── SS  seconds
│   │   ├── G   literal prefix
│   │   ├── x   gear: '0'=GR '1'=GN '2'=G1 '3'=G2 '4'=G3 '5'=G4 'C'=crash
│   │   └── SSS speed 000–100
│   └── Write mechanism: I²C page-write (atomic — all 11 bytes or none)
│
└── 🔑 KEY DESIGN DECISIONS
    ├── No malloc — all buffers are statically declared
    ├── ISR only increments — zero application logic in ISR
    ├── One EVENT per main loop cycle — prevents double-consume bugs
    ├── EEPROM write in step 4 only — keeps call stack at depth ≤ 6
    └── const char * const for all string tables — Flash, not RAM
```

---

## 🏗️ Architecture Mind Map — 4-Phase Main Loop

```
                    ╔══════════════════════════════════════════╗
                    ║         main.c — while(1) loop           ║
                    ╠══════════════════════════════════════════╣
                    ║                                          ║
                    ║  ① READ                                  ║
                    ║  ┌─────────────────────────────────┐    ║
                    ║  │ key = read_digital_keypad(EDGE)  │    ║
                    ║  │ evt = translate_key(key)         │    ║
                    ║  └────────────────┬────────────────┘    ║
                    ║                   │ EVENT                ║
                    ║  ② UPDATE         ▼                      ║
                    ║  ┌─────────────────────────────────┐    ║
                    ║  │ switch(get_status()) {           │    ║
                    ║  │   DASHBOARD → dashboard_update() │    ║
                    ║  │   LOGIN     → login_update()     │    ║
                    ║  │   MENU      → menu_update()      │    ║
                    ║  │   VIEW_LOGS → view_logs_update() │    ║
                    ║  │   SET_TIME  → set_time_update()  │    ║
                    ║  │   CHANGE_PW → set_pw_update()    │    ║
                    ║  │ }                                │    ║
                    ║  └────────────────┬────────────────┘    ║
                    ║                   │ sys mutated          ║
                    ║  ③ RENDER         ▼                      ║
                    ║  ┌─────────────────────────────────┐    ║
                    ║  │ switch(get_status()) {           │    ║
                    ║  │   DASHBOARD → dashboard_render() │    ║
                    ║  │   LOGIN     → login_render()     │    ║
                    ║  │   MENU      → menu_render()      │    ║
                    ║  │   VIEW_LOGS → view_logs_render() │    ║
                    ║  │   CLEAR_LOGS→ clear_logs()       │    ║
                    ║  │   DOWNLOAD  → download_logs()    │    ║
                    ║  │   SET_TIME  → set_time_render()  │    ║
                    ║  │   CHANGE_PW → set_pw_render()    │    ║
                    ║  │ }                                │    ║
                    ║  └────────────────┬────────────────┘    ║
                    ║                   │ LCD updated          ║
                    ║  ④ STORAGE        ▼                      ║
                    ║  ┌─────────────────────────────────┐    ║
                    ║  │ if (sys.log_pending) {           │    ║
                    ║  │   eeprom_write_log(&entry)       │    ║
                    ║  │ }                                │    ║
                    ║  └─────────────────────────────────┘    ║
                    ╚══════════════════════════════════════════╝
```

### The 5 Invariants That Make This Work

| # | Invariant | Why |
|---|-----------|-----|
| 1 | `_update()` never writes the LCD | Prevents hidden render side-effects |
| 2 | `_render()` never reads hardware | Keeps render deterministic & testable |
| 3 | One EVENT per cycle | Prevents double-consume on state transitions |
| 4 | EEPROM write only in step 4 | Keeps call depth ≤ 6 (PIC stack limit = 8) |
| 5 | ISR only increments counters | No race conditions between ISR and main |

---

## 🔌 Hardware Mind Map

```
PIC16F877A (PICGENIOS development board, PICSIMSLAB simulation)
│
├── PORTD [RD0–RD7] ─────────────────── HD44780 CLCD Data Bus (D0–D7)
│   (8-bit parallel, write-only)
│
├── PORTE
│   ├── RE1 (CLCD_EN) ─────────────── CLCD Enable strobe (active high, 100 µs)
│   └── RE2 (CLCD_RS) ─────────────── CLCD Register Select (0=Inst, 1=Data)
│
├── PORTC
│   ├── RC3 (SCL) ──────────────────── I²C clock → DS1307 SCL + AT24C04 SCL
│   ├── RC4 (SDA) ──────────────────── I²C data  → DS1307 SDA + AT24C04 SDA
│   ├── RC6 (TX)  ──────────────────── UART transmit → PC serial monitor
│   └── RC7 (RX)  ──────────────────── UART receive (reserved for future use)
│
├── PORTB [RB0–RB5] ─────────────────── 6-switch keypad (active-low, pull-up)
│   ├── RB0 (SW1) — Crash / Login: back / Menu: back / Log: back
│   ├── RB1 (SW2) — Gear up / Menu: scroll down / Set Time: confirm
│   ├── RB2 (SW3) — Gear down / Menu: scroll up / Log: previous
│   ├── RB3 (SW4) — Enter login / Digit '0' / Select / Increment field
│   ├── RB4 (SW5) — Digit '1' / Decrement field
│   └── RB5 (SW6) — Set Time: cycle active field (HH→MM→SS→HH)
│
└── PORTA
    └── AN0 (RA0) ───────────────────── Speed potentiometer → ADC CH0
                                        0V = 0 km/h, Vref = 100 km/h
```

### I²C Bus Topology

```
PIC16F877A (Master)
    │
    ├── SCL ──────┬──────────── DS1307 RTC  (addr 0x68)
    │             └──────────── AT24C04 EEPROM (addr 0xA0/0xA2)
    └── SDA ──────┴──────────── (shared I²C bus, 100 kHz)
```

---

## 📁 File Structure Mind Map

```
Car_Black_Box/
│
├── README.md                  ← This file
├── LOG.md                     ← Bug log, session notes, design decisions
│
├── cmake/
│   └── Car_Black_Box/
│       ├── default/
│       │   └── user.cmake     ← [NEW] Override: -O0 → -Os optimisation
│       └── default.production/
│           └── user.cmake     ← [NEW] Same override for production build
│
└── src/
    │
    ├── CONFIGURATION
    │   ├── main_config.h      Global: FOSC, _XTAL_FREQ, SYSTEM_STATE,
    │   │                               LOG_ENTRY, GEAR_STATE, include chain
    │   └── events.h/.c        EVENT enum + translate_key() bridge
    │
    ├── INFRASTRUCTURE
    │   ├── state.h/.c         Application state machine (STATE enum, accessors)
    │   ├── timer.h/.c         Timer0 (blink_tick) + Timer1 (timeout_tick) ISR
    │   └── blackbox_drivers.h/.c  Peripheral API (UART/I²C/CLCD/RTC/ADC/Keypad)
    │
    ├── APPLICATION SCREENS
    │   ├── main.c             Entry point: init + 4-phase while(1) loop
    │   ├── dashboard.h/.c     Live vehicle display (RTC + ADC + gear/crash)
    │   ├── login.h/.c         4-digit PIN auth with lockout + timeout
    │   ├── menu.h/.c          Scrollable 6-item authenticated menu
    │   ├── view_logs.h/.c     CLCD log viewer + UART log downloader
    │   ├── set_time.h/.c      Blinking-field RTC editor (BCD ↔ decimal)
    │   └── set_password.h/.c  Dual-buffer PIN change with EEPROM persistence
    │
    └── STORAGE
        └── eeprom.h/.c        AT24C04 circular 10-slot log driver (page-write)
```

---

## 🧩 Module Reference

### `main.c` — Entry Point

The master coordinator. Owns the `SYSTEM_STATE sys` global struct. After
`init_config()` (which calls all peripheral inits and loads the PIN from EEPROM),
the infinite loop runs four phases:
1. **READ** — polls keypad once, translates to `EVENT`.
2. **UPDATE** — dispatches event to active screen's `_update()` function.
3. **RENDER** — dispatches to active screen's `_render()` function.
4. **STORAGE** — if `sys.log_pending`, packs and writes EEPROM log entry.

The EEPROM write is here (not inside `dashboard_update`) specifically to keep
the call stack shallower — critical on the PIC16F877A's 8-level hardware stack.

---

### `main_config.h` — Global Configuration

```
main_config.h provides:
├── Pragma config bits (HS oscillator, WDT off, LVP off)
├── FOSC = 20000000UL       → used by UART/I²C baud calc + __delay_ms
├── _XTAL_FREQ = 20000000UL → used by XC8 built-in delay macros
├── GEAR_STATE enum (GR, GN, G1, G2, G3, G4)
├── FLAG_IGNITION_ON = 0x01, FLAG_CRASH = 0x02
├── LOG_ENTRY struct (hours, minutes, seconds, speed, gear, flags)
├── SYSTEM_STATE struct (all vehicle data + log_pending)
└── Include chain (xc.h → blackbox_drivers.h → events → timer
                   → eeprom → state → login → set_password
                   → set_time → view_logs → menu → dashboard)
```

**Rule:** Nothing else defines `FOSC` or `_XTAL_FREQ`. Peripheral drivers
that need these values receive them transitively from this header.

---

### `events.h/.c` — Hardware-to-Application Bridge

```
Raw keypad byte (unsigned char)
        │
        ▼
  translate_key()
        │
        ▼
  EVENT enum value
  ├── EVENT_NONE    (no key / unknown)
  ├── EVENT_SW1     (semantic: crash / back / cancel)
  ├── EVENT_SW2     (semantic: gear-up / down-scroll / confirm)
  ├── EVENT_SW3     (semantic: gear-down / up-scroll / previous)
  ├── EVENT_SW4     (semantic: enter-login / digit '0' / select)
  ├── EVENT_SW5     (semantic: digit '1' / decrement)
  └── EVENT_SW6     (semantic: cycle field)
```

**Why this matters:** If hardware changes (different keypad, remapped pins),
only `translate_key()` needs updating. All application logic is insulated.

---

### `state.h/.c` — State Machine

```
States:
├── DASHBOARD      — Boot state. Live clock, gear, speed.
├── LOGIN          — 4-digit binary PIN entry.
├── MENU           — Authenticated scrollable menu.
├── VIEW_LOGS      — Browse EEPROM entries on CLCD.
├── CLEAR_LOGS     — One-shot: erase EEPROM log.
├── DOWNLOAD_LOGS  — One-shot: dump all entries via UART.
├── SET_TIME       — Field-by-field RTC editor.
└── CHANGE_PASSWORD— Dual-buffer PIN change.

Accessors:
├── set_status(STATE) → clcd_clear() + UART log + state transition
└── get_status(void)  → returns current STATE        [FIX: was get_status()]
```

Every transition emits a `[STATE] name\r\n` UART log line — the serial monitor
shows a complete audit trail of every screen change.

---

### `timer.h/.c` — Hardware Timer Driver

```
Timer0 (8-bit)
├── Clock: FOSC/4 (5 MHz)
├── Prescaler: 1:256
├── Preload: 61 (256 − 195 = 61)
├── Overflow period: 195 × 256 × 200 ns ≈ 9.98 ms
└── Increments: blink_tick (unsigned char)
    Used by: login cursor blink (BLINK_THRESHOLD = 20 → 200 ms)
             set_time field blink (ST_BLINK_THRESHOLD = 25 → 250 ms)
             entry cooldown gate

Timer1 (16-bit)
├── Clock: FOSC/4 (5 MHz)
├── Prescaler: 1:8
├── Preload: 3036 (65536 − 62500 = 3036)
├── Overflow period: 62500 × 8 × 200 ns = 100.0 ms
└── Increments: timeout_tick (unsigned int, caps at 0xFFFF)
    Used by: login idle timeout (TIMEOUT_THRESHOLD = 50 → 5 s)
             menu inactivity (MENU_INACTIVITY = 300 → 30 s)
             login lockout countdown
             set_password idle timeout (CP_TIMEOUT_TICKS = 100 → 10 s)

ISR contract:
└── Increments counters and returns. NOTHING else.
    Application logic that reads counters runs in the main loop.
```

---

### `blackbox_drivers.h/.c` — Peripheral API

```
blackbox_drivers
│
├── UART (RC6=TX, RC7=RX)
│   ├── initUART(baud)           Configure USART, set SPBRG
│   ├── uart_putchar(ch)         Blocking single-byte TX (waits for TXIF)
│   ├── uart_puts(str)           Sends null-terminated string; \n → \r\n
│   │
│   └── [REMOVED - dead code, 2026-05-09]
│       ├── uart_getchar()       Non-blocking RX (returned 0 if no data)
│       └── uart_data_ready()    Returned RCIF flag
│
├── I²C MSSP Master (RC3=SCL, RC4=SDA, 100 kHz)
│   ├── initI2C(baud)            Configure SSPADD, enable MSSP
│   ├── i2c_start()              Assert START condition
│   ├── i2c_stop()               Assert STOP condition
│   ├── i2c_repeat_start()       Assert Repeated START
│   ├── i2c_write(data)          Send byte; returns 1 if ACK received
│   ├── i2c_read(ack)            Read byte; ack=1 → NACK (last byte)
│   └── [static] i2c_wait_for_idle()  Polls R_nW + SSPCON2 busy bits
│
├── CLCD HD44780 16×2 (PORTD, RE1=EN, RE2=RS)
│   ├── init_clcd()              TRIS config + HD44780 power-up sequence
│   ├── clcd_putch(ch, addr)     Write one character at DDRAM address
│   ├── clcd_print(str, addr)    Write null-terminated string from addr
│   └── clcd_clear()             Send 0x01 command + 2 ms delay
│
├── DS1307 RTC (I²C addr 0x68)
│   ├── init_rtc()               Clear CH bit (clock-halt) in register 0x00
│   ├── ds1307_i2c_read(addr)    Read one BCD register
│   └── ds1307_i2c_write(data, addr)  Write one BCD register
│
├── Digital Keypad (PORTB, RB0–RB5, active-low)
│   ├── init_digital_keypad()    Set TRISB=0xFF (all inputs)
│   └── read_digital_keypad(mode)
│       ├── LEVEL mode: returns PORTB & 0x3F immediately
│       └── EDGE mode:  returns key only on new press (debounced, once=flag)
│
└── ADC 10-bit (AN0–AN7)
    ├── initADC()                ADCS=010 (FOSC/32), right-justified, all analog
    └── read_adc(channel)        Set CHS, assert GO, wait nDONE, return 10-bit
```

---

### `dashboard.h/.c` — Live Vehicle Display

```
dashboard_update(EVENT evt)              dashboard_render(void)
─────────────────────────────────        ─────────────────────────────────
Reads from:                              Reads from:
  DS1307 → sys.hours/minutes/seconds      sys.hours/minutes/seconds
  ADC(CH0) → sys.speed                    sys.gear
  EVENT → sys.gear (SW2=up, SW3=down)     sys.flags
  EVENT → sys.flags (SW1=crash)           sys.speed
  EVENT → sys.log_pending                Cache: prev_speed, prev_gear,
                                          prev_flags
Writes to:                               Writes to:
  sys.*                                   CLCD (via clcd_print/clcd_putch)
  uart (telemetry on change only)
                                         Optimisation: redraws only when
Crash behaviour:                         cached values differ from sys.
  SW1 sets FLAG_CRASH | FLAG_IGNITION.   Time always redrawn (cheap).
  Subsequent SW2/SW3 ignored.
  log_pending = 1 on any gear/crash.

CLCD layout:
  LINE1: "TIME      EV   SD"
  LINE2: "HH:MM:SS  GN   072"
         └──────┘  └─┘  └─┘
           RTC     gear speed%
```

---

### `login.h/.c` — PIN Authentication

```
LOGIN_PHASE state machine:
│
├── PHASE_ENTERING (initial)
│   ├── Timer: 5 s idle timeout → DASHBOARD
│   ├── SW4 digit '0', SW5 digit '1'
│   ├── After 4 digits → pass_match()
│   │   ├── match    → PHASE_SUCCESS
│   │   └── mismatch → PHASE_FAIL (fail_count++)
│   └── SW1 → DASHBOARD (cancel)
│
├── PHASE_FAIL (one-shot render)
│   ├── Display "Invalid Entry" + tries remaining
│   ├── If fail_count >= MAX_FAILS → PHASE_LOCKED
│   └── Else → PHASE_LOCKOUT (timer starts)
│
├── PHASE_LOCKOUT (timed wait)
│   ├── Lockout durations: fail 1→0s, 2→15s, 3→30s, 4→60s
│   ├── Countdown displayed on LCD (seconds remaining)
│   └── Expires → PHASE_ENTERING
│
├── PHASE_SUCCESS (one-shot render)
│   ├── Display "Access Granted"
│   ├── logged_in_flag = 1
│   ├── fail_count = 0
│   └── → MENU
│
└── PHASE_LOCKED (permanent)
    ├── Display "Device Locked / Contact Admin"
    └── Only a power cycle resets fail_count (by design)

Password storage:
├── EEPROM @ 0xF0–0xF3 (4 ASCII bytes, '0' or '1')
├── Loaded at boot via login_load_password()
├── Default "1111" written if EEPROM reads all 0xFF
└── Updated live by login_set_password() after a change

Timing:
├── Cursor blink: 20 × 10 ms = 200 ms (Timer0)
└── Idle timeout: 50 × 100 ms = 5 s (Timer1)
```

---

### `menu.h/.c` — Authenticated Menu

```
Menu items (selection index 0–5):
  0: VIEW LOG        → VIEW_LOGS state
  1: CLEAR LOG       → CLEAR_LOGS state (one-shot)
  2: DOWNLOAD LOG    → DOWNLOAD_LOGS state (one-shot)
  3: SET TIME        → SET_TIME state
  4: CHANGE PASSWORD → CHANGE_PASSWORD state
  5: LOGOUT          → do_logout() → DASHBOARD

Controls:
  SW2: scroll down (selection++)
  SW3: scroll up   (selection--)
  SW4: confirm      set_status(destination) or do_logout()
  SW1: back         set_status(DASHBOARD)

CLCD layout (2-row preview):
  LINE1: "> [selected item]   "
  LINE2: "  [next item]       " (blank if at last item)

Inactivity: 300 × 100 ms = 30 s → auto-logout → DASHBOARD
Redraw: only when selection != last_drawn (sentinel: 0xFF)
```

---

### `view_logs.h/.c` — Log Viewer and Downloader

```
view_logs_update(EVENT)            view_logs_render(void)
─────────────────────────────      ─────────────────────────────
SW2: log_index++, needs_draw=1     if (!needs_draw) return  ← cache
SW3: log_index--, needs_draw=1     eeprom_read_log(index, buf)
SW1: → MENU                        gear_label(buf[7]) → gl
                                   Draw on CLCD:
                                   LINE1: "LOG xx/xx      "
                                   LINE2: "HH:MM:SS GL SSS"

gear_label() decode (mandatory at every read site):
  '0' → "GR"  (Reverse)
  '1' → "GN"  (Neutral)
  '2' → "G1"  (First)
  '3' → "G2"  (Second)
  '4' → "G3"  (Third)
  '5' → "G4"  (Fourth)
  'C' → "CR"  (Crash event)

download_logs() — one-shot UART dump:
  Sends all entries to serial monitor in tabular format:
  "=== CAR BLACK BOX LOG ===\n"
  "## HH:MM:SS GEAR SPD\n"
  "01 09:41:03 GN  072\n"
  ...
  Auto-transitions to MENU after dump.
```

---

### `eeprom.h/.c` — Circular Log Driver

```
Memory layout (AT24C04, 512 bytes):
  Addr 0x00     head  (next write slot, 0–9)
  Addr 0x01     count (valid entries, 0–10)
  Addr 0x02–0x72  10 slots × 11 bytes = 110 bytes log data
  Addr 0xF0–0xF3  4-byte PIN (ASCII, independent of log ring)

Circular buffer rules:
  Write: slot[head] ← record; head = (head+1)%10; count = min(count+1, 10)
  Read:  oldest = (head - count + 10) % 10; slot[i] = (oldest + i) % 10
  When full (count=10): head advances, silently overwrites oldest — no special case

Write path (atomic page-write):
  i2c_start()
  i2c_write(dev_addr | page_select)
  i2c_write(byte_addr)
  i2c_write(rec[0]) … i2c_write(rec[10])   ← 11 bytes in one transaction
  i2c_stop()
  ack_poll() (max 50 retries × 100 µs = 5 ms)
  → Either all 11 bytes commit or the slot is unchanged

Stack depth for eeprom_write_log():
  main(1) → eeprom_write_log(2) → eeprom_write_byte(3)
  → i2c_start(4) → i2c_wait_for_idle(5). ISR: 6. Safe (limit: 8)
```

---

## 📊 CLCD Display Reference

### DASHBOARD screen

```
┌────────────────┐
│ TIME  EV   SD  │  ← static labels (redrawn every frame — cheap)
│ 09:41:03 GN 72 │  ← live: BCD-decoded time | gear/crash | speed %
└────────────────┘
Gear field values: GR GN G1 G2 G3 G4  or  C  (crash)
Speed field:       000–100 (ADC mapped: 0–1023 → 0–100)
```

### LOGIN screen

```
┌────────────────┐
│ PassWord:      │
│ ** _           │  ← * = confirmed digit, _ = blinking cursor
└────────────────┘
Lockout:
│ Locked!        │
│ Retry in: 030s │
Permanent lock:
│ Device Locked  │
│ Contact Admin  │
```

### MENU screen

```
┌────────────────┐
│ > VIEW LOG     │  ← selected item (with > marker)
│   CLEAR LOG    │  ← next item preview
└────────────────┘
```

### VIEW LOGS screen

```
┌────────────────┐
│ LOG 03/10      │  ← entry 3 of 10
│ 09:43:19 CR 055│  ← time | gear ("CR"=crash) | speed
└────────────────┘
```

### SET TIME screen

```
┌────────────────┐
│ Set Time SW2:OK│
│ [09]:41:03  SW1:Back │  ← active field blinks (250 ms), others solid
└────────────────┘
```

---

## 🔌 Serial Monitor Reference

Connect a USB-UART bridge to **RC6 (TX) / RC7 (RX)**. Open terminal at **9600 8N1**.

| Prefix | Event |
|--------|-------|
| `=== CAR BLACK BOX STARTED ===` | Power-on / reset |
| `[STATE] DASHBOARD` | State machine transitioned |
| `[DASH] GEAR: G2` | Gear changed |
| `[DASH] CRASH` | SW1 pressed — crash latched |
| `[DASH] IGNITION ON` | First key event after boot |
| `[LOGIN] No saved pass — writing default` | Fresh EEPROM |
| `[LOGIN] Password loaded from EEPROM` | Normal boot |
| `[LOGIN] digit 2` | Second digit entered |
| `[LOGIN] CORRECT` | PIN matched |
| `[LOGIN] FAIL 2/5` | Second wrong attempt |
| `[LOGIN] TIMEOUT` | 5 s idle → DASHBOARD |
| `[LOGIN] ACCESS GRANTED` | Login succeeded |
| `[LOGIN] LOCKOUT EXPIRED` | Cooldown elapsed |
| `[LOGIN] LOGGED OUT` | do_logout() called |
| `[MENU] SELECT: VIEW LOG` | Menu item activated |
| `[MENU] BACK` | SW1 pressed in menu |
| `[MENU] INACTIVITY — auto-logout` | 30 s idle |
| `[LOGS] NEXT` | SW2 in view_logs |
| `[LOGS] PREV` | SW3 in view_logs |
| `[LOGS] BACK` | SW1 in view_logs |
| `[LOGS] empty` | No log entries exist |
| `[LOGS] CLEARED` | clear_logs() ran |
| `[DOWNLOAD] No logs stored.` | Empty EEPROM log |
| `[LOG] EEPROM OK` | Entry written successfully |
| `[LOG] EEPROM FAIL` | I²C write error |
| `[SET_TIME] SAVED to DS1307` | SW2 confirmed time |
| `[SET_TIME] DISCARDED — RTC unchanged` | SW1 cancelled |
| `[CP] MATCH — writing to EEPROM` | New PIN matches confirm |
| `[CP] MISMATCH — resetting` | PIN mismatch |
| `[CP] EEPROM write complete` | New PIN saved |
| `[CP] CANCELLED` | SW1 in set_password |
| `[CP] TIMEOUT — returning to MENU` | 10 s idle |

### UART Download Format

```
=== CAR BLACK BOX LOG ===
## HH:MM:SS GEAR SPD
01 09:41:03 GN  072
02 09:43:17 G2  055
03 09:43:19 CR  055
=========================
```

---

## 🛠️ Getting Started

### Prerequisites

| Tool | Version |
|------|---------|
| MPLAB X IDE | ≥ 6.x |
| XC8 Compiler | ≥ 2.x |
| Programmer | PICkit 3/4 (or PICSIMSLAB for simulation) |
| Serial terminal | Any 9600 8N1 (PuTTY, Tera Term, CoolTerm) |

### Build Steps

1. Clone the repository.
2. Open MPLAB X → **File → Open Project**.
3. Select the `Car_Black_Box` folder.
4. **Copy `user.cmake`** into both CMake output directories:
   - `cmake/Car_Black_Box/default.production/`
   - `cmake/Car_Black_Box/default/`
5. Clean and build — ROM usage should now be well within 8 KB.
6. Program via PICkit or load into PICSIMSLAB.

### Alternative: Setting Optimisation in MPLAB X GUI

If your project is not CMake-based:
- Project Properties → **XC8 Global Options** → **XC8 Compiler**
- → **Optimizations** → **Optimization Set** → `"s"` (size) or `"1"`

---

## ⚠️ Known Limitations

| Limitation | Notes |
|------------|-------|
| No date field | DS1307 date registers (0x03–0x06) not read |
| Crash flag volatile | `sys.flags` cleared on power cycle; not EEPROM-persisted |
| `set_time_reset()` not called | SET_TIME starts at "00:00:00" on entry; see LOG.md for one-line fix |
| Single ADC channel | Only AN0 (speed pot); AN1–AN7 unused |
| Binary PIN only | Password digits are `'0'` or `'1'`; 4-digit = 16 possible combos |
| 10-entry log cap | Oldest overwritten silently; no full-log alert |
| No UART RX in production | `uart_getchar` / `uart_data_ready` removed as dead code |

---

## 📐 PIC16F877A Resource Budget

| Resource | Total | Used (est. -Os) | Free |
|----------|-------|-----------------|------|
| Program Memory | 8192 words | ~5800–6200 | ~2000–2400 |
| Data Memory (RAM) | 368 bytes | ~200–250 | ~120–170 |
| EEPROM (internal) | 256 bytes | 0 (not used) | 256 |
| AT24C04 (external) | 512 bytes | 116 (log+PIN) | 396 |
| Hardware call stack | 8 levels | 6 (worst case) | 2 |
| Interrupt levels | 1 (no priority) | 1 (Timer0+1 ISR) | 0 |

---

## 📚 What I Learned

### Embedded Architecture
1. **4-phase event loop** — READ → UPDATE → RENDER → STORAGE eliminates hidden side effects and makes the data flow explicit.
2. **Update/Render separation** — the single biggest architectural improvement. Render reflects state; it never creates it.
3. **One source of truth** — `SYSTEM_STATE sys` eliminates private copies of vehicle data scattered across modules.
4. **Event abstraction** — `translate_key()` insulates application logic from hardware pin assignments.
5. **ISR discipline** — ISRs that only increment counters produce no race conditions and are trivially testable.

### PIC16F877A Constraints
6. **Hardware stack limit** — 8 levels. Every function call is a cost. Audit the deepest chain and leave headroom for ISR.
7. **ROM vs RAM trade-off** — `const char * const` tables live in Flash. Mutable string buffers burn scarce RAM.
8. **`-Os` not `-O0`** — `-O0` exists only for step-through debugging. `-Os` is the correct default for release.
9. **`__delay_ms()` not empty loops** — empty loops are optimised away. `__delay_ms()` uses `_XTAL_FREQ` and survives.
10. **Unsigned indices always** — signed `char` as an array index wraps negative below zero; use `unsigned char`.

### C Language Pitfalls
11. **`(void)` vs `()`** — In C90, `foo()` is "unknown parameters". `foo(void)` is "no parameters". Always write `(void)`.
12. **`static` never in headers** — declaring `static` functions in headers gives every TU its own copy; ROM bloat.
13. **Namespace with prefixes** — C has no namespaces. `uart_putchar`, not `putchar`. Module prefixes prevent stdlib collisions.
14. **Sentinel values are not indices** — `0xFF` as "unset" must never be dereferenced as an array index.
15. **Non-blocking drivers only** — `while (!RCIF);` in a driver is a latent denial-of-service in an event loop.

### I²C and EEPROM
16. **ACK/NACK on last byte** — single-byte read → NACK (1). Sending ACK tells the slave to send another byte.
17. **Page-write atomicity** — 11 sequential byte-writes are not atomic. One I²C page-write transaction is.
18. **ACK polling after write** — EEPROM needs up to 5 ms internal write cycle; poll until ACK before next access.

---

## 📄 License

MIT License — see [LICENSE](LICENSE) for details.