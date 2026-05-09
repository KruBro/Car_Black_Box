/**
 * @file state.h
 * @author krubro
 * @date 2026-05-09
 * @brief Application state machine — STATE enum and accessors.
 */

#ifndef STATE_H
#define STATE_H

typedef enum
{
    DASHBOARD,
    LOGIN,
    MENU,
    VIEW_LOGS,
    CLEAR_LOGS,
    DOWNLOAD_LOGS,
    SET_TIME,
    CHANGE_PASSWORD
} STATE;

void  set_status(STATE new_status);
STATE get_status(void);   /* FIX: was get_status() — empty list ≠ void in C90 */

#endif /* STATE_H */