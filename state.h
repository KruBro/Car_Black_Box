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

void set_status(STATE new_status);
STATE get_status();
#endif