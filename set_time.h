/**
 * @file set_time.h
 * @author krubro
 * @date 2026-05-09
 * @brief Set-time state — field-by-field RTC editing with copy isolation,
 *        update/render split.
 *
 * Controls:
 *   SW4 — increment active field (with rollover)
 *   SW5 — decrement active field (with underflow protection)
 *   SW6 — cycle active field: HH → MM → SS → HH
 *   SW1 — discard all edits and return to MENU
 *   SW2 — commit copy to DS1307 and return to MENU
 *
 * Design rule:
 *   set_time_reset() copies sys.hours/minutes/seconds into a local buffer.
 *   All edits modify the local buffer only.
 *   SW1 discards the buffer — the DS1307 is never touched.
 *   SW2 flushes the buffer to the DS1307 via ds1307_i2c_write().
 */

#ifndef SET_TIME_H
#define SET_TIME_H

#include "events.h"

/*
 * FIX [2026-05-09] — Dead Code Removal (Compiler Warning: unused function)
 *
 * set_time_reset() is never called in the current codebase. It is commentedzzz
 * out here (declaration) and in set_time.c (definition + bcd_to_dec helper).
 *
 * void set_time_reset(void); // Call once on transition INTO SET_TIME state
 */

void set_time_update(EVENT evt);
void set_time_render(void);

#endif /* SET_TIME_H */