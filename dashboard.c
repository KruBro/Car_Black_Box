/**
 * @file    dashboard.c
 * @author  krubro
 * @date    2026-05-09
 * @brief   Dashboard — update (hardware reads + state) and render (LCD writes).
 *
 * Design rule enforced here:
 *   dashboard_update()  owns hardware access and sys mutation. Zero LCD writes.
 *   dashboard_render()  owns LCD writes. Zero hardware reads, zero sys mutation.
 *   main.c calls them in order: update → render.
 */

#include "main_config.h"

static const char * const gear_labels[] = {"GR", "GN", "G1", "G2", "G3", "G4"};

static unsigned char prev_speed = 0xFFU; /* Cache — 0xFF = uninitialized     */
static unsigned char prev_gear_render = 0xFFU;
static unsigned char prev_flags_render = 0xFFU;

void invalidate_dashboard_cache(void)
{
    prev_speed        = 0xFFU;
    prev_gear_render  = 0xFFU;
    prev_flags_render = 0xFFU;
}

/* =========================================================================
 * UPDATE — reads hardware, mutates sys. No LCD.
 * ========================================================================= */

void dashboard_update(EVENT evt)
{
    static unsigned char prev_flags_uart = 0;
    static GEAR_STATE    prev_gear_uart  = GR;

    /* Read time from RTC — always fresh. */
    sys.seconds = ds1307_i2c_read(SEC_ADDRESS);
    sys.minutes = ds1307_i2c_read(MIN_ADDRESS);
    sys.hours   = ds1307_i2c_read(HOUR_ADDRESS);

    /* Read speed from ADC — always fresh. */
    sys.speed = (unsigned char)((unsigned long)read_adc(CHANNEL0) * 100UL / 1023UL);

    /* Process gear and crash events — only while not crashed. */
    if (!(sys.flags & FLAG_CRASH))
    {
        if (evt == EVENT_SW1)
        {
            sys.flags |= FLAG_IGNITION_ON | FLAG_CRASH;
            sys.log_pending = 1;
        }
        else if (evt == EVENT_SW2 && sys.gear < G4)
        {
            sys.gear++;
            sys.flags |= FLAG_IGNITION_ON;
            sys.log_pending = 1;
        }
        else if (evt == EVENT_SW3 && sys.gear > GR)
        {
            sys.gear--;
            sys.flags |= FLAG_IGNITION_ON;
            sys.log_pending = 1;
        }
    }

    /* UART telemetry — only on change, not every cycle. */
    if ((sys.flags & FLAG_CRASH) && !(prev_flags_uart & FLAG_CRASH))
    {
        uart_puts("[DASH] CRASH\n");
    }
    else if (sys.gear != prev_gear_uart)
    {
        uart_puts("[DASH] GEAR: ");
        uart_puts(gear_labels[sys.gear]);
        uart_puts("\n");
    }
    else if ((sys.flags & FLAG_IGNITION_ON) && !(prev_flags_uart & FLAG_IGNITION_ON))
    {
        uart_puts("[DASH] IGNITION ON\n");
    }

    prev_flags_uart = sys.flags;
    prev_gear_uart  = sys.gear;
}

/* =========================================================================
 * RENDER — reads sys, writes LCD. No hardware reads, no sys mutation.
 * ========================================================================= */

void dashboard_render(void)
{
    char time_buf[9];
    unsigned char speed_buf[4];

    /* --- Time label (static text — drawn every frame, cheap) --- */
    clcd_print("TIME", LINE1(0));

    /* --- Time value (unpack BCD from sys) --- */
    time_buf[0] = (char)(((sys.hours   >> 4) & 0x03U) + '0');
    time_buf[1] = (char)((sys.hours    & 0x0FU) + '0');
    time_buf[2] = ':';
    time_buf[3] = (char)(((sys.minutes >> 4) & 0x07U) + '0');
    time_buf[4] = (char)((sys.minutes  & 0x0FU) + '0');
    time_buf[5] = ':';
    time_buf[6] = (char)(((sys.seconds >> 4) & 0x07U) + '0');
    time_buf[7] = (char)((sys.seconds  & 0x0FU) + '0');
    time_buf[8] = '\0';
    clcd_print(time_buf, LINE2(0));

    /* --- Event label (static text) --- */
    clcd_print("EV", LINE1(10));

    /* --- Gear/crash (redraw only when changed) --- */
    if (sys.gear != prev_gear_render || sys.flags != prev_flags_render)
    {
        prev_gear_render  = sys.gear;
        prev_flags_render = sys.flags;

        if (sys.flags & FLAG_CRASH)
            clcd_print("C ", LINE2(10));
        else if (sys.flags & FLAG_IGNITION_ON)
            clcd_print(gear_labels[sys.gear], LINE2(10));
        else
            clcd_print("ON", LINE2(10));
    }

    /* --- Speed label (static) --- */
    clcd_print("SD", LINE1(13));

    /* --- Speed value (redraw only when changed) --- */
    if (sys.speed != prev_speed)
    {
        prev_speed    = sys.speed;
        speed_buf[0]  = (unsigned char)((sys.speed / 100U) + '0');
        speed_buf[1]  = (unsigned char)(((sys.speed / 10U) % 10U) + '0');
        speed_buf[2]  = (unsigned char)((sys.speed % 10U) + '0');
        speed_buf[3]  = '\0';
        clcd_print((const char *)speed_buf, LINE2(13));
    }
}