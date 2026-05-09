/**
 * @file    menu.c
 * @author  krubro
 * @date    2026-05-09
 * @brief   Scrollable menu — update/render split, LOGOUT item, inactivity timeout.
 *
 * Items:
 *   0  VIEW LOG        → VIEW_LOGS state
 *   1  CLEAR LOG       → CLEAR_LOGS state
 *   2  DOWNLOAD LOG    → DOWNLOAD_LOGS state
 *   3  SET TIME        → SET_TIME state
 *   4  CHANGE PASSWORD → CHANGE_PASSWORD state
 *   5  LOGOUT          → do_logout() + DASHBOARD (no state entry for logout)
 *
 * Inactivity:
 *   30 seconds (300 Timer1 ticks) of no input → auto-logout → DASHBOARD.
 *   timeout_tick is reset on state entry (menu_reset) and on every keypress.
 *
 * Render:
 *   Redraws only when selection changes (last_drawn sentinel).
 *   Padded 15-char strings overwrite previous content — no clcd_clear() needed.
 */

#include "main_config.h"
#include <builtins.h>

#define MENU_ITEMS          6U
#define MENU_INACTIVITY     300U    /* 300 × 100 ms = 30 s auto-logout threshold */
#define LOGOUT_INDEX        5U      /* Last item — handled separately in update() */

static const char * const labels[MENU_ITEMS] =
{
    "VIEW LOG       ",
    "CLEAR LOG      ",
    "DOWNLOAD LOG   ",
    "SET TIME       ",
    "CHANGE PASSWORD",
    "LOGOUT         "
};

/* Destinations for items 0..4 — LOGOUT (index 5) is handled explicitly */
static const STATE destinations[MENU_ITEMS - 1U] =
{
    VIEW_LOGS, CLEAR_LOGS, DOWNLOAD_LOGS, SET_TIME, CHANGE_PASSWORD
};

static unsigned char selection  = 0;
static unsigned char last_drawn = 0xFFU;  /* Sentinel — forces first draw, never used as index */

void menu_reset(void)
{
    selection  = 0;
    last_drawn = 0xFFU;
    reset_timeout_tick();  /* Inactivity timer starts fresh on state entry */
}

/* UPDATE — scrolls selection, triggers transitions. Zero LCD writes. */
void menu_update(EVENT evt)
{
    /* Any keypress resets the inactivity timer */
    if (evt != EVENT_NONE) reset_timeout_tick();

    /* Inactivity auto-logout */
    if (timeout_tick >= MENU_INACTIVITY)
    {
        uart_puts("[MENU] INACTIVITY — auto-logout\r\n");
        do_logout();
        invalidate_dashboard_cache();
        menu_reset();
        set_status(DASHBOARD);
        return;
    }

    if (evt == EVENT_SW2 && selection < MENU_ITEMS - 1U)
    {
        selection++;
    }
    else if (evt == EVENT_SW3 && selection > 0U)
    {
        selection--;
    }
    else if (evt == EVENT_SW4)
    {
        if (selection == LOGOUT_INDEX)
        {
            uart_puts("[MENU] LOGOUT\r\n");
            do_logout();
            menu_reset();
            clcd_clear();
            clcd_print("LOGGED OUT", LINE1(0));
            __delay_ms(2000);
            invalidate_dashboard_cache();
            set_status(DASHBOARD);
        }
        else
        {
            uart_puts("[MENU] SELECT: ");
            uart_puts(labels[selection]);
            uart_puts("\r\n");
            set_status(destinations[selection]);
            menu_reset();
        }
    }
    else if (evt == EVENT_SW1)
    {
        uart_puts("[MENU] BACK\r\n");
        set_status(DASHBOARD);
        invalidate_dashboard_cache();
        menu_reset();
    }
}

/* RENDER — draws two rows. Skips if selection unchanged. */
void menu_render(void)
{
    if (selection == last_drawn) return;
    last_drawn = selection;

    clcd_putch('>',               LINE1(0));
    clcd_putch(' ',               LINE1(1));
    clcd_print(labels[selection], LINE1(2));

    clcd_putch(' ', LINE2(0));
    clcd_putch(' ', LINE2(1));
    if (selection < MENU_ITEMS - 1U)
        clcd_print(labels[selection + 1U], LINE2(2));
    else
        clcd_print("               ", LINE2(2));
}