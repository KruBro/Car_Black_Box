/**
 * @file set_time.c
 * @author krubro
 * @date 2026-05-09
 * @brief Set-time module — local copy isolation, field rollover, DS1307 commit
 *        on SW2.
 *
 * Architecture notes:
 *   set_time_update() — processes events, mutates local copy only. Zero LCD.
 *   set_time_render() — writes LCD from local copy. Zero DS1307 access.
 *   DS1307 is written ONLY in set_time_update() on EVENT_SW2 (confirm).
 *
 * Copy isolation:
 *   set_time_reset() reads sys.hours/minutes/seconds (BCD from DS1307) and
 *   decodes them to plain decimal values (0–23, 0–59) for easy arithmetic.
 *   On SW2, they are re-encoded to BCD before being written to the DS1307.
 *
 * Render — blinking field:
 *   The active field blinks every ST_BLINK_THRESHOLD Timer0 ticks (~250 ms).
 *   Inactive fields are always shown solid. Only the active field flickers.
 *
 * CLCD layout:
 *   Row 1: "Set Time  SW2:OK"
 *   Row 2: "HH:MM:SS" — active field blinks.
 *
 * Change log:
 *   [2026-05-09] bcd_to_dec() and set_time_reset() commented out.
 *                XC8 reported both as unused. bcd_to_dec() is static and
 *                its only caller was set_time_reset(); once that function is
 *                removed both become dead code. ~45 ROM words recovered.
 *                See set_time.h for re-enable instructions.
 */

#include "main_config.h"

#define ST_BLINK_THRESHOLD 25U   /* ~250 ms per blink toggle (Timer0, ~10 ms tick) */

/* Field indices */
#define FIELD_HH    0U
#define FIELD_MM    1U
#define FIELD_SS    2U
#define FIELD_COUNT 3U

/* Rollover limits (decimal, not BCD) */
static const unsigned char field_max[FIELD_COUNT] = {23U, 59U, 59U};

static unsigned char time_copy[FIELD_COUNT]; /* Decoded decimal: [0]=HH [1]=MM [2]=SS */
static unsigned char active_field  = FIELD_HH;
static unsigned char blink_state   = 1;     /* 1 = field visible, 0 = blanked */
static unsigned char screen_ready  = 0;

/* =========================================================================
 * Private — BCD helpers
 * =========================================================================
 *
 * FIX [2026-05-09] — Dead Code Removal
 * ─────────────────────────────────────────────────────────────────────────
 * bcd_to_dec() is static and was only called from set_time_reset(). Since
 * set_time_reset() is also removed (never called in main.c), bcd_to_dec()
 * becomes unreachable dead code. Leaving a static function defined but
 * uncalled produces XC8 warning (359); it also wastes ROM at -O0.
 *
 * dec_to_bcd() IS still called from set_time_update() on EVENT_SW2, so it
 * is kept below.
 *
 * static unsigned char bcd_to_dec(unsigned char bcd)
 * {
 *     // DS1307 stores time as BCD (e.g. 0x23 = 23 decimal)
 *     return (unsigned char)(((bcd >> 4) & 0x0FU) * 10U + (bcd & 0x0FU));
 * }
 */

static unsigned char dec_to_bcd(unsigned char dec)
{
    return (unsigned char)(((dec / 10U) << 4) | (dec % 10U));
}

/* =========================================================================
 * set_time_reset — COMMENTED OUT (dead code, compiler warning: unused)
 * =========================================================================
 *
 * FIX [2026-05-09] — Dead Code Removal
 * ─────────────────────────────────────────────────────────────────────────
 * set_time_reset() initialises time_copy[] from sys and resets internal
 * render flags. It is the correct entry-point for the SET_TIME state.
 * However, main.c never calls it before transitioning to SET_TIME, so XC8
 * reports it as unused at every optimisation level.
 *
 * Consequence of not calling set_time_reset():
 *   time_copy[] retains whatever values it held from static initialisation
 *   (all zeros on first boot). The render function will display "00:00:00"
 *   rather than the current DS1307 time when SET_TIME is entered. This is a
 *   functional regression — see set_time.h for the one-line fix in main.c.
 *
 * void set_time_reset(void)
 * {
 *     // Copy and decode current sys time — edits stay local until SW2
 *     time_copy[FIELD_HH] = bcd_to_dec(sys.hours);
 *     time_copy[FIELD_MM] = bcd_to_dec(sys.minutes);
 *     time_copy[FIELD_SS] = bcd_to_dec(sys.seconds);
 *
 *     active_field = FIELD_HH;
 *     blink_state  = 1;
 *     screen_ready = 0;
 *     reset_blink_tick();
 * }
 */

