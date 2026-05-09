/**
 * @file    events.c
 * @author  krubro
 * @date    2026-05-09
 * @brief   translate_key() — the single point where hardware meets application.
 */

#include "main_config.h"

EVENT translate_key(unsigned char key)
{
    switch (key)
    {
        case SW1: return EVENT_SW1;
        case SW2: return EVENT_SW2;
        case SW3: return EVENT_SW3;
        case SW4: return EVENT_SW4;
        case SW5: return EVENT_SW5;
        case SW6: return EVENT_SW6;
        default:  return EVENT_NONE;
    }
}