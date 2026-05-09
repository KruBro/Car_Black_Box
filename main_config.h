/**
 * @file    main_config.h
 * @author  krubro
 * @date    2026-05-09
 * @brief   Global project configuration — hardware constants, types, includes.
 */

#ifndef MAIN_CONFIG_H
#define MAIN_CONFIG_H

#pragma config FOSC  = HS
#pragma config WDTE  = OFF
#pragma config PWRTE = OFF
#pragma config BOREN = ON
#pragma config LVP   = OFF
#pragma config CPD   = OFF
#pragma config WRT   = OFF
#pragma config CP    = OFF

#ifndef _XTAL_FREQ
#define _XTAL_FREQ  20000000UL
#endif

#ifndef FOSC
#define FOSC        20000000UL
#endif

/* -------------------------------------------------------------------------
 * Gear state — index maps directly into the event[] render array.
 * ------------------------------------------------------------------------- */
typedef enum { GR = 0, GN, G1, G2, G3, G4 } GEAR_STATE;

/* -------------------------------------------------------------------------
 * Event flags — bitmask stored in SYSTEM_STATE.flags and LOG_ENTRY.flags.
 * ------------------------------------------------------------------------- */
#define FLAG_IGNITION_ON    0x01U
#define FLAG_CRASH          0x02U

/* -------------------------------------------------------------------------
 * EEPROM log entry — packed when writing; field order must not change.
 * ------------------------------------------------------------------------- */
typedef struct
{
    unsigned char hours;
    unsigned char minutes;
    unsigned char seconds;
    unsigned char speed;
    unsigned char gear;
    unsigned char flags;
} LOG_ENTRY;

/* -------------------------------------------------------------------------
 * SYSTEM_STATE — single authoritative vehicle state.
 *
 * Ownership rules:
 *   hours / minutes / seconds  — written by dashboard_update() from DS1307.
 *   gear / flags               — written by dashboard_update() from keypad.
 *   speed                      — written by dashboard_update() from ADC.
 *   log_pending                — set by dashboard_update(), cleared by main().
 * ------------------------------------------------------------------------- */
typedef struct
{
    unsigned char hours;
    unsigned char minutes;
    unsigned char seconds;
    GEAR_STATE    gear;
    unsigned char speed;
    unsigned char flags;
    unsigned char log_pending;
} SYSTEM_STATE;

extern SYSTEM_STATE sys;    // Defined once in main.c

/* -------------------------------------------------------------------------
 * Headers — lower layers first; types above must be defined before eeprom.h
 * ------------------------------------------------------------------------- */
#include <xc.h>
#include "blackbox_drivers.h"
#include "events.h"
#include "timer.h"
#include "eeprom.h"
#include "state.h"
#include "login.h"
#include "set_password.h"
#include "set_time.h"
#include "view_logs.h"
#include "menu.h"
#include "dashboard.h"

#endif /* MAIN_CONFIG_H */