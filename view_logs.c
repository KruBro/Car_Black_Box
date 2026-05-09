/**
 * @file    view_logs.c
 * @author  krubro
 * @date    2026-05-09
 * @brief   Log viewer — update/render split.
 *
 * update()  — processes navigation events, updates scroll index. No LCD.
 * render()  — reads EEPROM entry at current index, writes LCD.
 *
 * FIX (from previous session): gear bytes in EEPROM are raw GEAR_STATE indices
 * ('0'=GR, '1'=GN, '2'=G1 ... 'C'=crash). gear_label() translates them before
 * display. Without this, GN stored as '1' would print as "G1" — First Gear.
 *
 * CLCD layout:
 *   Row 1:  LOG 03/10
 *   Row 2:  HH:MM:SS GN 085
 *
 * UART download format:
 *   01 HH:MM:SS GN 085
 */

#include "main_config.h"

/* Maps the stored gear byte to its two-character display label.
 * This decode step is mandatory at every read site. */
static const char * gear_label(char g)
{
    switch (g)
    {
        case '0': return "GR";
        case '1': return "GN";
        case '2': return "G1";
        case '3': return "G2";
        case '4': return "G3";
        case '5': return "G4";
        case 'C': return "CR";
        default:  return "??";
    }
}

static void print_two_digits(unsigned char val, unsigned char addr)
{
    clcd_putch((char)((val / 10U) + '0'), addr);
    clcd_putch((char)((val % 10U) + '0'), addr + 1U);
}

/* =========================================================================
 * Private state
 * ========================================================================= */

static unsigned char log_index   = 0;
static unsigned char needs_draw  = 1;  /* 1 = render must refresh the entry  */
static unsigned char empty_shown = 0;

void view_logs_reset(void)
{
    log_index   = 0;
    needs_draw  = 1;
    empty_shown = 0;
}

/* =========================================================================
 * UPDATE — navigation events, no LCD.
 * ========================================================================= */

void view_logs_update(EVENT evt)
{
    unsigned char count = eeprom_get_entry_count();

    if (count == 0)
    {
        if (evt == EVENT_SW1)
        {
            uart_puts("[LOGS] BACK\n");
            view_logs_reset();
            set_status(MENU);
        }
        return;
    }

    if (evt == EVENT_SW2 && log_index < count - 1U)
    {
        log_index++;
        needs_draw = 1;
        uart_puts("[LOGS] NEXT\n");
    }
    else if (evt == EVENT_SW3 && log_index > 0U)
    {
        log_index--;
        needs_draw = 1;
        uart_puts("[LOGS] PREV\n");
    }
    else if (evt == EVENT_SW1)
    {
        uart_puts("[LOGS] BACK\n");
        view_logs_reset();
        set_status(MENU);
    }
}

/* =========================================================================
 * RENDER — reads EEPROM at log_index, writes LCD. Skips if !needs_draw.
 * ========================================================================= */

void view_logs_render(void)
{
    unsigned char count = eeprom_get_entry_count();
    char buf[12];
    const char *gl;

    if (count == 0)
    {
        if (!empty_shown)
        {
            clcd_clear();
            clcd_print("No Logs Found   ", LINE1(0));
            clcd_print("SW1: Back       ", LINE2(0));
            uart_puts("[LOGS] empty\n");
            empty_shown = 1;
        }
        return;
    }

    empty_shown = 0;

    if (!needs_draw) return;
    needs_draw = 0;

    eeprom_read_log(log_index, buf);  /* "HHMMSSGxSSS\0" */
    gl = gear_label(buf[7]);

    /* Row 1: LOG xx/xx */
    clcd_print("LOG ", LINE1(0));
    print_two_digits(log_index + 1U, LINE1(4));
    clcd_putch('/',                   LINE1(6));
    print_two_digits(count,           LINE1(7));
    clcd_print("       ",             LINE1(9));

    /* Row 2: HH:MM:SS GL SSS */
    clcd_putch(buf[0], LINE2(0));
    clcd_putch(buf[1], LINE2(1));
    clcd_putch(':',    LINE2(2));
    clcd_putch(buf[2], LINE2(3));
    clcd_putch(buf[3], LINE2(4));
    clcd_putch(':',    LINE2(5));
    clcd_putch(buf[4], LINE2(6));
    clcd_putch(buf[5], LINE2(7));
    clcd_putch(' ',    LINE2(8));
    clcd_putch(gl[0],  LINE2(9));
    clcd_putch(gl[1],  LINE2(10));
    clcd_putch(' ',    LINE2(11));
    clcd_putch(buf[8],  LINE2(12));
    clcd_putch(buf[9],  LINE2(13));
    clcd_putch(buf[10], LINE2(14));
    clcd_putch(' ',     LINE2(15));
}

/* =========================================================================
 * DOWNLOAD_LOGS — one-shot UART dump, auto-transitions to MENU.
 * ========================================================================= */

void download_logs(void)
{
    unsigned char count = eeprom_get_entry_count();
    unsigned char k;
    char buf[12];
    const char *gl;

    clcd_clear();

    if (count == 0)
    {
        clcd_print("No Logs Found   ", LINE1(0));
        uart_puts("[DOWNLOAD] No logs stored.\n");
    }
    else
    {
        clcd_print("Sending Logs... ", LINE1(0));
        print_two_digits(count, LINE2(0));
        clcd_print(" entries        ", LINE2(2));

        uart_puts("=== CAR BLACK BOX LOG ===\n");
        uart_puts("## HH:MM:SS GEAR SPD\n");

        for (k = 0; k < count; k++)
        {
            eeprom_read_log(k, buf);
            gl = gear_label(buf[7]);

            uart_putchar((char)((k + 1U) / 10U + '0'));
            uart_putchar((char)((k + 1U) % 10U + '0'));
            uart_putchar(' ');
            uart_putchar(buf[0]); uart_putchar(buf[1]);
            uart_putchar(':');
            uart_putchar(buf[2]); uart_putchar(buf[3]);
            uart_putchar(':');
            uart_putchar(buf[4]); uart_putchar(buf[5]);
            uart_putchar(' ');
            uart_putchar(gl[0]); uart_putchar(gl[1]);
            uart_putchar(' ');
            uart_putchar(buf[8]); uart_putchar(buf[9]); uart_putchar(buf[10]);
            uart_puts("\r\n");
        }
        uart_puts("=========================\n");
    }

    __delay_ms(1500);
    set_status(MENU);
}

/* =========================================================================
 * CLEAR_LOGS
 * ========================================================================= */

void clear_logs(void)
{
    clcd_clear();
    clcd_print("LOGS CLEARED    ", LINE1(0));
    uart_puts("[LOGS] CLEARED\n");
    __delay_ms(300);
    eeprom_clear_log();
}