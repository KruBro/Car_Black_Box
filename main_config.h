/**
 * @file    main_config.h
 * @author  krubro
 * @date    2026-05-05
 * @brief   Global project configuration — hardware constants, peripheral includes,
 *          and forward declarations shared across all modules.
 */

#ifndef MAIN_CONFIG_H
#define MAIN_CONFIG_H

/* -------------------------------------------------------------------------
 * Configuration Bits — PIC16F877A @ 20 MHz (PICGENIOS / PICSIMSLAB)
 * ------------------------------------------------------------------------- */
#pragma config FOSC  = HS       // High-Speed crystal oscillator
#pragma config WDTE  = OFF      // Watchdog Timer disabled
#pragma config PWRTE = OFF      // Power-up Timer disabled
#pragma config BOREN = ON       // Brown-out Reset enabled
#pragma config LVP   = OFF      // Low-Voltage Programming disabled
#pragma config CPD   = OFF      // Data EEPROM code protection off
#pragma config WRT   = OFF      // Flash write protection off
#pragma config CP    = OFF      // Code protection off

/* -------------------------------------------------------------------------
 * Oscillator Frequency — defined ONCE here, nowhere else
 * ------------------------------------------------------------------------- */
#ifndef _XTAL_FREQ
#define _XTAL_FREQ  20000000UL  // Required by __delay_ms / __delay_us macros
#endif

#ifndef FOSC
#define FOSC        20000000UL  // Used for UART and I2C baud rate calculations
#endif

/* -------------------------------------------------------------------------
 * Headers
 * ------------------------------------------------------------------------- */
#include <xc.h>
#include "blackbox_drivers.h"
#include "timer.h"
#include "state.h"
#include "login.h"

/* -------------------------------------------------------------------------
 * Forward Declarations
 * ------------------------------------------------------------------------- */
void invalidate_dashboard_cache(void);
void clcd_dashboard(unsigned char key);

#endif /* MAIN_CONFIG_H */