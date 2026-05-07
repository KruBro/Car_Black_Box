/**
 * @file    digital_keypad.h
 * @author  krubro
 * @date    2026-05-05
 * @brief   Public API for the 6-switch active-low digital keypad driver.
 *
 * @details
 *   Six momentary switches are wired from PORTB pins RB0–RB5 to GND.
 *   Internal or external pull-ups keep the pins high when no switch is
 *   pressed. Pressing a switch drives the corresponding pin low.
 *
 *   Switch-to-bit mapping (bit = 0 means pressed):
 *     SW1 → RB0   SW2 → RB1   SW3 → RB2
 *     SW4 → RB3   SW5 → RB4   SW6 → RB5
 *
 *   RB6 and RB7 are masked out (not used). ALL_RELEASED (0x3F) means
 *   all six switch bits are high (no switch pressed).
 *
 *   Trigger modes:
 *     LEVEL — Returns the raw port state every time. Suitable for
 *              hold-to-activate behaviour.
 *     EDGE  — Returns the port state ONCE per press, then ALL_RELEASED
 *              until the switch is fully released (software debounce).
 *              Suitable for increment/decrement buttons.
 */

#ifndef DIGITAL_KEYPAD_H
#define DIGITAL_KEYPAD_H

/* -------------------------------------------------------------------------
 * Switch Masks (active-low: bit is 0 when pressed)
 * Mask against (PORTB & ALL_RELEASED) to identify the pressed switch.
 * ------------------------------------------------------------------------- */
#define SW1     0x3EU   /**< RB0 low — crash event / ignition on            */
#define SW2     0x3DU   /**< RB1 low — gear up                              */
#define SW3     0x3BU   /**< RB2 low — gear down                            */
#define SW4     0x37U   /**< RB3 low — reserved                             */
#define SW5     0x2FU   /**< RB4 low — reserved                             */
#define SW6     0x1FU   /**< RB5 low — reserved                             */

/* -------------------------------------------------------------------------
 * Port and Mask
 * ------------------------------------------------------------------------- */
#define KEYPAD_PORT     PORTB   /**< Hardware port connected to switches     */
#define ALL_RELEASED    0x3FU   /**< All 6 switch bits high = no key pressed */

/* -------------------------------------------------------------------------
 * Trigger Mode Constants
 * ------------------------------------------------------------------------- */
#define LEVEL   1   /**< Continuous read: returns state every call          */
#define EDGE    0   /**< Edge-triggered: returns state once per press       */

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/**
 * @brief   Configures PORTB as input with KEYPAD_PORT initialised high.
 * @details TRISB = 0xFF sets all PORTB pins as inputs. Writing 0xFF to
 *          PORTB enables the on-chip weak pull-ups via the data latch.
 */
void init_digital_keypad(void);

/**
 * @brief   Reads the current keypad state using the specified trigger mode.
 * @param[in] trigger_method  LEVEL for continuous, EDGE for single-fire.
 * @return   Masked PORTB value. Compare against SWx constants to identify
 *           which switch is pressed. Returns ALL_RELEASED (0x3F) if none.
 */
unsigned char read_digital_keypad(unsigned char trigger_method);

#endif /* DIGITAL_KEYPAD_H */