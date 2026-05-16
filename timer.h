/**
 * @file    timer.h
 * @author  krubro
 * @date    2026-05-08
 * @brief   Hardware timer driver — Timer0 (10 ms tick) and Timer1 (100 ms tick).
 *
 * @note    Call init_timers() once before the main loop.
 *          Read blink_tick and timeout_tick from any module; reset via helpers.
 */

#ifndef TIMER_H
#define TIMER_H

#include <xc.h>

// Tick counters written by ISR, read by application modules
extern volatile unsigned char blink_tick;   // Incremented every ~10 ms  (Timer0)
extern volatile unsigned int  timeout_tick; // Incremented every ~100 ms (Timer1)

/** @brief  Configures Timer0 and Timer1, enables interrupts. */
void init_timers(void);

/** @brief  Resets the blink tick counter to zero. */
void reset_blink_tick(void);

/** @brief  Resets the timeout tick counter to zero (atomic — GIE protected). */
void reset_timeout_tick(void);

/**
 * @brief  Returns a safe atomic snapshot of timeout_tick.
 *
 * On PIC16F877A, reading a 16-bit variable requires two 8-bit instructions.
 * The Timer1 ISR can fire between them and corrupt the value read by the
 * main context. This function disables global interrupts for the minimum
 * time needed to copy both bytes, then re-enables them.
 *
 * Use this function everywhere timeout_tick is read outside the ISR.
 * Do NOT read timeout_tick directly from main-context code.
 */
unsigned int get_timeout_tick(void);

#endif /* TIMER_H */