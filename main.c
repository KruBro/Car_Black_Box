/**
 * @file    main.c
 * @author  krubro
 * @date    2026-05-09
 * @brief   Application entry point — event-driven main loop.
 *
 * Loop structure:
 *   1. READ INPUTS    — hardware polled once; translated to one EVENT.
 *   2. UPDATE STATE   — active screen's _update(evt) modifies sys / module state.
 *   3. RENDER OUTPUTS — active screen's _render() writes LCD from current state.
 *   4. HANDLE STORAGE — EEPROM write if dashboard_update() set sys.log_pending.
 *
 * Stack depth (PIC16F877A, 8-level hardware limit):
 *   main(1) → eeprom_write_log(2) → eeprom_write_byte(3)
 *           → i2c_start(4) → i2c_wait_for_idle(5).
 *   ISR adds 1 → max 6. Safe (limit is 8).
 */

#include "main_config.h"

// Single authoritative vehicle state — owned by main.c, written by dashboard_update()
SYSTEM_STATE sys = {0};

static void init_config(void)
{
    init_clcd();
    init_digital_keypad();
    initUART(9600UL);
    initI2C(100000UL);
    init_rtc();
    initADC();
    init_timers();
    login_load_password();  // Load PIN from EEPROM before the main loop
}

static void log_entry_from_sys(LOG_ENTRY *e)
{
    e->hours   = sys.hours;
    e->minutes = sys.minutes;
    e->seconds = sys.seconds;
    e->speed   = sys.speed;
    e->gear    = (unsigned char)sys.gear;
    e->flags   = sys.flags;
}

void main(void)
{
    EVENT     evt;
    LOG_ENTRY pending_entry;

    init_config();

    uart_puts("=== CAR BLACK BOX STARTED ===\r\n");
    uart_puts("[STATE] DASHBOARD\r\n");

    while (1)
    {
        /* ---- 1. READ INPUTS -------------------------------------------- */
        evt = translate_key(read_digital_keypad(EDGE));

        /* ---- 2. UPDATE STATE -------------------------------------------- */
        switch (get_status())
        {
        case DASHBOARD:
            if (evt == EVENT_SW4)
            {
                if (is_logged_in())
                {
                    menu_reset();
                    set_status(MENU);
                }
                else
                {
                    login_reset();
                    set_status(LOGIN);
                }
                evt = EVENT_NONE;
            }
            else
            {
                dashboard_update(evt);
            }
            break;

        case LOGIN:
            if (evt == EVENT_SW1)
            {
                uart_puts("[LOGIN] CANCELLED\r\n");
                login_reset();
                invalidate_dashboard_cache();
                set_status(DASHBOARD);
                evt = EVENT_NONE;
            }
            else
            {
                login_update(evt);
            }
            break;

        case MENU:
            menu_update(evt);
            break;

        case VIEW_LOGS:
            view_logs_update(evt);
            break;

        case CHANGE_PASSWORD:
            set_password_update(evt);
            break;

        case SET_TIME:
            set_time_update(evt);
            break;

        case CLEAR_LOGS:
        case DOWNLOAD_LOGS:
            break; // One-shot — handled in render phase
        }

        /* ---- 3. RENDER OUTPUTS ------------------------------------------ */
        switch (get_status())
        {
        case DASHBOARD:
            dashboard_render();
            break;

        case LOGIN:
            login_render();
            break;

        case MENU:
            menu_render();
            break;

        case VIEW_LOGS:
            view_logs_render();
            break;

        case CHANGE_PASSWORD:
            set_password_render();
            break;

        case SET_TIME:
            set_time_render();
            break;

        case CLEAR_LOGS:
            clear_logs();
            menu_reset();
            set_status(MENU);
            break;

        case DOWNLOAD_LOGS:
            download_logs(); // Runs once, auto-transitions inside
            break;
        }

        /* ---- 4. HANDLE STORAGE ------------------------------------------ */
        if (sys.log_pending)
        {
            sys.log_pending = 0;
            log_entry_from_sys(&pending_entry);
            if (eeprom_write_log(&pending_entry))
                uart_puts("[LOG] EEPROM OK\r\n");
            else
                uart_puts("[LOG] EEPROM FAIL\r\n");
        }
    }
}