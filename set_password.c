/**
 * @file    set_password.c
 * @author  krubro
 * @date    2026-05-09
 * @brief   Change-password module — dual-buffer PIN entry, EEPROM persistence.
 *
 * Architecture notes:
 *   set_password_update()  — processes timer ticks and events, mutates phase. Zero LCD writes.
 *   set_password_render()  — writes LCD from phase. Zero hardware reads, zero state mutation
 *                            except for one-shot transitions (MATCH, MISMATCH) which advance
 *                            the phase after the feedback frame is displayed.
 *
 * Password storage:
 *   The new PIN is written to EEPROM at EEPROM_PASS_ADDR (4 bytes).
 *   login_set_password() is then called so the running login module adopts
 *   the new password immediately without a reboot.
 *   On next power-on, login_load_password() reads EEPROM_PASS_ADDR back into RAM.
 *
 * Timing source:
 *   blink_tick  (Timer0, ~10 ms) — cursor toggle and entry cooldown.
 *   timeout_tick (Timer1, ~100 ms) — 10 s inactivity timeout.
 *   Both counters are reset in set_password_reset() so they start fresh each entry.
 */

#include "main_config.h"

#define CP_BLINK_THRESHOLD  20U     // ~200 ms per blink toggle (same cadence as login)
#define CP_TIMEOUT_TICKS   100U     // 100 × 100 ms = 10 s inactivity timeout
#define CP_COOLDOWN_TICKS    2U     // 2 blink ticks = ~200 ms — swallows entry keypress

typedef enum
{
    CP_ENTER_NEW,   // Waiting for the first 4-digit PIN
    CP_RE_ENTER,    // Waiting for the confirmation PIN
    CP_MATCH,       // Both PINs match — render writes EEPROM, then → MENU
    CP_MISMATCH     // PINs differ — render shows feedback, then resets to CP_ENTER_NEW
} CP_PHASE;

static CP_PHASE      phase          = CP_ENTER_NEW;
static unsigned char new_pass[PASS_LENGTH];     // First entry buffer
static unsigned char re_pass[PASS_LENGTH];      // Re-entry confirmation buffer
static unsigned char digit_count    = 0;
static unsigned char cursor_visible = 1;
static unsigned char screen_ready  = 0;
static unsigned char cooldown_ticks = 0;

/* =========================================================================
 * set_password_reset — call once on transition INTO CHANGE_PASSWORD state
 * ========================================================================= */

void set_password_reset(void)
{
    phase          = CP_ENTER_NEW;
    digit_count    = 0;
    cursor_visible = 1;
    screen_ready   = 0;
    cooldown_ticks = 0;         // Block input for ~200 ms to swallow the menu SW4 press
    reset_blink_tick();
    reset_timeout_tick();
}

/* =========================================================================
 * UPDATE — advances phase from timer ticks and events. Zero LCD writes.
 * ========================================================================= */

