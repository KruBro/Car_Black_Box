/**
 * @file    set_password.h
 * @author  krubro
 * @date    2026-05-09
 * @brief   Change-password state — dual-buffer PIN entry, 10 s timeout, update/render split.
 *
 * Flow:
 *   CP_ENTER_NEW   → user types 4-digit new PIN  (SW4='0', SW5='1')
 *   CP_RE_ENTER    → user re-types to confirm
 *   CP_MATCH       → render shows "PASS CHANGED", writes to EEPROM, → MENU
 *   CP_MISMATCH    → render shows "MISMATCH!", resets to CP_ENTER_NEW
 *
 * Timing:
 *   Cursor blink  : BLINK_THRESHOLD * ~10 ms  = ~200 ms  (Timer0)
 *   Idle timeout  : CP_TIMEOUT_TICKS * ~100 ms = 10 s     (Timer1)
 *   Entry cooldown: COOLDOWN_TICKS blink ticks = ~200 ms  (Timer0, same as login)
 */

#ifndef SET_PASSWORD_H
#define SET_PASSWORD_H

#include "events.h"

void set_password_reset(void);      // Call once on transition INTO CHANGE_PASSWORD state
void set_password_update(EVENT evt);
void set_password_render(void);

#endif /* SET_PASSWORD_H */