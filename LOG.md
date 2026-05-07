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

**Problem:** `#define FOSC 20000000` appeared in both `uart.h` and `i2c.h`. When both headers are included in the same translation unit (e.g. `main.c` via `main_config.h`), the preprocessor issues a macro-redefinition warning. On stricter build settings this can be treated as an error.

**Fix:** Removed `FOSC` from both peripheral headers. Centralised it in `main_config.h` as the single source of truth. All peripheral modules that need it include `main_config.h` indirectly.

**Lesson:** Global hardware constants (`FOSC`, `_XTAL_FREQ`) belong in one top-level config header, not scattered across peripheral drivers.

---

### [2026-05-07] — `getchar`, `putchar`, `puts` Conflict with C Standard Library

**File:** `uart.h`, `uart.c`

**Problem:** The UART driver defined functions named `getchar`, `putchar`, and `puts` — names that are reserved by the C standard library (`<stdio.h>`). Even without an explicit `#include <stdio.h>`, XC8 may pull in the standard definitions internally, causing duplicate-symbol linker errors or silent aliasing to the wrong implementation.

**Fix:** Renamed all three to `uart_getchar`, `uart_putchar`, and `uart_puts`. Updated all call-sites in `uart.c` and `dashboard.c` accordingly.

**Lesson:** Never shadow standard library identifiers. Prefix driver-level functions with the module name (`uart_`, `i2c_`, etc.) to create an unambiguous namespace.

---

### [2026-05-07] — `static` Function Declared in `i2c.h`

**File:** `i2c.h`

**Problem:** `static void i2c_wait_for_idle()` was declared in the public header. A `static` function in a header means every `.c` file that includes the header gets its own private copy of the function — producing multiple definitions and dead-code bloat. More importantly, it defeats the purpose of the `static` keyword (internal linkage).

**Fix:** Removed the declaration from `i2c.h`. The function is now `static` only inside `i2c.c`, making it a true translation-unit-private helper.

**Lesson:** `static` functions are implementation details. They must never appear in public headers.

---

### [2026-05-07] — Spurious ADC Conversion Started Inside `initADC()`

**File:** `adc.c`

**Problem:** `GO = 1` was placed inside `initADC()`, *before* `ADON = 1`. This started an A/D conversion while the ADC module was still powered down, resulting in an undefined initial reading. The intent was clearly to configure the module, not to sample.

**Fix:** Removed the `GO = 1` line from `initADC()`. Conversions are now only started inside `read_adc()`, which is the correct and only place.

**Lesson:** Initialisation functions must only configure; they must not trigger operations. Read the datasheet section on ADC startup sequencing (acquisition time requirements).

---

### [2026-05-07] — Wrong ACK/NACK Sent on DS1307 Single-Byte Read

**File:** `ds1307.c`

**Problem:** The `ds1307_i2c_read()` function called `i2c_read(0)`, which sends an **ACK** after receiving the byte. According to the I²C specification, a master must send a **NACK** after the *last* (or only) byte it wishes to receive, to signal to the slave that no further bytes are required. Sending ACK causes the DS1307 to attempt to clock out another byte, stretching the bus and potentially garbling subsequent transactions.

The comment in the code even said `"Master MUST send NACK (1)"`, making this a clear copy-paste error.

**Fix:** Changed `i2c_read(0)` → `i2c_read(1)` so that NACK is correctly transmitted after the single byte.

**Lesson:** Always verify that the argument you pass matches the documented contract of the callee. A comment contradicting the code is a red flag — fix the code, not the comment.

---

### [2026-05-07] — Unreliable Busy-Wait Delay in `clcd_clear()`

**File:** `clcd.c`

**Problem:** `for(unsigned char i = 0; i < 50; i++);` was used as a post-clear delay. The iteration count is entirely dependent on compiler optimisation level: at `-O2` the loop may be eliminated entirely, leaving no delay and causing LCD commands to be sent before the controller has finished clearing (HD44780 requires ≥ 1.52 ms after a Clear Display command).

**Fix:** Replaced with `__delay_ms(2)` — a compiler-intrinsic delay that is independent of optimisation level and precisely matches the HD44780 datasheet requirement.

**Lesson:** Never use empty loops for timing in embedded C. Always use `__delay_us()` / `__delay_ms()` (XC8) or a hardware timer.

---

### [2026-05-07] — Unused Variable `delay` in `main()`

**File:** `main.c`

**Problem:** `unsigned char delay = 0;` was declared inside `main()` but never read or written after initialisation. This generates a compiler warning and wastes a RAM byte.

**Fix:** Removed the declaration.

**Lesson:** Remove dead code before committing. Enable `-Wall` and treat warnings as errors during development.

---

### [2026-05-07] — `display_event()` Gear Index Typed as `signed char`

**File:** `dashboard.c`

**Problem:** `static char i = 1` was used as the array index into the gear-state string array. Because `char` is *signed* on XC8 by default, decrementing `i` below 0 would produce -1, which is a valid (negative) array subscript — causing an out-of-bounds memory read.

**Fix:** Changed to `static unsigned char i = 1`. The lower-bound guard `if(i > 0) i--;` now operates correctly on an unsigned type.

**Lesson:** Array indices must always be unsigned. Use `uint8_t` / `unsigned char` consistently for index variables.

---

## Design Notes

### Why `uart_` prefix instead of `UART_`?
Function names use `lower_snake_case` with a module prefix (`uart_`, `i2c_`, `ds1307_`). Macros and constants use `UPPER_SNAKE_CASE`. This is consistent with the MISRA-C naming guidelines and makes the origin of every identifier immediately clear.

### Why centralise `FOSC` in `main_config.h`?
The oscillator frequency is a board-level property, not a peripheral property. Placing it in peripheral headers couples unrelated drivers to a hardware detail they shouldn't care about. A peripheral should accept frequency as a parameter or read it from a shared config.

### DS1307 ACK/NACK Rule
On single-byte I²C reads, always NACK. On multi-byte reads, ACK all bytes except the last. This is mandatory per the I²C specification and the DS1307 datasheet (§5.0 — Serial Data Transfer).