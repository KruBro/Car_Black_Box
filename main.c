/**
 * @file    main.c
 * @author  krubro
 * @date    2026-05-05
 * @brief   Application entry point for the Car Black Box project.
 *
 * @details
 *   Initialises all on-board peripherals (CLCD, keypad, UART, I2C, RTC, ADC)
 *   and then enters an infinite loop that continuously refreshes the CLCD
 *   dashboard with current time, gear/event state, and vehicle speed.
 *
 *   Peripheral init order matters:
 *     1. CLCD   — GPIO-only, safe to configure first.
 *     2. Keypad — GPIO-only.
 *     3. UART   — configures EUSART; must precede any debug prints.
 *     4. I2C    — configures MSSP; must precede RTC init.
 *     5. RTC    — reads/writes DS1307 over I2C; depends on I2C being ready.
 *     6. ADC    — configures the A/D module; safe at any point after reset.
 */

#include "main_config.h"

/* -------------------------------------------------------------------------
 * Private Function Prototypes
 * ------------------------------------------------------------------------- */
static void init_config(void);

/* -------------------------------------------------------------------------
 * init_config
 * ------------------------------------------------------------------------- */
/**
 * @brief   Initialises all hardware peripherals used by the application.
 * @details Must be called once before the main loop. See file-level comment
 *          for the rationale behind the initialisation order.
 */
static void init_config(void)
{
    init_clcd();
    init_digital_keypad();
    initUART(9600UL);
    initI2C(100000UL);
    init_rtc();
    initADC();
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */
/**
 * @brief   Application entry point.
 * @return  This function never returns (infinite loop).
 */
void main(void)
{
    init_config();

    while (1)
    {
        clcd_dashboard();
    }
}