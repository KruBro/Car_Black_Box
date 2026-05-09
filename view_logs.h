/**
 * @file    view_logs.h
 * @author  krubro
 * @date    2026-05-09
 */

#ifndef VIEW_LOGS_H
#define VIEW_LOGS_H

#include "events.h"

void view_logs_update(EVENT evt);
void view_logs_render(void);
void view_logs_reset(void);

void download_logs(void);   /* Runs once, dumps UART, auto-transitions to MENU. */
void clear_logs(void);

#endif /* VIEW_LOGS_H */