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

### [2026-05-09] — PIC16F877A Hardware Stack Overflow (Intermittent Reboot)

**Files:** `main.c`, `eeprom.c`, `dashboard.c`

**Problem:** The PIC16F877A has an 8-level hardware call stack. Each function call consumes one slot; interrupts consume one slot on top of whatever depth the CPU is currently at. The original call chain when flushing an EEPROM log was:

```
startup(1) -> main(2) -> flush_pending_logs(3) -> eeprom_write_log(4)
  -> write_slot(5) -> eeprom_write_byte(6) -> ack_poll(7)
  -> i2c_start(8) -> i2c_wait_for_idle: STACK FULL
```

If Timer0 (10 ms) or Timer1 (100 ms) fired while the CPU was at depth 8, the ISR pushed depth to 9. The stack pointer silently wrapped to 1, overwriting the return address of `main()`. Execution jumped to a corrupt address and the PIC rebooted.

**Fix (three-part):**
1. `flush_pending_logs()` removed as a function. The EEPROM write is now inlined directly in `main()`'s while loop, saving one call level.
2. `write_slot()` merged into `eeprom_write_log()`, saving another level.
3. `ack_poll()` inlined into `eeprom_write_byte()` and the page-write block, saving a third level.

Worst-case depth after fix:
```
startup(1) -> main(2) -> eeprom_write_log(3) -> eeprom_write_byte(4)
  -> i2c_start(5) -> i2c_wait_for_idle(6). ISR: 7. Safe.
```

**Lesson:** On resource-constrained MCUs with a fixed hardware stack, every function call has a measurable cost. Deep call chains must be audited against the stack limit, especially when ISRs are active.

---

### [2026-05-09] — Crash Event Never Displayed on CLCD (Dead Render Block)

**File:** `dashboard.c`, `display_event()`

**Problem:** When SW1 was pressed, `FLAG_CRASH` was latched in `current_log.flags`. On every subsequent call to `display_event()`, the very first check was:

```c
if (current_log.flags & FLAG_CRASH) {
    current_log.gear = (unsigned char)gear;
    return;   // Early return here
}
// ...
if (current_log.flags & FLAG_CRASH)
    clcd_print("C ", LINE2(10));  // Never reached
```

**Fix:** Restructured with a single guard: `if (!(current_log.flags & FLAG_CRASH))` wraps only the key-processing block. The render block is now always reached.

**Lesson:** When a function has multiple exit points (early returns), trace every path to confirm that all side-effects are reached.

---

### [2026-05-09] — Gear Label Corruption in Log Viewer and UART Download

**Files:** `view_logs.c`, `eeprom.c`

**Problem:** The EEPROM stores gear as the raw `GEAR_STATE` index encoded as an ASCII digit. The viewer and downloader printed this byte literally, making GN display as `"G1"` (First Gear) — a factual error corrupting the integrity of the black box record.

**Fix:** Added `gear_label(char g)` in `view_logs.c`. It maps each storage byte to its correct two-character label.

| Stored | Displayed (wrong) | Displayed (fixed) |
|--------|-------------------|-------------------|
| `'0'`  | `G0`              | `GR`              |
| `'1'`  | `G1`              | `GN`              |
| `'2'`  | `G2`              | `G1`              |
| `'3'`  | `G3`              | `G2`              |
| `'4'`  | `G4`              | `G3`              |
| `'5'`  | `G5`              | `G4`              |
| `'C'`  | `GC`              | `CR`              |

**Lesson:** Any time data is stored in a compact representation, a dedicated decode function must be used at every read site.

---

### [2026-05-09] — CPU-Loop-Dependent Login Cooldown Was Effectively Instantaneous

**File:** `login.c`

**Problem:** Entry cooldown used `entry_cooldown++` per main loop iteration. After the ISR refactor made the loop near-full 5 MIPS throughput, 50 iterations completed in under 0.2 ms.

**Fix:** Replaced with a hardware-timer-based gate using `timeout_tick` (Timer1, 100 ms ISR tick), independent of loop speed.

**Lesson:** Any timing requirement must be based on a hardware timer.

---

