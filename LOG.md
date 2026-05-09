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

If Timer0 (10 ms) or Timer1 (100 ms) fired while the CPU was at depth 8, the ISR pushed depth to 9. The stack pointer silently wrapped to 1, overwriting the return address of `main()`. Execution jumped to a corrupt address and the PIC rebooted. This was intermittent because the reboot only occurred if a timer interrupt happened to fire during the ~5 ms EEPROM ACK-poll window.

**Fix (three-part):**
1. `flush_pending_logs()` removed as a function. The EEPROM write is now inlined directly in `main()`'s while loop, saving one call level.
2. `write_slot()` merged into `eeprom_write_log()`, saving another level.
3. `ack_poll()` inlined into `eeprom_write_byte()` and the page-write block, saving a third level.

Worst-case depth after fix:
```
startup(1) -> main(2) -> eeprom_write_log(3) -> eeprom_write_byte(4)
  -> i2c_start(5) -> i2c_wait_for_idle(6). ISR: 7. Safe.
```

**Lesson:** On resource-constrained MCUs with a fixed hardware stack, every function call has a measurable cost. Deep call chains must be audited against the stack limit, especially when ISRs are active. The fix is to reduce call depth by inlining small functions rather than adding explicit `noinline` attributes that would hide the problem.

---

### [2026-05-09] — Crash Event Never Displayed on CLCD (Dead Render Block)

**File:** `dashboard.c`, `display_event()`

**Problem:** When SW1 was pressed, `FLAG_CRASH` was latched in `current_log.flags`. On every subsequent call to `display_event()`, the very first check was:

```c
if (current_log.flags & FLAG_CRASH) {
    current_log.gear = (unsigned char)gear;
    return;   // Early return here
}
// ... key handling ...
if (current_log.flags & FLAG_CRASH)
    clcd_print("C ", LINE2(10));  // Never reached
```

The early `return` meant the render block at the bottom of the function was dead code the moment a crash was latched. The LCD was frozen on whatever gear label was last displayed (e.g., "G3"), never showing "C ".

**Fix:** Restructured with a single guard: `if (!(current_log.flags & FLAG_CRASH))` wraps only the key-processing block. The render block is now always reached. A crash correctly shows "C " on the display for all subsequent frames.

**Lesson:** When a function has multiple exit points (early returns), trace every path to confirm that all side-effects (in this case, the LCD write) are reached. Early returns are a code smell in render functions.

---

### [2026-05-09] — Gear Label Corruption in Log Viewer and UART Download

**Files:** `view_logs.c`, `eeprom.c`

**Problem:** The EEPROM stores gear as the raw `GEAR_STATE` index encoded as an ASCII digit: `'0'` = Reverse, `'1'` = Neutral, `'2'` = First gear, etc. The viewer and downloader printed this byte literally as `"G"` + digit, producing `"G0"` for Reverse and `"G1"` for Neutral. A forensic investigator reading a post-crash UART dump would see `"G1"` and conclude the vehicle was in First Gear when it was actually in Neutral — a factual error that corrupts the integrity of the black box record.

**Fix:** Added `gear_label(char g)` in `view_logs.c`. It maps each storage byte to its correct two-character label:

| Stored | Displayed (wrong) | Displayed (fixed) |
|--------|-------------------|-------------------|
| `'0'`  | `G0`              | `GR`              |
| `'1'`  | `G1`              | `GN`              |
| `'2'`  | `G2`              | `G1`              |
| `'3'`  | `G3`              | `G2`              |
| `'4'`  | `G4`              | `G3`              |
| `'5'`  | `G5`              | `G4`              |
| `'C'`  | `GC`              | `CR`              |

Both `render_entry()` (CLCD) and `download_logs()` (UART) now call `gear_label()`.

**Lesson:** Any time data is stored in a compact representation (index, enum ordinal, BCD), a dedicated decode function must be used at every read site. Printing the raw storage value directly is always wrong.

---

### [2026-05-09] — CPU-Loop-Dependent Login Cooldown Was Effectively Instantaneous

**File:** `login.c`

**Problem:** To prevent key bounce on entering the login screen, the code used:

```c
if (entry_cooldown < 50) { entry_cooldown++; return; }
```

`entry_cooldown` was incremented once per `while(1)` iteration. Before the Timer ISR refactor, the loop was slow enough that 50 cycles took measurable time. After the ISR refactor, the main loop runs at near-full 5 MIPS throughput, so 50 iterations complete in under 0.2 ms. A single physical button press lasting ~100 ms would register several digits.

**Fix:** Replaced with a hardware-timer-based gate. At state entry, `entry_tick = timeout_tick` is recorded. Input is suppressed until `timeout_tick - entry_tick >= 5` (i.e., 500 ms of real time, unconditionally). `timeout_tick` is driven by Timer1 (100 ms ISR tick) and is independent of loop speed.