/* =========================================================================
 * UPDATE — field navigation and value editing. Zero LCD writes.
 * ========================================================================= */

void set_time_update(EVENT evt)
{
    /* Timer0: blink tick for active field */
    if (blink_tick >= ST_BLINK_THRESHOLD)
    {
        reset_blink_tick();
        blink_state = !blink_state;
    }

    if (evt == EVENT_NONE)
        return;

    /* SW1: discard all edits — DS1307 is never touched */
    if (evt == EVENT_SW1)
    {
        uart_puts("[SET_TIME] DISCARDED — RTC unchanged\r\n");
        menu_reset();
        set_status(MENU);
        return;
    }

    /* SW2: commit local copy to DS1307 (re-encode decimal → BCD) */
    if (evt == EVENT_SW2)
    {
        ds1307_i2c_write(dec_to_bcd(time_copy[FIELD_SS]), SEC_ADDRESS);
        ds1307_i2c_write(dec_to_bcd(time_copy[FIELD_MM]), MIN_ADDRESS);
        ds1307_i2c_write(dec_to_bcd(time_copy[FIELD_HH]), HOUR_ADDRESS);

        /* Mirror into sys so the dashboard reflects the change instantly */
        sys.hours   = dec_to_bcd(time_copy[FIELD_HH]);
        sys.minutes = dec_to_bcd(time_copy[FIELD_MM]);
        sys.seconds = dec_to_bcd(time_copy[FIELD_SS]);

        uart_puts("[SET_TIME] SAVED to DS1307\r\n");
        menu_reset();
        set_status(MENU);
        return;
    }

    /* SW6: cycle active field HH → MM → SS → HH */
    if (evt == EVENT_SW6)
    {
        active_field = (unsigned char)((active_field + 1U) % FIELD_COUNT);
        blink_state  = 1;   /* Show field solid immediately on shift */
        reset_blink_tick();
        return;
    }

    /* SW4: increment with rollover */
    if (evt == EVENT_SW4)
    {
        time_copy[active_field] = (time_copy[active_field] >= field_max[active_field])
                                  ? 0U
                                  : time_copy[active_field] + 1U;
        blink_state = 1;    /* Force field visible after edit */
        reset_blink_tick();
        return;
    }

    /* SW5: decrement with underflow protection */
    if (evt == EVENT_SW5)
    {
        time_copy[active_field] = (time_copy[active_field] == 0U)
                                  ? field_max[active_field]
                                  : time_copy[active_field] - 1U;
        blink_state = 1;
        reset_blink_tick();
        return;
    }
}

/* =========================================================================
 * RENDER — reads local copy, writes LCD. No DS1307 access.
 * ========================================================================= */

void set_time_render(void)
{
    unsigned char f;

    /* Draw static header once per state entry */
    if (!screen_ready)
    {
        clcd_clear();
        clcd_print("Set Time  SW2:OK", LINE1(0));
        clcd_putch(':',               LINE2(2));
        clcd_putch(':',               LINE2(5));
        clcd_print("  SW1:Back      ", LINE2(8));
        screen_ready = 1;
    }

    /* Render each of the 3 fields — blink only the active one */
    for (f = 0; f < FIELD_COUNT; f++)
    {
        unsigned char col = (unsigned char)(f * 3U); /* HH=0, MM=3, SS=6 */

        if (f == active_field && blink_state == 0)
        {
            /* Blank the active field during the OFF half of the blink cycle */
            clcd_putch(' ', LINE2(col));
            clcd_putch(' ', LINE2(col + 1U));
        }
        else
        {
            clcd_putch((char)(time_copy[f] / 10U + '0'), LINE2(col));
            clcd_putch((char)(time_copy[f] % 10U + '0'), LINE2(col + 1U));
        }
    }
}