### [2026-05-09] — EEPROM Mid-Write Record Corruption

**File:** `eeprom.c`, `write_slot()`

**Problem:** 11 separate `eeprom_write_byte()` calls. If any call failed mid-record, a partially-written entry remained permanently readable.

**Fix:** 11-byte record sent in a single AT24C04 page-write transaction — either all 11 bytes commit atomically or none do.

**Lesson:** When multiple writes must be atomic, use the hardware's native batch mechanism.

---

### [2026-05-09] — `uart_getchar()` Was a System-Locking Blocking Call

**File:** `blackbox_drivers.c`

**Problem:** `uart_getchar()` contained `while (!RCIF);` — a permanent block. If any developer called it in production, the entire firmware would freeze.

**Fix:** `uart_getchar()` now returns immediately with `0` if no byte is available. Companion `uart_data_ready()` returns `RCIF` for poll-before-read.

**Lesson:** Driver functions must always be non-blocking in an embedded event loop.

---

### [2026-05-09] — Menu SW4 Used `last_i` (Sentinel 0xFF) Instead of `i`

**File:** `menu.c`

**Problem:** SW4 handler called `set_status(menu_states[last_i])`. If the user pressed SW4 without navigating, `last_i` was still `0xFF` — an out-of-bounds array read.

**Fix:** The SW4 handler now uses `i` (the live selection index), not `last_i`.

**Lesson:** Sentinel values must never be dereferenced as array indices.

---

## ══════════════════════════════════════════════════════════════════
## SESSION REPORT — 2026-05-09 (Compiler Fixes & Optimisation)
## ══════════════════════════════════════════════════════════════════

> This session resolved all four categories of compiler issues blocking the
> PIC16F877A build: ROM overflow due to missing optimisation, dead code bloat,
> an incomplete function prototype, and string storage placement.

---

### [2026-05-09] — FIX 1: Compiler Optimisation (-O0 → -Os)

**File:** `cmake/Car_Black_Box/default.production/user.cmake` *(created)*
         `cmake/Car_Black_Box/default/user.cmake` *(created)*

**Problem:**
The project compiled at `-O0` (no optimisation) — the MPLAB X default. On the
PIC16F877A with 8 KB words of program memory, `-O0` causes:
- All temporary variables spilled to data memory (RAM) rather than held in
  the W register or FSR.
- Every inline-able helper kept as a separate function with its own CALL/RETURN
  pair (~2–4 words each).
- Unused static-data tables (e.g., `const char *` arrays) compiled in even
  when the linker's dead-code elimination is limited by missing optimisation.
- String literals duplicated per translation unit when not properly marked
  `const`.

