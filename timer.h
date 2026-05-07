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

/** @brief  Resets the timeout tick counter to zero. */
void reset_timeout_tick(void);

#endif /* TIMER_H */