void set_password_update(EVENT evt)
{
    // One-shot phases are consumed entirely inside render — nothing to do here
    if (phase == CP_MATCH || phase == CP_MISMATCH)
        return;

    // Timer0: cursor blink and entry cooldown counter
    if (blink_tick >= CP_BLINK_THRESHOLD)
    {
        reset_blink_tick();
        cursor_visible = !cursor_visible;
        if (cooldown_ticks < CP_COOLDOWN_TICKS) cooldown_ticks++;
    }

    // Timer1: 10 s inactivity timeout — back to MENU on idle
    if (evt == EVENT_NONE && timeout_tick >= CP_TIMEOUT_TICKS)
    {
        uart_puts("[CP] TIMEOUT — returning to MENU\r\n");
        menu_reset();
        set_status(MENU);
        return;
    }

    // Any input resets the idle timer
    if (evt != EVENT_NONE)
        reset_timeout_tick();

    // SW1: discard and return to MENU at any entry stage
    if (evt == EVENT_SW1)
    {
        uart_puts("[CP] CANCELLED\r\n");
        menu_reset();
        set_status(MENU);
        return;
    }

    // Block digit input for the first ~200 ms (swallows the SW4 that entered this state)
    if (cooldown_ticks < CP_COOLDOWN_TICKS)
        return;

    // Digit entry — SW4 = '0', SW5 = '1'
    if (digit_count < PASS_LENGTH && (evt == EVENT_SW4 || evt == EVENT_SW5))
    {
        unsigned char val = (evt == EVENT_SW4) ? '0' : '1';

        if (phase == CP_ENTER_NEW)
            new_pass[digit_count] = val;
        else
            re_pass[digit_count] = val;

        digit_count++;
        cursor_visible = 1;
        reset_blink_tick();

        uart_puts("[CP] digit ");
        uart_putchar((char)('0' + digit_count));
        uart_puts("\r\n");
    }

    // Transition on 4th digit
    if (digit_count == PASS_LENGTH)
    {
        if (phase == CP_ENTER_NEW)
        {
            uart_puts("[CP] first entry complete — awaiting re-entry\r\n");
            phase        = CP_RE_ENTER;
            digit_count  = 0;
            screen_ready = 0;       // Force redraw for the re-entry prompt
            cursor_visible = 1;
            reset_blink_tick();
        }
        else
        {
            // Compare the two buffers
            unsigned char k;
            unsigned char match = 1;
            for (k = 0; k < PASS_LENGTH; k++)
            {
                if (new_pass[k] != re_pass[k]) { match = 0; break; }
            }

            phase = match ? CP_MATCH : CP_MISMATCH;

            if (match)
                uart_puts("[CP] MATCH — writing to EEPROM\r\n");
            else
                uart_puts("[CP] MISMATCH — resetting\r\n");
        }
    }
}

/* =========================================================================
 * RENDER — writes LCD from phase. No hardware reads.
 *          One-shot frames (MATCH, MISMATCH) advance the phase after display.
 * ========================================================================= */

void set_password_render(void)
{
    unsigned char k;

    // ── MATCH: write EEPROM, update live login, transition to MENU ─────────
    if (phase == CP_MATCH)
    {
        clcd_clear();
        clcd_print("PASS CHANGED    ", LINE1(0));
        clcd_print("Saving...       ", LINE2(0));

        // Persist new PIN to EEPROM — 4 bytes starting at EEPROM_PASS_ADDR
        for (k = 0; k < PASS_LENGTH; k++)
            eeprom_write_byte((unsigned int)(EEPROM_PASS_ADDR + k), new_pass[k]);

        // Update the running login module immediately (no reboot required)
        login_set_password(new_pass);

        uart_puts("[CP] EEPROM write complete\r\n");
        __delay_ms(1500);

        menu_reset();
        set_status(MENU);
        return;
    }

    // ── MISMATCH: show feedback, reset to CP_ENTER_NEW ────────────────────
    if (phase == CP_MISMATCH)
    {
        clcd_clear();
        clcd_print("MISMATCH!       ", LINE1(0));
        clcd_print("Try Again       ", LINE2(0));
        __delay_ms(1500);

        // Full reset — user starts from scratch with a new first entry
        set_password_reset();
        return;
    }

    // ── CP_ENTER_NEW / CP_RE_ENTER: PIN entry prompt ───────────────────────

    // Draw static header once per phase entry
    if (!screen_ready)
    {
        clcd_clear();
        clcd_print(phase == CP_ENTER_NEW ? "New Pass:       "
                                         : "Re-enter Pass:  ", LINE1(0));
        for (k = 0; k < PASS_LENGTH; k++) clcd_putch(' ', LINE2(k));
        screen_ready = 1;
    }

    // Draw confirmed digits as asterisks
    for (k = 0; k < digit_count; k++)
        clcd_putch('*', LINE2(k));

    // Draw blinking cursor at the active position
    if (digit_count < PASS_LENGTH)
        clcd_putch(cursor_visible ? '_' : ' ', LINE2(digit_count));
}