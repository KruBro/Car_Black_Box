/**
 * @file    clcd.h
 * @author  krubro
 * @date    2026-05-05
 * @brief   Public API for the HD44780 16×2 Character LCD driver.
 *
 * @details
 *   The display is driven in 8-bit parallel mode with a separate RS and EN
 *   line. The data bus is mapped to PORTD; control lines to PORTE.
 *
 *   Pin mapping:
 *     PORTD [RD0–RD7] → D0–D7  (LCD data bus)
 *     RE1             → EN     (Enable pulse)
 *     RE2             → RS     (Register Select: 0=instruction, 1=data)
 *
 *   Line address macros:
 *     LINE1(col) → DDRAM address for row 0, column col  (col = 0–15)
 *     LINE2(col) → DDRAM address for row 1, column col  (col = 0–15)
 */

#ifndef CLCD_H
#define CLCD_H

/* -------------------------------------------------------------------------
 * Hardware Mapping
 * ------------------------------------------------------------------------- */
#define CLCD_DATA_PORT_DDR      TRISD   /**< PORTD direction register        */
#define CLCD_RS_DDR             TRISE2  /**< RE2 direction bit               */
#define CLCD_EN_DDR             TRISE1  /**< RE1 direction bit               */

#define CLCD_DATA_PORT          PORTD   /**< 8-bit data bus port             */
#define CLCD_RS                 RE2     /**< Register Select line            */
#define CLCD_EN                 RE1     /**< Enable line                     */

/* -------------------------------------------------------------------------
 * RS Mode Constants
 * ------------------------------------------------------------------------- */
#define INST_MODE               0       /**< RS=0: write instruction byte    */
#define DATA_MODE               1       /**< RS=1: write character data      */

/* -------------------------------------------------------------------------
 * Logic Level Aliases
 * ------------------------------------------------------------------------- */
#define HI                      1
#define LOW                     0

/* -------------------------------------------------------------------------
 * DDRAM Address Macros
 * ------------------------------------------------------------------------- */
#define LINE1(x)        (0x80U + (x))   /**< Row 0, column x (0-indexed)    */
#define LINE2(x)        (0xC0U + (x))   /**< Row 1, column x (0-indexed)    */

/* -------------------------------------------------------------------------
 * HD44780 Instruction Codes
 * ------------------------------------------------------------------------- */
#define EIGHT_BIT_MODE              0x33U   /**< Power-on function set byte  */
#define TWO_LINES_5x8_8_BIT_MODE    0x38U   /**< 2-line, 5×8 font, 8-bit    */
#define CLEAR_DISP_SCREEN           0x01U   /**< Clear display; cursor home  */
#define DISP_ON_AND_CURSOR_OFF      0x0CU   /**< Display ON, cursor OFF      */

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/**
 * @brief   Initialises the CLCD port pins and runs the HD44780 init sequence.
 * @details Must be called once before any other clcd_ function.
 *          Follows the HD44780 recommended power-on initialisation procedure
 *          (three function-set writes with specified delays).
 */
void init_clcd(void);

/**
 * @brief   Writes a single character at a given DDRAM address.
 * @param[in] data  ASCII character to display.
 * @param[in] addr  DDRAM address — use LINE1(col) or LINE2(col).
 */
void clcd_putch(const char data, unsigned char addr);

/**
 * @brief   Writes a null-terminated string starting at a given DDRAM address.
 * @param[in] str   Null-terminated string to display.
 * @param[in] addr  DDRAM start address — use LINE1(col) or LINE2(col).
 */
void clcd_print(const char *str, unsigned char addr);

/**
 * @brief   Sends the Clear Display command and waits for it to complete.
 * @details The HD44780 requires up to 1.52 ms to execute Clear Display.
 *          A __delay_ms(2) is used to safely exceed this minimum.
 */
void clcd_clear(void);

#endif /* CLCD_H */