**Lesson:** Any timing requirement — even a debounce cooldown — must be based on a hardware timer. Loop counters produce timing that silently changes whenever the loop body changes. The fix has the additional benefit of being self-documenting: `5U` timer ticks is obviously 500 ms; `50U` loop iterations is meaningless without a calibration comment.

---

### [2026-05-09] — EEPROM Mid-Write Record Corruption

**File:** `eeprom.c`, `write_slot()`

**Problem:** The original `write_slot()` called `eeprom_write_byte()` 11 times in a for loop. If any single call returned failure (I²C NACK, bus glitch, power dip), the loop aborted early. The `head` and `count` metadata were only updated after a successful return, so the system would silently skip the failed slot. However, the partially written record — for example, six bytes of valid time data followed by five bytes of stale data from a previous entry — remained in the EEPROM at that slot address. The next valid write would advance `head` past it, leaving the corrupted partial record permanently readable by `eeprom_read_log()`.

**Fix:** The 11-byte record is now sent in a single AT24C04 page-write transaction (one I²C START → address → 11 data bytes → STOP). The EEPROM's internal page latch either commits all 11 bytes atomically or discards them entirely. There is no intermediate state where a partial record can be read back.

**Lesson:** When multiple writes must be atomic, use the hardware's native batch mechanism — in this case, I²C page write. Sequential byte writes with individual ACK polls are neither atomic nor efficient.

---

### [2026-05-09] — `uart_getchar()` Was a System-Locking Blocking Call

**File:** `blackbox_drivers.c`

**Problem:** `uart_getchar()` contained `while (!RCIF);` — a permanent block waiting for a byte to arrive on the RX pin. The function was never called in production code, but if any developer had tried to implement `SET_TIME` or `CHANGE_PASSWORD` using serial input (the natural choice), calling `uart_getchar()` would have frozen the entire firmware. The dashboard would stop updating, the crash-logging ISR would continue firing but no log entries would be flushed, and the device would be completely unresponsive to the keypad until a byte arrived over UART.

**Fix:** `uart_getchar()` now returns immediately with `0` if no byte is available (`!RCIF`). A companion function `uart_data_ready()` returns `RCIF` so callers can poll without blocking:

```c
if (uart_data_ready()) {
    ch = uart_getchar();
}
```

**Lesson:** In an embedded event loop, any function that blocks indefinitely is a latent denial-of-service. Driver functions must always be non-blocking; blocking behaviour must be composed explicitly in the caller where the wait is intentional.

---

### [2026-05-09] — Menu SW4 Used `last_i` (Sentinel 0xFF) Instead of `i`

**File:** `menu.c`

**Problem:** `last_i` is initialised to `0xFF` as a sentinel to force a full redraw on the first call. The SW4 handler called `set_status(menu_states[last_i])`. If the user pressed SW4 without navigating first, `last_i` was still `0xFF` and `menu_states[0xFF]` was an out-of-bounds array read on the PIC's data memory, loading a garbage `STATE` value and transitioning to an undefined screen.

**Fix:** The SW4 handler now uses `i` (the live, always-valid selection index), not `last_i`. `last_i` is now used only as a redraw guard.

**Lesson:** Sentinel values (e.g., `0xFF` as "unset") must never be dereferenced as array indices. Keep the live value and the cache-invalidation sentinel in separate variables with clear, distinct roles.

---

## Session Summary — 2026-05-07

> Tonight's session restructured the Car Black Box firmware from a fragile procedural script into a hardened, non-blocking state machine capable of running at 5 MIPS without race conditions or hardware lockups.

### Core Architectural Upgrades

**1. Input Decoupling — Single-Read Router Pattern**
Hardware is polled exactly once per cycle in `main.c`. The result is passed contextually down the call stack to the active module, eliminating the race condition where `dashboard.c` consumed the `SW4` edge before the global router could act on it.

**2. State Isolation — Zero Input Bleed**
Inputs are handled entirely inside their contextual `switch` cases. The input buffer is explicitly zeroed (`key = ALL_RELEASED`) on state transition, preventing the physical hold of `SW4` from ghosting into the newly initialised Login screen.

**3. Canvas Locking & Cache Invalidation**
The `screen_initialized` flag ensures static UI elements are drawn exactly once per state entry. `invalidate_dashboard_cache()` forces dynamic elements to redraw only when re-entering the Dashboard from another state.

**4. Non-Blocking Execution**
Recursive `verify_password()` calls and infinite `while(1);` traps were removed. All modules update flags and return control to the master loop immediately.

**5. Hardware Timer ISRs**
Loop-counter timers replaced with Timer0 (10 ms blink tick) and Timer1 (100 ms timeout tick), driven by ISRs. All timing in the codebase is now hardware-accurate and loop-speed-independent.

---

## Session Summary — 2026-05-09

> This session fixed six deeper logical and architectural bugs identified by static analysis: a PIC16 hardware stack overflow, a dead render block for crash events, gear label data corruption in the log viewer, a loop-speed-dependent cooldown, EEPROM partial-write corruption, and a blocking UART receive function. UART event logging was added to all modules.