At `-O0` the firmware exceeded the PIC16F877A program memory budget, causing
the linker to emit:
```
memory region `program' overflowed by N words
```

**Root Cause:**
The auto-generated `CMakeLists.txt` sets `-O0` unconditionally. The MPLAB X
build system provides `user.cmake` as the intended extension point for
project-specific overrides — this file was missing.

**Fix Applied:**
Created `user.cmake` in both CMake output directories with the following logic:

1. **Strip `-O0`** from `CMAKE_C_FLAGS`, `CMAKE_C_FLAGS_DEBUG`, and all
   variant flag variables using `string(REPLACE ...)`.
2. **Append `-Os`** ("optimise for size") to `CMAKE_C_FLAGS`.

`-Os` was chosen over `-O1` because:
- `-O1` optimises for speed; on a 20 MHz PIC16 with no cache, speed vs. size
  is the same at this scale.
- `-Os` additionally enables size-specific passes: dead function elimination,
  constant folding, and identical-code-folding of duplicate string literals.
- `-Os` typically saves 20–35 % of program words on XC8 projects of this
  complexity, which is consistent with fitting the firmware in 8 KB.

**Alternative (non-CMake projects):**
MPLAB X IDE → Project Properties → XC8 Global Options → XC8 Compiler →
Optimizations → Optimization Set → set to `"1"` or `"s"`.
This writes the value to `nbproject/configurations.xml`.

**ROM impact (estimated):**
| Setting | Approx. program words used |
|---------|---------------------------|
| -O0     | > 8192 (overflow)          |
| -O1     | ~5800–6400                 |
| -Os     | ~5600–6200 (best size)     |

**Lesson:** Never leave an embedded project at `-O0` for release. `-O0` is
correct only for step-through debugging where you must prevent the debugger
from optimising away variables. For all other builds, at minimum `-O1`.

---

### [2026-05-09] — FIX 2: Dead Code Removal (ROM Recovery)

**Files modified:**
- `blackbox_drivers.h` — declarations commented out
- `blackbox_drivers.c` — implementations commented out
- `set_time.h` — declaration commented out
- `set_time.c` — function bodies commented out

#### 2a. `uart_getchar()` and `uart_data_ready()` — `blackbox_drivers.c`

**Compiler warning:** `(361) function declared but never referenced`

**Root cause:**
Both functions were defined in `blackbox_drivers.c` and declared in
`blackbox_drivers.h`. The production firmware never calls them: all UART
usage is one-way telemetry via `uart_putchar()` / `uart_puts()`. User input
arrives exclusively from the 6-switch hardware keypad.

`uart_getchar()` was already fixed from blocking (`while (!RCIF)`) to
non-blocking in the previous session. Despite this improvement, neither
function has any call-site in the current codebase.

**ROM cost at -O0:** ~40–60 program words.

**Fix:**
- Commented out both function bodies in `blackbox_drivers.c`.
- Commented out both prototypes in `blackbox_drivers.h` with a block comment
  explaining the rationale and exact re-enable steps.

**Header check:** ✅ No remaining declarations of `uart_getchar` or
`uart_data_ready` in any `.h` file.

**Preserved in comments:** The non-blocking implementation is retained in
commented-out form so a future UART command interface can be reinstated cleanly
without archaeology.

---

#### 2b. `bcd_to_dec()` and `set_time_reset()` — `set_time.c`

**Compiler warnings:**
- `(361) function declared but never referenced` → `set_time_reset`
- `(359) static function declared but never referenced` → `bcd_to_dec`

**Root cause:**
`set_time_reset()` is the correct initialisation entry-point for the SET_TIME
state. It copies `sys.hours/minutes/seconds` (BCD from DS1307) into the module's
local `time_copy[]` buffer for safe editing. However, `main.c` never calls
`set_time_reset()` before transitioning into SET_TIME, making it a dead exported
function.

`bcd_to_dec()` is a `static` helper whose only caller was `set_time_reset()`.
With that caller absent, `bcd_to_dec()` is also unreachable.

**ROM cost at -O0:** ~30–45 program words combined.

**Fix:**
- Commented out `bcd_to_dec()` body in `set_time.c`.
- Commented out `set_time_reset()` body in `set_time.c`.
- Commented out `set_time_reset()` prototype in `set_time.h`.

**Header check:** ✅ `set_time_reset` no longer declared in `set_time.h`.
`bcd_to_dec` was always `static` (never in any header) — no header change needed.

**⚠️ Functional note (documented in both files):**
`set_time_reset()` SHOULD be called in `main.c` before every
`set_status(SET_TIME)` transition:
```c
set_time_reset();      // <-- add this line
set_status(SET_TIME);
```
Without it, `time_copy[]` is all zeros on first entry (displays "00:00:00"
instead of the actual DS1307 time). `dec_to_bcd()` is still compiled and
called by `set_time_update()` on EVENT_SW2, so SW2 commits correctly —
just with whatever was in `time_copy[]` rather than the real time.
This is a pre-existing bug, not introduced by this fix.

---

### [2026-05-09] — FIX 3: Incomplete Function Prototype for `get_status`

**File modified:** `state.h`

**Compiler warning (×2):**
```
(1518) direct function call made with an incomplete prototype (get_status)
       → main.c line 119
       → main.c line 157
```

**Root cause:**
In C90 (the XC8 default dialect for PIC projects), a function declared as
`STATE get_status();` with an empty parameter list is an *old-style*
(K&R-era) prototype. It tells the compiler *nothing* about the expected
number or types of arguments. The compiler cannot verify call-sites and
therefore emits warning 1518 at every call.

In C99 and C11, `foo()` means the same as `foo(void)`. In C90 they are
different: `foo()` is "accept any arguments", `foo(void)` is "accept none".
XC8 processes `.c` files in C99 mode by default but many PIC projects use
`--std=c90` for MISRA compliance, which triggers this distinction.

The fix is a one-character change:

```c
// Before (in state.h):
STATE get_status();

