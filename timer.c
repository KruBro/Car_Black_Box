/**
 * @file    timer.c
 * @author  krubro
 * @date    2026-05-08
 * @brief   Timer0 and Timer1 ISR driver — replaces all software loop timers.
 *
 * @details
 *   Timer0 — 8-bit, internal clock, prescaler 1:256
 *     Overflow period = (256 - TMR0_PRELOAD) * 256 * (1 / (FOSC/4))
 *                     = 195 * 256 * 200 ns  ≈ 9.98 ms  → blink_tick
 *
 *   Timer1 — 16-bit, internal clock, prescaler 1:8
 *     Overflow period = (65536 - TMR1_PRELOAD) * 8 * (1 / (FOSC/4))
 *                     = 62500 * 8 * 200 ns   = 100.0 ms → timeout_tick
 */

#include "main_config.h"

// Timer0: reload value for ~10 ms at 20 MHz / 4 with 1:256 prescaler
#define TMR0_PRELOAD    61U         // 256 - 195 = 61

// Timer1: reload value for ~100 ms at 20 MHz / 4 with 1:8 prescaler
#define TMR1_PRELOAD    3036U       // 65536 - 62500 = 3036
#define TMR1H_PRELOAD   ((TMR1_PRELOAD) >> 8)       // 0x0B
#define TMR1L_PRELOAD   ((TMR1_PRELOAD) & 0xFFU)    // 0xEC

volatile unsigned char blink_tick   = 0;
volatile unsigned int  timeout_tick = 0;

/**
 * @brief  Configures Timer0 (prescaler 1:256) and Timer1 (prescaler 1:8),
 *         then enables Timer0, Timer1, peripheral, and global interrupts.
 */
void init_timers(void)
{
    // Timer0: internal clock (T0CS=0), prescaler to TMR0 (PSA=0), 1:256 (PS=111)
    OPTION_REG = (OPTION_REG & 0xC0U) | 0x07U;
    TMR0   = TMR0_PRELOAD;
    TMR0IF = 0;
    TMR0IE = 1;

    // Timer1: internal clock (TMR1CS=0), prescaler 1:8 (T1CKPS=11), on (TMR1ON=1)
    T1CON  = 0x31U;
    TMR1H  = TMR1H_PRELOAD;
    TMR1L  = TMR1L_PRELOAD;
    TMR1IF = 0;
    TMR1IE = 1;

    PEIE = 1;   // Enable peripheral interrupts (required for Timer1)
    GIE  = 1;   // Enable global interrupts
}

void reset_blink_tick(void)   
{ 
    blink_tick   = 0; 
}

void reset_timeout_tick(void)
{ 
    timeout_tick = 0; 
}

/**
 * @brief  Single ISR entry point — services Timer0 and Timer1 overflow flags.
 */
void __interrupt() isr(void)
{
    if (TMR0IF) // Timer0 overflow — ~10 ms tick
    {
        TMR0   = TMR0_PRELOAD;  // Reload (minor jitter acceptable for UI timing)
        TMR0IF = 0;
        blink_tick++;
    }

    if (TMR1IF) // Timer1 overflow — ~100 ms tick
    {
        TMR1H  = TMR1H_PRELOAD;
        TMR1L  = TMR1L_PRELOAD;
        TMR1IF = 0;
        if (timeout_tick < 0xFFFFU) timeout_tick++; // Guard against overflow
    }
}