### Core Changes

**1. Stack Depth Audit and Reduction**
Every function call from `main()` to the deepest I²C primitive was counted against the PIC16F877A's 8-level hardware limit. Three functions were eliminated or merged to bring the worst-case depth to 6 (with headroom for startup and ISR).

**2. UART Telemetry**
Every state transition, gear change, crash event, login attempt, and log operation now emits a `[TAG] message\n` line over UART at 9600 baud. This makes the serial monitor a complete real-time audit trail of the system.

**3. EEPROM Page Write**
11-byte log records are now committed in a single I²C page write, making each record atomic with respect to power faults and I²C glitches.

**4. Non-Blocking UART**
`uart_getchar()` no longer blocks. `uart_data_ready()` added for poll-before-read pattern.

---

## Design Notes

### Why `uart_` prefix instead of `UART_`?
Function names use `lower_snake_case` with a module prefix. Macros and constants use `UPPER_SNAKE_CASE`. This is consistent with MISRA-C naming guidelines and makes the origin of every identifier immediately clear.

### Why centralise `FOSC` in `main_config.h`?
The oscillator frequency is a board-level property. Placing it in peripheral headers couples unrelated drivers to a hardware detail they should not know about.

### DS1307 ACK/NACK Rule
On single-byte I²C reads, always NACK. On multi-byte reads, ACK all bytes except the last. This is mandatory per the I²C specification and DS1307 datasheet §5.0.

### PIC16F877A Stack Accounting
The hardware stack is 8 levels. Level 1 is consumed by the startup-to-main call. Levels 2–7 are available to application code. Level 8 must be left free for ISR entry. Any call chain deeper than 6 from `main()` risks stack overflow under interrupt load.

### Why `current_log` Is Not `static` in `dashboard.c`
Making `current_log` visible to `main.c` allows `eeprom_write_log()` to be called directly from the main loop without an intermediary `flush_pending_logs()` function. Removing that one wrapper saves one hardware stack level — the difference between safe and overflowing.

---

### [2026-05-09] — Architectural Refactor: Update/Render Split + Event Model

**Files:** All module files

**Problem:** Every screen module (`dashboard.c`, `login.c`, `menu.c`, `view_logs.c`) mixed hardware reads, state mutation, and LCD writes into single functions. `display_event()` read a keypad byte, updated gear state, and wrote "G3" to the LCD in the same function. `display_speed()` read the ADC and printed the result in the same function. This made testing impossible, caused hidden side effects, and violated the principle that rendering should reflect state — not create it.

Raw keypad bytes (`SW1 = 0x3E`, `SW4 = 0x37`) were passed directly into every module. The meaning of those bytes was re-decoded by switch statements inside each module, coupling hardware pin assignments to application logic throughout the codebase.

**Fix:**
Every module now has two functions with strictly enforced contracts:
- `_update(EVENT evt)` — reads hardware, mutates state. Zero LCD writes.
- `_render(void)` — reads state, writes LCD. Zero hardware reads, zero state mutations.

An `EVENT` enum (`EVENT_NONE`, `EVENT_SW1`…`EVENT_SW6`) replaces raw keypad bytes. `translate_key()` in `events.c` is the single conversion point. Every module downstream receives a semantic event label, never a hardware mask.

`SYSTEM_STATE sys` centralises all vehicle data (time, gear, speed, flags) in one struct defined in `main.c`. No module holds a private copy of vehicle data.

The main loop in `main.c` was restructured into four explicit phases:
1. `READ` — poll hardware once, translate to EVENT.
2. `UPDATE` — active screen's `_update(evt)` modifies `sys` or module statics.
3. `RENDER` — active screen's `_render()` writes LCD from current state.
4. `STORAGE` — EEPROM write if `sys.log_pending`.

**Lesson:** Separate update (state) from render (display). Rendering should reflect state, not create it. Hardware codes are hardware details; translate them to semantic events at the earliest possible point so application logic never sees pin assignments.

---

### [2026-05-09] — `menu_reset()` Missing Before `set_status(MENU)` in One-Shot Screens

**Files:** `main.c`

**Problem:** `CLEAR_LOGS`, `DOWNLOAD_LOGS`, `SET_TIME`, and `CHANGE_PASSWORD` are one-shot screens that execute and immediately return to `MENU`. The `menu_reset()` call was missing before their `set_status(MENU)`, so `last_drawn` was never reset to the sentinel value. On re-entering the menu, `last_drawn` still held the last valid selection index, and `menu_render()` would skip the initial draw because `selection == last_drawn`.

**Fix:** `menu_reset()` added before every `set_status(MENU)` call for one-shot screens.

**Lesson:** Any module that has initialisation state (`screen_init`, `last_drawn`, `needs_draw`) must expose a reset function, and that function must be called before every entry into the state — including entries from unexpected paths.