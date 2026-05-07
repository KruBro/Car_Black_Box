/**
 * @file    login.c
 * @author  krubro
 * @date    2026-05-05
 * @brief   4-digit PIN login screen with ISR-driven blink cursor, attempt limiting, and timeout.
 */

#include "main_config.h"

// Timing thresholds (hardware-accurate, independent of loop speed)
#define BLINK_THRESHOLD     20U      // blink_tick units:  10 * 10 ms  = 100 ms per toggle
#define TIMEOUT_THRESHOLD   50U    // timeout_tick units: 50 * 100 ms = 5 s idle timeout

static unsigned char  i                  = 0;
static unsigned char  user_pass[PASS_LENGTH + 1] = "1111";
static unsigned char  my_pass[PASS_LENGTH + 1];
static unsigned char  attempts_left      = MAX_ATTEMPTS;
static unsigned char  cursor_visible     = 0;
static unsigned char  login_successful   = 0;
static unsigned char  entry_cooldown     = 0;
static unsigned char  screen_initialized = 0;

/**
 * @brief  Minimal strcmp — avoids pulling in <string.h>.
 */
int my_strcmp(const char *s1, const char *s2)
{
    unsigned int index = 0;
    while (s1[index] != '\0' && s1[index] == s2[index])
        index++;
    return (unsigned char)s1[index] - (unsigned char)s2[index];
}

/**
 * @brief  Returns to DASHBOARD after TIMEOUT_THRESHOLD * 100 ms of idle.
 *         Uses hardware Timer1 tick — accurate regardless of loop speed.
 */
static void time_out(unsigned char key)
{
    if (key != ALL_RELEASED)
    {
        reset_timeout_tick(); // Any keypress resets the idle timer
        return;
    }

    if (timeout_tick >= TIMEOUT_THRESHOLD && login_successful == 0)
    {
        reset_timeout_tick();
        i                  = 0;
        entry_cooldown     = 0;
        screen_initialized = 0;
        invalidate_dashboard_cache();
        set_status(DASHBOARD);
    }
}

/**
 * @brief  Toggles the underscore cursor every BLINK_THRESHOLD * 10 ms.
 *         Uses hardware Timer0 tick — no busy-wait, no loop counter.
 */
static void handle_blinker(void)
{
    if (blink_tick < BLINK_THRESHOLD)
        return;

    reset_blink_tick();
    cursor_visible = !cursor_visible;

    if (i < PASS_LENGTH)
        clcd_putch(cursor_visible ? '_' : ' ', LINE2(i));
}

/**
 * @brief  Appends one digit ('0' via SW4, '1' via SW5) after a debounce cooldown.
 */
static void process_keypress(unsigned char key)
{
    if (entry_cooldown < 50) // Swallow inputs for the first 50 cycles after state entry
    {
        entry_cooldown++;
        return;
    }

    if (key == SW4 || key == SW5)
    {
        my_pass[i] = (key == SW4) ? '0' : '1';
        clcd_putch('*', LINE2(i));
        i++;

        reset_blink_tick();
        cursor_visible = 1;

        if (i < PASS_LENGTH)
            clcd_putch('_', LINE2(i)); // Advance cursor placeholder
    }
}

/**
 * @brief  Compares the entered PIN once PASS_LENGTH digits have been typed.
 */
static void verify_password(void)
{
    if (i != PASS_LENGTH)
        return;

    my_pass[PASS_LENGTH] = '\0';

    if (my_strcmp((const char *)user_pass, (const char *)my_pass) == 0)
    {
        login_successful = 1;
        return;
    }

    // Wrong PIN
    uart_puts("Invalid Entry\n");
    attempts_left--;

    clcd_clear();
    clcd_print("Invalid Entry   ", LINE1(0));
    clcd_putch(attempts_left + '0', LINE2(0));
    clcd_print(" Left!        ",   LINE2(1));
    __delay_ms(2000);

    i                  = 0;
    entry_cooldown     = 0;
    screen_initialized = 0;
    reset_blink_tick();
    reset_timeout_tick();
    clcd_clear();
}

/**
 * @brief  Renders the locked-out screen when all attempts are exhausted.
 */
static void handle_lockout(void)
{
    clcd_print("Device Locked   ", LINE1(0));
    clcd_print("Contact Admin   ", LINE2(0));
}

/**
 * @brief  Returns the remaining login attempt count.
 */
unsigned char get_attempts_left(void)
{
    return attempts_left;
}

/**
 * @brief  Main login screen handler — call every loop cycle while in LOGIN state.
 * @param[in] key  Current keypad reading from the main router.
 */
void show_login_screen(unsigned char key)
{
    if (attempts_left == 0)
    {
        handle_lockout();
        return;
    }

    // Draw static header exactly once per state entry
    if (screen_initialized == 0)
    {
        clcd_clear();
        clcd_print("PassWord:       ", LINE1(0));
        reset_blink_tick();
        reset_timeout_tick();
        screen_initialized = 1;
    }

    handle_blinker();
    process_keypress(key);
    verify_password();
    time_out(key);

    if (login_successful == 1)
    {
        clcd_clear();
        clcd_print("Correct Entry   ", LINE1(0));
        clcd_print("Access Granted  ", LINE2(0));
        __delay_ms(1000);

        uart_puts("Login Successful\n");
        login_successful   = 0;
        i                  = 0;
        entry_cooldown     = 0;
        screen_initialized = 0;
        reset_timeout_tick();
        set_status(MENU);
    }
}