# Development Log

> A running record of bugs encountered, lessons learned, and design decisions made during the Car Black Box project.

---

## Format

Each entry follows: **[DATE] — Title** → Root cause → Fix applied → Lesson.

---

## Bug Log

---

### [2026-05-07] — `FOSC` Defined in Multiple Headers

**Files:** `uart.h`, `i2c.h`

**Problem:** `#define FOSC 20000000` appeared in both `uart.h` and `i2c.h`. When both headers are included in the same translation unit, the preprocessor issues a macro-redefinition warning, which stricter build settings treat as an error.

**Fix:** Removed `FOSC` from both peripheral headers. Centralised it in `main_config.h` as the single source of truth.

**Lesson:** Global hardware constants (`FOSC`, `_XTAL_FREQ`) belong in one top-level config header, not scattered across peripheral drivers.

---

### [2026-05-07] — `getchar`, `putchar`, `puts` Conflict with C Standard Library

**File:** `uart.h`, `uart.c`

**Problem:** The UART driver defined functions named `getchar`, `putchar`, and `puts` — names reserved by the C standard library. XC8 may pull in the standard definitions internally, causing duplicate-symbol linker errors or silent aliasing.

**Fix:** Renamed all three to `uart_getchar`, `uart_putchar`, and `uart_puts`. Updated all call-sites accordingly.

**Lesson:** Never shadow standard library identifiers. Prefix driver functions with the module name (`uart_`, `i2c_`, etc.).

---

### [2026-05-07] — `static` Function Declared in `i2c.h`

**File:** `i2c.h`

**Problem:** `static void i2c_wait_for_idle()` was declared in the public header. Every `.c` file that includes the header gets its own private copy, producing multiple definitions and dead-code bloat.

**Fix:** Removed the declaration from `i2c.h`. The function is now `static` only inside `i2c.c`.

**Lesson:** `static` functions are implementation details and must never appear in public headers.

---

### [2026-05-07] — Spurious ADC Conversion Started Inside `initADC()`

**File:** `adc.c`

**Problem:** `GO = 1` was placed inside `initADC()`, before `ADON = 1`. This started a conversion while the ADC was still powered down, producing an undefined initial reading.

**Fix:** Removed `GO = 1` from `initADC()`. Conversions are now only started inside `read_adc()`.

**Lesson:** Initialisation functions must only configure — never trigger operations.

---

### [2026-05-07] — Wrong ACK/NACK Sent on DS1307 Single-Byte Read

**File:** `ds1307.c`

**Problem:** `ds1307_i2c_read()` called `i2c_read(0)`, sending an ACK after the only received byte. The I²C spec requires a NACK after the last (or only) byte. The comment in the code explicitly said "Master MUST send NACK (1)" — a clear copy-paste error.

**Fix:** Changed `i2c_read(0)` → `i2c_read(1)`.

**Lesson:** A comment that contradicts the code is a red flag — fix the code, not the comment.

---

### [2026-05-07] — Unreliable Busy-Wait Delay in `clcd_clear()`

**File:** `clcd.c`

**Problem:** `for(unsigned char i = 0; i < 50; i++);` was used as a post-clear delay. At `-O2` the loop is eliminated entirely, leaving no delay before the next HD44780 command (requires ≥ 1.52 ms).

**Fix:** Replaced with `__delay_ms(2)`.

**Lesson:** Never use empty loops for timing. Always use `__delay_us()` / `__delay_ms()` or a hardware timer.

---

### [2026-05-07] — Unused Variable `delay` in `main()`

**File:** `main.c`

**Problem:** `unsigned char delay = 0;` was declared but never used, wasting a RAM byte and generating a compiler warning.

**Fix:** Removed the declaration.

**Lesson:** Remove dead code before committing. Enable `-Wall` and treat warnings as errors.

---

### [2026-05-07] — `display_event()` Gear Index Typed as `signed char`

