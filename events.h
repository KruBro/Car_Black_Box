/**
 * @file    events.h
 * @author  krubro
 * @date    2026-05-09
 * @brief   Application event model. Translates raw hardware keycodes into
 *          semantic events before they enter any state-update function.
 *
 * Rule: every module that reacts to user input takes an EVENT, never a raw
 * keypad byte. The keypad is hardware; the EVENT is application logic.
 */

#ifndef EVENTS_H
#define EVENTS_H

typedef enum
{
    EVENT_NONE = 0,
    EVENT_SW1,          /* Dashboard: crash / Login: back / Menu: back         */
    EVENT_SW2,          /* Dashboard: gear up  / Menu: scroll down             */
    EVENT_SW3,          /* Dashboard: gear down / Menu: scroll up              */
    EVENT_SW4,          /* Dashboard: enter login / Login: digit '0' / select  */
    EVENT_SW5,          /* Login: digit '1'                                    */
    EVENT_SW6
} EVENT;

/* Converts a raw keypad byte (SW1-SW6 mask or ALL_RELEASED) to an EVENT. */
EVENT translate_key(unsigned char key);

#endif /* EVENTS_H */