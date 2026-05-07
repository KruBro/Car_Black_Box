/**
 * @file    state.c
 * @author  krubro
 * @date    2026-05-05
 * @brief   Global state machine — tracks and transitions the active application screen.
 */

#include "main_config.h"

static STATE current_state = DASHBOARD;

/**
 * @brief  Clears the LCD and transitions to a new application state.
 */
void set_status(STATE new_state)
{
    clcd_clear();
    current_state = new_state;
}

/**
 * @brief  Returns the currently active application state.
 */
STATE get_status(void)
{
    return current_state;
}