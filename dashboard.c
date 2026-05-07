/**
 * @file    dashboard.c
 * @author  krubro
 * @date    2026-05-05
 * @brief   CLCD dashboard: composes time, gear/event state, and speed
 *          onto the 16×2 display each main loop iteration.
 *
 * @details
 *   Display layout (16 columns × 2 rows):
 *
 *     Col:  0123456789012345
 *     ROW1: TIME      EV SD
 *     ROW2: HH:MM:SS  GN 075
 *
 *   Columns 0–7   → timestamp (TIME label + HH:MM:SS)
 *   Columns 10–11 → event / gear state (EV label + state string)
 *   Columns 13–15 → speed (SD label + 3-digit value 000–100)
 *
 *   BUG FIXED: The gear-state array index `i` was declared as `static char`
 *   (signed). Decrementing below 0 would produce -1 and cause an out-of-bounds
 *   array read. Changed to `static unsigned char` — the lower-bound guard
 *   `if (i > 0) i--` is now safe on an unsigned type.
 */

#include "main_config.h"

/* =========================================================================
 * Private Helper: Time
 * ========================================================================= */

/**
 * @brief   Reads the current HH:MM:SS from the DS1307 into a BCD buffer.
 * @param[out] clock_reg  Three-element array: [0]=sec, [1]=min, [2]=hour (BCD).
 */

static void get_time(unsigned char *clock_reg)
{
    clock_reg[0] = ds1307_i2c_read(SEC_ADDRESS);
    clock_reg[1] = ds1307_i2c_read(MIN_ADDRESS);
    clock_reg[2] = ds1307_i2c_read(HOUR_ADDRESS);
}

/**
 * @brief   Formats a BCD time buffer and prints it to the CLCD.
 * @details BCD nibble extraction:
 *            Hours   bits [5:4] → tens, [3:0] → units
 *            Minutes bits [6:4] → tens, [3:0] → units
 *            Seconds bits [6:4] → tens, [3:0] → units
 * @param[in] clock_reg  BCD time buffer from get_time().
 */

static void display_time(unsigned char *clock_reg)
{
    char time[9];   /* "HH:MM:SS" + null terminator                         */

    clcd_print("TIME", LINE1(0));

    /* Hours — bits [5:4] = tens digit, [3:0] = units digit                 */
    time[0] = (char)(((clock_reg[2] >> 4) & 0x03U) + '0');
    time[1] = (char)((clock_reg[2] & 0x0FU) + '0');
    time[2] = ':';

    /* Minutes — bits [6:4] = tens digit, [3:0] = units digit               */
    time[3] = (char)(((clock_reg[1] >> 4) & 0x07U) + '0');
    time[4] = (char)((clock_reg[1] & 0x0FU) + '0');
    time[5] = ':';

    /* Seconds — bits [6:4] = tens digit, [3:0] = units digit               */
    time[6] = (char)(((clock_reg[0] >> 4) & 0x07U) + '0');
    time[7] = (char)((clock_reg[0] & 0x0FU) + '0');
    time[8] = '\0';

    clcd_print(time, LINE2(0));
}

/* =========================================================================
 * Private Helper: Event / Gear State
 * ========================================================================= */

/**
 * @brief   Reads keypad input and updates the gear/event state on the CLCD.
 *
 *   Gear state array (index → display string):
 *     0 = "GR" (Reverse)   1 = "GN" (Neutral)
 *     2 = "G1"             3 = "G2"
 *     4 = "G3"             5 = "G4"
 *
 *   SW1 — Crash detected: sets crash_detected latch, prints "C " permanently.
 *   SW2 — Gear up   (increment index, max 5).
 *   SW3 — Gear down (decrement index, min 0).
 *
 *   BUG FIX: Changed `static char i` to `static unsigned char i`.
 *     char is signed on XC8; if i==0 and SW3 fires, `i--` wraps to 255,
 *     causing an out-of-bounds read into the event[] array.
 */

static void display_event(void)
{
    /* Gear state labels indexed 0–5                                         */
    const char * const event[] = {"GR", "GN", "G1", "G2", "G3", "G4"};

    /* FIX: unsigned char prevents wrap-around below 0                      */
    static unsigned char gear_index   = 0;  /* Start in Neutral (index 1)   */
    static unsigned char ignition_on  = 0;
    static unsigned char crash_detected = 0;

    clcd_print("EV", LINE1(10));

    unsigned char key = read_digital_keypad(EDGE);

    if (key == SW1 && crash_detected == 0)
    {
        ignition_on     = 1;
        crash_detected  = 1;
        clcd_print("C ", LINE2(10));    /* Latch crash indicator             */
    }
    else if (key == SW2 && crash_detected == 0)
    {
        ignition_on = 1;
        if (gear_index < 5U)
            gear_index++;               /* Gear up, clamp at G4 (index 5)   */
    }
    else if (key == SW3 && crash_detected == 0)
    {
        ignition_on = 1;
        if (gear_index > 0U)
            gear_index--;               /* Gear down, clamp at GR (index 0) */
    }

    if (ignition_on == 1 && crash_detected == 0)
    {
        clcd_print(event[gear_index], LINE2(10));
    }
    else if (ignition_on == 0)
    {
        clcd_print("ON", LINE2(10));    /* Display "ON" before first keypress */
    }
    /* If crash_detected == 1, "C " was already printed and is latched on   */
}

/* =========================================================================
 * Private Helper: Speed
 * ========================================================================= */

/**
 * @brief   Converts an ADC percentage value (0–100) to a 3-char ASCII string.
 * @param[in]  reg_val  Speed value in the range 0–100.
 * @param[out] buff     4-byte buffer; receives "000"–"100" + null terminator.
 */

void convert_adc_val_to_char(unsigned short reg_val, unsigned char *buff)
{
    buff[0] = (unsigned char)((reg_val / 100U) + '0');          /* Hundreds */
    buff[1] = (unsigned char)(((reg_val / 10U) % 10U) + '0');  /* Tens     */
    buff[2] = (unsigned char)((reg_val % 10U) + '0');           /* Ones     */
    buff[3] = '\0';
}

/**
 * @brief   Reads the ADC, scales to 0–100, and prints the speed to the CLCD.
 * @details Only updates the display when the value changes (reduces flicker).
 *          Scale factor: (ADC_max=1023) / 10.23 ≈ 100.0 %
 */
static void display_speed(void)
{
    static unsigned short curr_speed;
    static unsigned short prev_speed = 0xFFFFU; /* Force update on first call */
    unsigned char buff[4];

    clcd_print("SD", LINE1(13));

    curr_speed = (unsigned short)((unsigned long)read_adc(CHANNEL0) * 100UL / 1023UL);

    if (curr_speed != prev_speed)
    {
        prev_speed = curr_speed;
        convert_adc_val_to_char(curr_speed, buff);
        clcd_print((const char *)buff, LINE2(13));
    }
}

/* =========================================================================
 * Public: Dashboard Entry Point
 * ========================================================================= */

/**
 * @brief   Refreshes the full CLCD dashboard (time, event, speed).
 * @details Called once per main loop iteration. Internally calls each
 *          sub-display handler in sequence.
 */
void clcd_dashboard(void)
{
    unsigned char clock_reg[3];

    get_time(clock_reg);
    display_time(clock_reg);
    display_event();
    display_speed();
}