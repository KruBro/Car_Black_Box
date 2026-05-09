/**
 * @file    login.h
 * @author  krubro
 * @date    2026-05-09
 */

#ifndef LOGIN_H
#define LOGIN_H

#include "events.h"

#define PASS_LENGTH     4

/*  Progressive lockout — fails before permanent lock:
 *    Fail 1 →  30 s cooldown
 *    Fail 2 →  45 s cooldown
 *    Fail 3 →  60 s cooldown
 *    Fail 4 → 120 s cooldown
 *    Fail 5 → Device Locked — Contact Admin (permanent)
 */
#define MAX_FAILS       5U

void          login_update(EVENT evt);
void          login_render(void);
void          login_reset(void);

unsigned char is_logged_in(void);   /* 1 = authenticated session active */
void          do_logout(void);      /* Clears the logged-in flag        */

#endif /* LOGIN_H */