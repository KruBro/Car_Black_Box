/**
 * @file    main.c
 * @author  krubro
 * @date    2026-05-05
 * @brief   Application entry point — peripheral init and main state-machine loop.
 */

#include "main_config.h"

static void init_config(void);

/**
 * @brief  Initialises all hardware peripherals in dependency order.
 */
static void init_config(void)
{
    init_clcd();            // GPIO only — safe first
    init_digital_keypad();  // GPIO only
    initUART(9600UL);       // USART must be ready before any debug prints
    initI2C(100000UL);      // MSSP must be ready before RTC
    init_rtc();             // Clears DS1307 Clock Halt bit
    initADC();              // A/D module
    init_timers();          // Timer0 (10 ms) and Timer1 (100 ms) — enables GIE
}

/**
 * @brief  Application entry point. Never returns.
 */
void main(void)
{
    init_config();

    unsigned char key;

    while (1)
    {
        key = read_digital_keypad(EDGE); // Poll hardware exactly once per cycle

        switch (get_status())
        {
        case DASHBOARD:
            if (key == SW4)
            {
                set_status(LOGIN);
                key = ALL_RELEASED; // Prevent SW4 bleeding into Login screen
            }
            else
            {
                clcd_dashboard(key);
            }
            break;

        case LOGIN:
            if (key == SW1)
            {
                invalidate_dashboard_cache();
                set_status(DASHBOARD);
                key = ALL_RELEASED;
            }
            else
            {
                show_login_screen(key);
            }
            break;

        case MENU:
            break;

        case VIEW_LOGS:
            break;

        case CLEAR_LOGS:
            break;

        case DOWNLOAD_LOGS:
            break;

        case SET_TIME:
            break;

        case CHANGE_PASSWORD:
            break;
        }
    }
}