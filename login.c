/**
 * @file    login.c
 * @author  krubro
 * @date    2026-05-09
 * @brief   4-digit PIN login — progressive lockout, logout flag, update/render split.
 *
 * Phases:
 *   PHASE_ENTERING  — waiting for 4 digits
 *   PHASE_FAIL      — wrong PIN — render shows feedback, then transitions to LOCKOUT
 *   PHASE_LOCKOUT   — timed cooldown before next attempt (countdown on CLCD)
 *   PHASE_SUCCESS   — correct PIN — render grants access, transitions to MENU
 *   PHASE_LOCKED    — all 5 attempts exhausted — permanent, Contact Admin
 *
 * Lockout durations (fail_count → seconds):
 *   1→30  2→45  3→60  4→120  5→permanent lock
 *
 * Timing:
 *   Cursor blink : BLINK_THRESHOLD * ~10 ms = ~200 ms  (Timer0)
 *   Idle timeout : TIMEOUT_THRESHOLD * ~100 ms = 5 s   (Timer1)
 *   Cooldown     : COOLDOWN_TICKS blink ticks  (~200 ms, Timer0-driven, input-independent)
 *   Lockout clock: timeout_tick / 10 = seconds elapsed (10 ticks × 100 ms = 1 s)
 */

#include "main_config.h"

#define BLINK_THRESHOLD     20U   /* ~200 ms per blink toggle        */
#define TIMEOUT_THRESHOLD   50U   /* 50 × 100 ms = 5 s idle timeout  */
#define COOLDOWN_TICKS       2U   /* 2 blink ticks = ~200 ms guard   */

typedef enum
{
    PHASE_ENTERING,
    PHASE_FAIL,
    PHASE_LOCKOUT,
    PHASE_SUCCESS,
    PHASE_LOCKED
} LOGIN_PHASE;

static const unsigned char lockout_durations[MAX_FAILS - 1U] = {0U, 15U, 30U, 60U};

static const unsigned char my_pass[PASS_LENGTH + 1] = "1111";

static LOGIN_PHASE   phase                  = PHASE_ENTERING;
static unsigned char fail_count             = 0;    /* Persists across resets — tracks lifetime fails */
static unsigned char digit_count            = 0;
static unsigned char entered[PASS_LENGTH + 1];
static unsigned char cursor_visible         = 0;
static unsigned char screen_ready           = 0;
static unsigned char cooldown_ticks         = 0;
static unsigned char lockout_secs_remaining = 0;    /* Updated in update(), read in render() */
static unsigned char logged_in_flag         = 0;

/* =========================================================================
 * Auth accessors
 * ========================================================================= */

unsigned char is_logged_in(void) { return logged_in_flag; }
void          do_logout(void)    { logged_in_flag = 0; uart_puts("[LOGIN] LOGGED OUT\r\n"); }

/* =========================================================================
 * Private helpers
 * ========================================================================= */

static int pass_match(void)
{
    unsigned char k;
    for (k = 0; k < PASS_LENGTH; k++)
        if (entered[k] != my_pass[k]) return 0;
    return 1;
}

/* =========================================================================
 * login_reset — call once on transition INTO LOGIN state
 * ========================================================================= */

void login_reset(void)
{
    digit_count    = 0;
    cursor_visible = 0;
    screen_ready   = 0;
    cooldown_ticks = 0;  /* Counts up via blink ticks — independent of keypresses */
    phase          = (fail_count >= MAX_FAILS) ? PHASE_LOCKED : PHASE_ENTERING;
    reset_blink_tick();
    reset_timeout_tick();
}

/* =========================================================================
 * UPDATE — advances phase from timer ticks + events. Zero LCD writes.
 * ========================================================================= */