**File:** `dashboard.c`

**Problem:** `static char i = 1` was used as an array index. XC8 defaults `char` to signed, so decrementing below 0 yields -1 — a valid negative subscript causing an out-of-bounds read.

**Fix:** Changed to `static unsigned char i = 1`.

**Lesson:** Array indices must always be unsigned. Use `uint8_t` / `unsigned char` consistently.

---

### [2026-05-07] — Global State Trap (The Ghost Wipe)

**File:** `main.c`

**Problem:** An unconditional `if(key == SW4)` block at the top of the `while(1)` loop acted as a global override. Every `SW4` press triggered `set_status(LOGIN)`, which called `clcd_clear()` — wiping the display mid-keystroke even when the user was already on the Login screen trying to type a `'0'`.

**Fix:** Removed the global trap. The `set_status(LOGIN)` transition now lives strictly inside `case DASHBOARD:`. `case LOGIN:` can safely interpret the same physical button as a localised `'0'` without triggering a state reset.

**Lesson:** State-transition logic belongs inside its originating state case, never at global scope. A button's meaning is always context-dependent.

---

## Session Summary — 2026-05-07

> Tonight's session restructured the Car Black Box firmware from a fragile procedural script into a hardened, non-blocking state machine capable of running at 5 MIPS without race conditions or hardware lockups.

### Core Architectural Upgrades

**1. Input Decoupling — Single-Read Router Pattern**
Hardware is polled exactly once per cycle in `main.c`. The result is passed contextually down the call stack to the active module, eliminating the race condition where `dashboard.c` consumed the `SW4` edge before the global router could act on it.

**2. State Isolation — Zero Input Bleed**
Inputs are handled entirely inside their contextual `switch` cases. The input buffer is explicitly zeroed (`key = ALL_RELEASED`) on state transition, preventing the physical hold of `SW4` from ghosting into the newly initialised Login screen.

**3. Canvas Locking & Cache Invalidation**
The `screen_initialized` flag ensures static UI elements are drawn exactly once per state entry, eliminating the 70 ms per-cycle CPU freeze caused by unconditionally re-printing text over the HD44780 bus. `invalidate_dashboard_cache()` forces dynamic elements to redraw only when re-entering the Dashboard from another state.

**4. Non-Blocking Execution**
Recursive `verify_password()` calls and infinite `while(1);` traps were removed. All modules update flags and return control to the master loop immediately. Loop timers upgraded from 8-bit (`unsigned char`, max 255) and 16-bit (`unsigned int`, max 65 535) to 32-bit `unsigned long` to accurately track human-scale time without overflowing.

**5. Strict Hardware Debouncing**
Compiler-dependent `for` loop debounce (< 0.75 ms) replaced with `__delay_ms(20)` blocks that reliably swallow both the initial push bounce and the subsequent release bounce of physical switch contacts.

### What's Next

- Replace `wait++` software loop timers with **Timer0 / Timer1 hardware interrupts** (ISRs) for accurate, CPU-free time tracking.
- Implement **EEPROM logging** so crash events survive power cycles.
- Build out the **MENU, VIEW\_LOGS, SET\_TIME, and CHANGE\_PASSWORD** state handlers now that the state machine skeleton is stable.

---

## Design Notes

### Why `uart_` prefix instead of `UART_`?
Function names use `lower_snake_case` with a module prefix. Macros and constants use `UPPER_SNAKE_CASE`. This is consistent with MISRA-C naming guidelines and makes the origin of every identifier immediately clear.

### Why centralise `FOSC` in `main_config.h`?
The oscillator frequency is a board-level property. Placing it in peripheral headers couples unrelated drivers to a hardware detail they should not know about.

### DS1307 ACK/NACK Rule
On single-byte I²C reads, always NACK. On multi-byte reads, ACK all bytes except the last. This is mandatory per the I²C specification and DS1307 datasheet §5.0.