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

unsigned char is_logged_in(void);   // 1 = authenticated session active
void          do_logout(void);      // Clears the logged-in flag

// Called once in init_config() — loads PIN from EEPROM into RAM
// Falls back to default "1111" if EEPROM is unprogrammed (reads 0xFF)
void          login_load_password(void);

// Called by set_password after a successful change — updates live PIN in RAM
void          login_set_password(const unsigned char *new_pass);

#endif /* LOGIN_H */