void login_update(EVENT evt)
{
    if (phase == PHASE_LOCKED || phase == PHASE_SUCCESS)
        return;

    /* Cursor blink + cooldown counter — Timer0, independent of input */
    if (blink_tick >= BLINK_THRESHOLD)
    {
        reset_blink_tick();
        cursor_visible = !cursor_visible;
        if (cooldown_ticks < COOLDOWN_TICKS) cooldown_ticks++;
    }

    /* Lockout countdown — uses timeout_tick / 10 as a seconds counter */
    if (phase == PHASE_LOCKOUT)
    {
        unsigned char elapsed = (unsigned char)(timeout_tick / 10U);
        unsigned char dur     = lockout_durations[fail_count - 1U];

        lockout_secs_remaining = (elapsed < dur) ? (unsigned char)(dur - elapsed) : 0U;

        if (lockout_secs_remaining == 0U)
        {
            uart_puts("[LOGIN] LOCKOUT EXPIRED\r\n");
            phase          = PHASE_ENTERING;
            screen_ready   = 0;
            cooldown_ticks = COOLDOWN_TICKS;  /* Skip cooldown — user already waited */
            reset_blink_tick();
            reset_timeout_tick();
        }
        return;
    }

    /* Idle timeout — only in PHASE_ENTERING */
    if (evt == EVENT_NONE && timeout_tick >= TIMEOUT_THRESHOLD)
    {
        uart_puts("[LOGIN] TIMEOUT\r\n");
        reset_timeout_tick();
        login_reset();
        invalidate_dashboard_cache();
        set_status(DASHBOARD);
        return;
    }

    /* Block digit input for ~200 ms after state entry (swallows triggering keypress) */
    if (cooldown_ticks < COOLDOWN_TICKS) return;

    /* Digit entry */
    if (digit_count < PASS_LENGTH && (evt == EVENT_SW4 || evt == EVENT_SW5))
    {
        entered[digit_count] = (evt == EVENT_SW4) ? '0' : '1';
        digit_count++;
        reset_blink_tick();
        reset_timeout_tick();
        cursor_visible = 1;

        uart_puts("[LOGIN] digit ");
        uart_putchar((char)('0' + digit_count));
        uart_puts("\r\n");
    }

    /* Verify on 4th digit */
    if (digit_count == PASS_LENGTH)
    {
        if (pass_match())
        {
            uart_puts("[LOGIN] CORRECT\r\n");
            phase = PHASE_SUCCESS;
        }
        else
        {
            fail_count++;
            uart_puts("[LOGIN] FAIL ");
            uart_putchar((char)('0' + fail_count));
            uart_puts("/");
            uart_putchar((char)('0' + MAX_FAILS));
            uart_puts("\r\n");
            phase = PHASE_FAIL;
        }
    }
}

/* =========================================================================
 * RENDER — writes LCD from phase + statics. No hardware reads.
 *          One-shot frames (FAIL, SUCCESS) advance the phase after display.
 * ========================================================================= */

void login_render(void)
{
    unsigned char k;

    /* Permanent lock */
    if (phase == PHASE_LOCKED)
    {
        clcd_print("Device Locked   ", LINE1(0));
        clcd_print("Contact Admin   ", LINE2(0));
        return;
    }

    /* Access granted */
    if (phase == PHASE_SUCCESS)
    {
        clcd_clear();
        clcd_print("Correct Entry   ", LINE1(0));
        clcd_print("Access Granted  ", LINE2(0));
        __delay_ms(1000);
        uart_puts("[LOGIN] ACCESS GRANTED\r\n");
        logged_in_flag = 1;
        lockout_secs_remaining = lockout_durations[0];
        fail_count = 0;
        login_reset();
        set_status(MENU);
        return;
    }

    /* Wrong PIN feedback — fires once, then transitions to LOCKOUT or LOCKED */
    if (phase == PHASE_FAIL)
    {
        unsigned char tries_left = (unsigned char)(MAX_FAILS - fail_count);

        clcd_clear();
        clcd_print("Invalid Entry   ", LINE1(0));

        if (fail_count >= MAX_FAILS)
        {
            clcd_print("Device Locked!  ", LINE2(0));
            __delay_ms(2000);
            phase = PHASE_LOCKED;
        }
        else
        {
            clcd_putch((char)('0' + tries_left), LINE2(0));
            clcd_print(" tries left     ", LINE2(1));
            __delay_ms(1500);

            reset_timeout_tick();                       /* Lockout clock starts here */
            lockout_secs_remaining = lockout_durations[fail_count - 1U];
            phase        = PHASE_LOCKOUT;
            screen_ready = 0;
            digit_count  = 0;
        }
        clcd_clear();
        return;
    }

    /* Timed lockout countdown */
    if (phase == PHASE_LOCKOUT)
    {
        static unsigned char last_shown = 0xFFU;  /* Sentinel — forces first draw */

        if (!screen_ready)
        {
            clcd_clear();
            clcd_print("  Locked!       ", LINE1(0));
            screen_ready = 1;
            last_shown   = 0xFFU;
        }

        if (lockout_secs_remaining != last_shown)     /* Redraw only when seconds change */
        {
            last_shown = lockout_secs_remaining;
            clcd_print("Retry in: ", LINE2(0));
            clcd_putch((char)(lockout_secs_remaining / 100U + '0'),            LINE2(10));
            clcd_putch((char)((lockout_secs_remaining / 10U) % 10U + '0'),     LINE2(11));
            clcd_putch((char)(lockout_secs_remaining % 10U + '0'),             LINE2(12));
            clcd_putch('s',                                                     LINE2(13));
            clcd_print("  ", LINE2(14));
        }
        return;
    }

    /* PHASE_ENTERING — password prompt, drawn once per entry */
    if (!screen_ready)
    {
        clcd_clear();
        clcd_print("PassWord:       ", LINE1(0));
        for (k = 0; k < PASS_LENGTH; k++) clcd_putch(' ', LINE2(k));
        screen_ready = 1;
    }

    for (k = 0; k < digit_count; k++) clcd_putch('*', LINE2(k));

    if (digit_count < PASS_LENGTH)
        clcd_putch(cursor_visible ? '_' : ' ', LINE2(digit_count));
}
