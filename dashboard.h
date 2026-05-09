/**
 * @file    dashboard.h
 * @author  krubro
 * @date    2026-05-09
 * @brief   Dashboard module — split into update (state) and render (display).
 *
 * update reads hardware, modifies sys. render reads sys, writes LCD.
 * Neither calls the other. main.c sequences them explicitly.
 */

#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "events.h"

/* Reads RTC + ADC, processes gear/crash events, updates sys. No LCD writes. */
void dashboard_update(EVENT evt);

/* Reads sys, writes LCD. No hardware reads, no state mutations. */
void dashboard_render(void);

/* Forces speed cache to invalidate so render redraws on next entry. */
void invalidate_dashboard_cache(void);

#endif /* DASHBOARD_H */