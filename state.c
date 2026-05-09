/**
 * @file    state.c
 * @author  krubro
 * @date    2026-05-09
 * @brief   Global state machine with UART transition logging.
 */

#include "main_config.h"

static STATE current_state = DASHBOARD;

/* Names match the STATE enum order in state.h. */
static const char * const state_names[] = {
    "DASHBOARD", "LOGIN", "MENU", "VIEW_LOGS",
    "CLEAR_LOGS", "DOWNLOAD_LOGS", "SET_TIME", "CHANGE_PASSWORD"
};

void set_status(STATE new_state)
{
    clcd_clear();
    uart_puts("[STATE] ");
    uart_puts(state_names[new_state]);
    uart_puts("\n");
    current_state = new_state;
}

STATE get_status(void)
{
    return current_state;
}