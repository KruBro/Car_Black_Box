/**
 * @file    main_config.h
 * @author  krubro
 * @date    2026-05-05
 * @brief   Global project configuration: hardware constants, peripheral
 *          includes, and forward declarations shared across all modules.
 *
 * @details
 *   Target hardware: PIC16F877A on the PICGENIOS board (PICSIMSLAB).
 *
 *   This is the single top-level header that every translation unit includes.
 *   It is the ONLY place where FOSC and _XTAL_FREQ should be defined so that
 *   the oscillator frequency is never duplicated or silently inconsistent.
 *
 *   Include order:
 *     1. MPLAB XC8 / device header (<xc.h>)
 *     2. Peripheral driver headers
 *     3. Application-level forward declarations
 */

#ifndef MAIN_CONFIG_H
#define MAIN_CONFIG_H

/* -------------------------------------------------------------------------
 * Fuse / Configuration Bits  (PIC16F877A — XC8 syntax)
 * Board: PICGENIOS on PICSIMSLAB
 * ------------------------------------------------------------------------- */
#pragma config FOSC  = HS       /**< High-Speed crystal oscillator (20 MHz) */
#pragma config WDTE  = OFF      /**< Watchdog Timer disabled (safe for dev) */
#pragma config PWRTE = OFF      /**< Power-up Timer disabled                */
#pragma config BOREN = ON       /**< Brown-out Reset enabled                */
#pragma config LVP   = OFF      /**< Low-Voltage Programming disabled       */
#pragma config CPD   = OFF      /**< Data EEPROM code protection off        */
#pragma config WRT   = OFF      /**< Flash write protection off             */
#pragma config CP    = OFF      /**< Code protection off                    */

/* -------------------------------------------------------------------------
 * Oscillator Frequency
 * Define ONCE here; remove from all peripheral headers.
 * ------------------------------------------------------------------------- */
#ifndef _XTAL_FREQ
/** @brief Crystal oscillator frequency in Hz. Used by __delay_ms/_us macros. */
#define _XTAL_FREQ      20000000UL
#endif

#ifndef FOSC
/** @brief CPU clock frequency in Hz. Used for UART and I2C baud calculations. */
#define FOSC            20000000UL
#endif

/* -------------------------------------------------------------------------
 * Compiler / Device Header
 * ------------------------------------------------------------------------- */
#include <xc.h>

/* -------------------------------------------------------------------------
 * Peripheral Driver Headers
 * ------------------------------------------------------------------------- */
#include "clcd.h"
#include "ds1307.h"
#include "i2c.h"
#include "uart.h"
#include "digital_keypad.h"
#include "adc.h"

/* -------------------------------------------------------------------------
 * Application-level Forward Declarations (defined in dashboard.c)
 * ------------------------------------------------------------------------- */

/**
 * @brief   Refreshes the entire CLCD dashboard (time, event, speed).
 * @details Called once per main loop iteration. Internally calls
 *          get_time(), display_time(), display_event(), and display_speed().
 */
void clcd_dashboard(void);

/**
 * @brief   Reads HH:MM:SS registers from the DS1307 into a caller-supplied buffer.
 * @param[out] clock_reg  Three-element array: [0]=sec, [1]=min, [2]=hour (BCD).
 */
static void display_time(unsigned char *clock_reg);

/**
 * @brief   Formats and prints the time stored in clock_reg to the CLCD.
 * @param[in] clock_reg  Three-element BCD array from get_time().
 */
static void get_time(unsigned char *clock_reg);

#endif /* MAIN_CONFIG_H */