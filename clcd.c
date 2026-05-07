/**
 * @file    clcd.c
 * @author  krubro
 * @date    2026-05-05
 * @brief   HD44780 16×2 Character LCD driver implementation (8-bit mode).
 *
 * @details
 *   BUG FIXED: clcd_clear() previously used an empty `for` loop as a delay.
 *   At compiler optimisation level -O1 or higher the loop body is eliminated,
 *   leaving no delay at all. The HD44780 datasheet specifies a minimum of
 *   1.52 ms for the Clear Display command to complete. The loop has been
 *   replaced with __delay_ms(2), which is optimisation-safe.
 *
 *   Initialisation follows the HD44780 application note power-on sequence:
 *     1. Wait ≥ 15 ms after Vcc rises above 4.5 V.
 *     2. Send Function Set (0x33) — wait ≥ 4.1 ms.
 *     3. Send Function Set again (0x33) — wait ≥ 100 µs.
 *     4. Send Function Set a third time (0x33).
 *     5. Now set final configuration: 2-line, 5×8, 8-bit.
 *     6. Clear display.
 *     7. Display ON.
 */

#include "main_config.h"  
#include "clcd.h"

/* =========================================================================
 * Private Helper
 * ========================================================================= */

/**
 * @brief   Sends one byte to the LCD (instruction or data).
 * @param[in] byte  The byte to send.
 * @param[in] mode  INST_MODE (0) for command, DATA_MODE (1) for character.
 */
static void clcd_write(unsigned char byte, unsigned char mode)
{
    CLCD_RS         = mode;     /* Select instruction or data register       */
    CLCD_DATA_PORT  = byte;     /* Place byte on 8-bit data bus              */

    CLCD_EN = HI;               /* Pulse EN high (data latched on falling edge) */
    __delay_us(100);
    CLCD_EN = LOW;

    __delay_us(4100);           /* Wait for the command to execute (≥ 4.1 ms) */
}

/* =========================================================================
 * Private Initialisation Sequence
 * ========================================================================= */

/**
 * @brief   Executes the HD44780 recommended power-on initialisation procedure.
 * @details Uses the 3-write "software reset" sequence from the HD44780
 *          application note to guarantee a known state regardless of Vcc
 *          rise time.
 */
static void init_display_controller(void)
{
    __delay_ms(30);             /* Wait ≥ 15 ms after power on (using 30 ms) */

    /* Step 1: First function-set write (reset sequence)                     */
    clcd_write(EIGHT_BIT_MODE, INST_MODE);
    __delay_us(4100);           /* ≥ 4.1 ms required                         */

    /* Step 2: Second function-set write                                     */
    clcd_write(EIGHT_BIT_MODE, INST_MODE);
    __delay_us(100);            /* ≥ 100 µs required                         */

    /* Step 3: Third function-set write                                      */
    clcd_write(EIGHT_BIT_MODE, INST_MODE);
    __delay_us(1);

    /* Step 4: Set 2-line, 5×8 font, 8-bit interface                        */
    clcd_write(TWO_LINES_5x8_8_BIT_MODE, INST_MODE);
    __delay_us(100);

    /* Step 5: Clear display screen                                          */
    clcd_write(CLEAR_DISP_SCREEN, INST_MODE);
    __delay_us(500);

    /* Step 6: Display ON, cursor OFF                                        */
    clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
    __delay_us(100);
}

/* =========================================================================
 * Public Function Implementations
 * ========================================================================= */

/**
 * @brief   Configures GPIO and runs the HD44780 power-on initialisation.
 */
void init_clcd(void)
{
    CLCD_DATA_PORT_DDR  = 0x00; /* PORTD: all pins output (data bus)         */
    CLCD_RS_DDR         = 0;    /* RE2: output (Register Select)             */
    CLCD_EN_DDR         = 0;    /* RE1: output (Enable)                      */

    init_display_controller();
}

/**
 * @brief   Writes a single character at the specified DDRAM address.
 */
void clcd_putch(const char data, unsigned char addr)
{
    clcd_write(addr, INST_MODE);    /* Move cursor to address                */
    clcd_write(data, DATA_MODE);    /* Write character                       */
}

/**
 * @brief   Writes a null-terminated string starting at the specified DDRAM address.
 */
void clcd_print(const char *str, unsigned char addr)
{
    clcd_write(addr, INST_MODE);    /* Move cursor to start address          */

    while (*str != '\0')
    {
        clcd_write((unsigned char)*str, DATA_MODE);
        str++;
    }
}

/**
 * @brief   Clears the display and waits the required execution time.
 *
 * @note    FIX: Replaced the original `for(i=0;i<50;i++);` busy-wait with
 *          __delay_ms(2). The empty loop was not optimisation-safe and could
 *          be eliminated by the compiler, violating the HD44780's 1.52 ms
 *          minimum clear time.
 */
void clcd_clear(void)
{
    clcd_write(CLEAR_DISP_SCREEN, INST_MODE);
    __delay_ms(2);              /* HD44780 requires ≥ 1.52 ms after Clear   */
}