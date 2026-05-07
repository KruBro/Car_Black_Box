/**
 * @file    digital_keypad.c
 * @author  krubro
 * @date    2026-05-05
 * @brief   6-switch active-low digital keypad driver implementation.
 *
 * @details
 *   EDGE mode algorithm:
 *     - `once` acts as a "consumed" flag. It starts at 1 (ready to fire).
 *     - On a key press: if once==1, debounce 500 iterations, re-read. If
 *       still pressed, set once=0 (consume) and return the key state.
 *     - While any key is held, once==0, so subsequent calls return ALL_RELEASED.
 *     - When all keys are released, once is reset to 1, ready for next press.
 *
 *   Note on the debounce loop:
 *     `for(unsigned int i = 0; i < 500; i++);`
 *     This is a short busy-wait and is sufficient for debounce given the 20 MHz
 *     clock (~25 µs). A hardware timer-based debounce would be more robust for
 *     production use, but is acceptable here.
 */

#include "digital_keypad.h"
#include <xc.h>

/* =========================================================================
 * Public Function Implementations
 * ========================================================================= */

/**
 * @brief   Initialises PORTB as an input port for the keypad.
 */
void init_digital_keypad(void)
{
    TRISB       = 0xFF;     /* All PORTB pins: input                         */
    KEYPAD_PORT = 0xFF;     /* Drive data latch high (enables weak pull-ups) */
}

/**
 * @brief   Reads the keypad and returns the masked PORTB value.
 *
 *   LEVEL mode: Returns (PORTB & ALL_RELEASED) immediately every call.
 *   EDGE  mode: Returns the key state exactly once per physical press.
 *               After firing, returns ALL_RELEASED until the key is released.
 */
unsigned char read_digital_keypad(unsigned char trigger_method)
{
    static unsigned char once = 1; /* 1 = ready to fire; 0 = edge consumed  */

    if (trigger_method == LEVEL)
    {
        return (unsigned char)(KEYPAD_PORT & ALL_RELEASED);
    }
    else if (trigger_method == EDGE)
    {
        if ((KEYPAD_PORT & ALL_RELEASED) != ALL_RELEASED && once)
        {
            /* Key appears pressed — debounce by waiting ~500 iterations     */
            for (unsigned int i = 0U; i < 500U; i++);

            if ((KEYPAD_PORT & ALL_RELEASED) != ALL_RELEASED)
            {
                once = 0;                           /* Consume this edge     */
                return (unsigned char)(KEYPAD_PORT & ALL_RELEASED);
            }
        }
        else if ((KEYPAD_PORT & ALL_RELEASED) == ALL_RELEASED)
        {
            once = 1;   /* All keys released: arm the edge detector again   */
        }
    }

    return ALL_RELEASED;    /* No valid edge detected                        */
}