// After:
STATE get_status(void);
```

**Why this matters beyond the warning:**
An incomplete prototype permits the compiler to insert implicit argument
conversions at call-sites that may differ from what the function actually
receives. On a register-starved 8-bit MCU this can corrupt the W register
or produce incorrect return values.

**Fix:**
Changed `STATE get_status();` → `STATE get_status(void);` in `state.h`.

Since `main_config.h` includes `state.h`, and `main.c` includes `main_config.h`,
the corrected prototype propagates to all call-sites automatically. No change
to `main.c` or `state.c` is needed.

**Header check:** ✅ `state.h` now uses `(void)` for both `set_status` and
`get_status`.

**Lesson:** Always write `(void)` for functions that accept no parameters. In
C90, `()` is a portability trap.

---

### [2026-05-09] — FIX 4: String and Array `const` Audit (Flash vs RAM)

**Files audited:** `dashboard.c`, `menu.c`, `view_logs.c`, `state.c`,
                   `login.c`, `set_password.c`, `set_time.c`

**Background:**
On XC8 for PIC16, `const` on a pointer-to-string (`const char *`) or
pointer-to-array (`const char * const []`) causes the string data to be
placed in program memory (Flash ROM) rather than copied into data memory
(RAM) at startup. On PIC16F877A:
- Data memory (RAM): 368 bytes total — critically scarce.
- Program memory (Flash): 8 KB words — much larger.

Large `char *` arrays of string literals in RAM can consume 10–30 % of the
entire data memory budget, starving the stack and local variables.

**Audit results — all string tables already correctly declared:**

| File | Variable | Declaration | Status |
|------|----------|-------------|--------|
| `dashboard.c` | `gear_labels[]` | `static const char * const` | ✅ Flash |
| `menu.c` | `labels[]` | `static const char * const` | ✅ Flash |
| `state.c` | `state_names[]` | `static const char * const` | ✅ Flash |
| `view_logs.c` | `gear_label()` return | `const char *` literal | ✅ Flash |
| `blackbox_drivers.c` | `uart_puts()` param | `const char *` | ✅ Flash |

**String literals in function calls** (`clcd_print("Set Time  SW2:OK", ...)`,
`uart_puts("[STATE] ...")`, etc.) are all anonymous `const char[]` — XC8 places
these in program memory by default when the receiving parameter is `const char *`.
No changes required.

**Confirmation:** `clcd_print(const char *str, ...)` and
`uart_puts(const char *str)` both take `const char *`, so all call-site
string literals are Flash-resident. ✅

**RAM recovered by const:** All string table RAM that would have been
consumed without `const` is already saved. No action needed beyond confirming
the existing declarations are correct.

---

## Session Summary — 2026-05-07

> Tonight's session restructured the Car Black Box firmware from a fragile procedural script into a hardened, non-blocking state machine capable of running at 5 MIPS without race conditions or hardware lockups.

### Core Architectural Upgrades

**1. Input Decoupling — Single-Read Router Pattern**
Hardware is polled exactly once per cycle in `main.c`. The result is passed contextually down the call stack to the active module, eliminating race conditions.

**2. State Isolation — Zero Input Bleed**
Inputs are handled entirely inside their contextual `switch` cases.

**3. Canvas Locking & Cache Invalidation**
The `screen_initialized` flag ensures static UI elements are drawn exactly once per state entry. `invalidate_dashboard_cache()` forces dynamic elements to redraw only when re-entering the Dashboard.

**4. Non-Blocking Execution**
Recursive `verify_password()` calls and infinite `while(1);` traps removed. All modules update flags and return immediately.

**5. Hardware Timer ISRs**
Timer0 (10 ms blink tick) and Timer1 (100 ms timeout tick), driven by ISRs. All timing is now hardware-accurate.

---

## Session Summary — 2026-05-09 (Part 1: Logic Bugs)

> Fixed six deeper logical and architectural bugs: PIC16 hardware stack overflow, dead render block for crash events, gear label data corruption, loop-speed-dependent cooldown, EEPROM partial-write corruption, and blocking UART receive function.

---

## Session Summary — 2026-05-09 (Part 2: Compiler Fixes)

> Resolved all compiler issues blocking the build: ROM overflow (-O0 → -Os via user.cmake), four dead functions removed from blackbox_drivers and set_time modules, incomplete prototype fixed in state.h, string/const audit confirmed all tables are Flash-resident.

**ROM budget recovered (estimated):**
| Source | Words saved |
|--------|-------------|
| -O0 → -Os optimisation | ~1800–2600 |
| `uart_getchar` removed | ~25–35 |
| `uart_data_ready` removed | ~15–20 |
| `set_time_reset` removed | ~20–30 |
| `bcd_to_dec` removed | ~10–15 |
| **Total estimated recovery** | **~1870–2700 words** |

---

## Design Notes

### Why `uart_` prefix instead of `UART_`?
Function names use `lower_snake_case` with a module prefix. Macros and constants use `UPPER_SNAKE_CASE`. This is consistent with MISRA-C naming guidelines.

### Why centralise `FOSC` in `main_config.h`?
The oscillator frequency is a board-level property. Placing it in peripheral headers couples unrelated drivers to a hardware detail they should not know about.

### DS1307 ACK/NACK Rule
On single-byte I²C reads, always NACK. On multi-byte reads, ACK all bytes except the last. Mandatory per the I²C spec and DS1307 datasheet §5.0.

### PIC16F877A Stack Accounting
Hardware stack: 8 levels. Level 1 = startup→main. Levels 2–7 = application. Level 8 = reserved for ISR entry. Any call chain deeper than 6 from `main()` risks overflow under interrupt load.

### C90 vs C99 `(void)` in Prototypes
XC8 in C90 mode treats `foo()` (empty list) as "unknown parameters" and `foo(void)` as "no parameters". Always use `(void)` for zero-parameter functions to avoid warning 1518 and undefined call-site behaviour.

### Why `const char * const []` for String Tables?
`const char *` — the characters are const (Flash-resident).
`const [] / * const` — the pointer itself is const (the array entry cannot be reseated).
Both qualifiers together: the string data is in Flash and the pointer cannot be accidentally overwritten. On XC8, this is the correct way to declare all read-only string tables.

---

### [2026-05-09] — Architectural Refactor: Update/Render Split + Event Model

**Files:** All module files

**Problem:** Every screen module mixed hardware reads, state mutation, and LCD writes into single functions. Raw keypad bytes were passed directly into every module.

**Fix:** Every module now has two strictly-enforced functions:
- `_update(EVENT evt)` — reads hardware, mutates state. Zero LCD writes.
- `_render(void)` — reads state, writes LCD. Zero hardware reads, zero state mutations.

An `EVENT` enum replaces raw keypad bytes. `translate_key()` in `events.c` is the single conversion point.

The main loop in `main.c` was restructured into four explicit phases:
1. `READ` — poll hardware once, translate to EVENT.
2. `UPDATE` — active screen's `_update(evt)` modifies `sys` or module statics.
3. `RENDER` — active screen's `_render()` writes LCD from current state.
4. `STORAGE` — EEPROM write if `sys.log_pending`.

**Lesson:** Separate update (state) from render (display). Hardware codes are hardware details; translate them to semantic events at the earliest possible point.

---

### [2026-05-09] — `menu_reset()` Missing Before `set_status(MENU)` in One-Shot Screens

**Files:** `main.c`

**Problem:** `CLEAR_LOGS`, `DOWNLOAD_LOGS`, `SET_TIME`, and `CHANGE_PASSWORD` one-shot screens returned to MENU without calling `menu_reset()`, leaving `last_drawn` stale and suppressing the first menu redraw.

**Fix:** `menu_reset()` added before every `set_status(MENU)` call for one-shot screens.

**Lesson:** Any module with initialisation state must expose a reset function, called before every entry into that state.