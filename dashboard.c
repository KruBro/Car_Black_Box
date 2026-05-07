/**
 * @file    dashboard.c
 * @author  krubro
 * @date    2026-05-05
 * @brief   CLCD dashboard — time, gear/event state, and scaled speed.
 */

#include "main_config.h"

static unsigned short prev_speed = 0xFFFFU; // Sentinel: forces first draw

/**
 * @brief  Resets the speed cache so the dashboard redraws on next entry.
 */
void invalidate_dashboard_cache(void)
{
    prev_speed = 0xFFFFU;
}

/**
 * @brief  Reads HH:MM:SS from the DS1307 into a 3-byte BCD array.
 */
static void get_time(unsigned char *clock_reg)
{
    clock_reg[0] = ds1307_i2c_read(SEC_ADDRESS);
    clock_reg[1] = ds1307_i2c_read(MIN_ADDRESS);
    clock_reg[2] = ds1307_i2c_read(HOUR_ADDRESS);
}

/**
 * @brief  Renders "TIME" label on LINE1 and HH:MM:SS on LINE2.
 */
static void display_time(unsigned char *clock_reg)
{
    char time[9];

    clcd_print("TIME", LINE1(0));

    // Unpack BCD digits into ASCII characters
    time[0] = (char)(((clock_reg[2] >> 4) & 0x03U) + '0');
    time[1] = (char)((clock_reg[2] & 0x0FU) + '0');
    time[2] = ':';
    time[3] = (char)(((clock_reg[1] >> 4) & 0x07U) + '0');
    time[4] = (char)((clock_reg[1] & 0x0FU) + '0');
    time[5] = ':';
    time[6] = (char)(((clock_reg[0] >> 4) & 0x07U) + '0');
    time[7] = (char)((clock_reg[0] & 0x0FU) + '0');
    time[8] = '\0';

    clcd_print(time, LINE2(0));
}

/**
 * @brief  Handles gear/crash state updates and renders the event label.
 * @param[in] key  Current keypad reading (SWx mask or ALL_RELEASED).
 */
static void display_event(unsigned char key)
{
    const char * const event[] = {"GR", "GN", "G1", "G2", "G3", "G4"};
    static unsigned char gear_index    = 1; // Start in Neutral
    static unsigned char ignition_on   = 0;
    static unsigned char crash_detected = 0;

    clcd_print("EV", LINE1(10));

    // Update state — inputs are locked out after a crash
    if (key == SW1 && crash_detected == 0)
    {
        ignition_on    = 1;
        crash_detected = 1;
    }
    else if (key == SW2 && crash_detected == 0)
    {
        ignition_on = 1;
        if (gear_index < 5U) gear_index++;
    }
    else if (key == SW3 && crash_detected == 0)
    {
        ignition_on = 1;
        if (gear_index > 0U) gear_index--;
    }

    // Render state
    if (crash_detected == 1)
        clcd_print("C ", LINE2(10));
    else if (ignition_on == 1)
        clcd_print(event[gear_index], LINE2(10));
    else
        clcd_print("ON", LINE2(10));
}

/**
 * @brief  Converts a 0–100 speed value to a null-terminated ASCII string.
 */
void convert_adc_val_to_char(unsigned short reg_val, unsigned char *buff)
{
    buff[0] = (unsigned char)((reg_val / 100U) + '0');
    buff[1] = (unsigned char)(((reg_val / 10U) % 10U) + '0');
    buff[2] = (unsigned char)((reg_val % 10U) + '0');
    buff[3] = '\0';
}

/**
 * @brief  Samples the ADC, scales to 0–100, and redraws only on change.
 */
static void display_speed(void)
{
    static unsigned short curr_speed;
    unsigned char buff[4];

    clcd_print("SD", LINE1(13));

    curr_speed = (unsigned short)((unsigned long)read_adc(CHANNEL0) * 100UL / 1023UL);

    if (curr_speed != prev_speed) // Skip redraw if value unchanged
    {
        prev_speed = curr_speed;
        convert_adc_val_to_char(curr_speed, buff);
        clcd_print((const char *)buff, LINE2(13));
    }
}

/**
 * @brief  Composes and refreshes the full CLCD dashboard for one loop cycle.
 * @param[in] key  Current keypad reading passed from the main router.
 */
void clcd_dashboard(unsigned char key)
{
    unsigned char clock_reg[3];

    get_time(clock_reg);
    display_time(clock_reg);
    display_event(key);
    display_speed();
}