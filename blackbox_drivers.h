/**
 * @file    blackbox_drivers.h
 * @author  krubro
 * @date    2026-05-05
 * @brief   Unified peripheral driver API for the Car Black Box project.
 *          Covers USART, I²C, CLCD, DS1307, Keypad, and ADC.
 *
 * @note    Include main_config.h in application code — not this file directly.
 *          FOSC and _XTAL_FREQ must be defined before this header is processed.
 */

#ifndef BLACKBOX_DRIVERS_H
#define BLACKBOX_DRIVERS_H

#include <xc.h>

/* =========================================================================
 * SECTION 1 — USART  (RC6 = TX, RC7 = RX)
 * ========================================================================= */

/** @brief  Initialises the USART in asynchronous 8-N-1 mode. */
void initUART(unsigned long baud);

/** @brief  Receives one byte from the USART RX buffer (blocking). Returns 0 on framing error. */
unsigned char uart_getchar(void);

/** @brief  Transmits one byte over USART (blocking). */
void uart_putchar(unsigned char ch);

/** @brief  Transmits a null-terminated string over USART. */
void uart_puts(const char *str);


/* =========================================================================
 * SECTION 2 — I²C MSSP Master  (RC3 = SCL, RC4 = SDA)
 * ========================================================================= */

/** @brief  Initialises the MSSP as an I²C master. */
void initI2C(unsigned long baud);

/** @brief  Issues an I²C START condition. */
void i2c_start(void);

/** @brief  Issues an I²C STOP condition. */
void i2c_stop(void);

/** @brief  Issues an I²C REPEATED START condition (switch direction without releasing bus). */
void i2c_repeat_start(void);

/** @brief  Writes one byte to the I²C bus. Returns 1 if ACKed, 0 if NACKed. */
int i2c_write(unsigned char data);

/** @brief  Reads one byte from the I²C bus. Pass ack=0 for ACK, ack=1 for NACK (last byte). */
unsigned char i2c_read(unsigned char ack);


/* =========================================================================
 * SECTION 3 — CLCD  HD44780 16×2, 8-bit parallel (PORTD, RE1=EN, RE2=RS)
 * ========================================================================= */

// Hardware mapping
#define CLCD_DATA_PORT_DDR  TRISD
#define CLCD_RS_DDR         TRISE2
#define CLCD_EN_DDR         TRISE1
#define CLCD_DATA_PORT      PORTD
#define CLCD_RS             RE2     // Register Select: 0 = instruction, 1 = data
#define CLCD_EN             RE1     // Enable pulse

// RS mode
#define INST_MODE           0
#define DATA_MODE           1

// Logic levels
#define HI                  1
#define LOW                 0

// DDRAM address helpers
#define LINE1(x)    (0x80U + (x))   // Row 0, column x
#define LINE2(x)    (0xC0U + (x))   // Row 1, column x

// HD44780 instruction codes
#define EIGHT_BIT_MODE              0x33U
#define TWO_LINES_5x8_8_BIT_MODE    0x38U
#define CLEAR_DISP_SCREEN           0x01U
#define DISP_ON_AND_CURSOR_OFF      0x0CU

/** @brief  Initialises CLCD GPIO pins and runs the HD44780 power-on sequence. */
void init_clcd(void);

/** @brief  Writes one character at a DDRAM address. Use LINE1(col) or LINE2(col). */
void clcd_putch(const char data, unsigned char addr);

/** @brief  Writes a null-terminated string starting at a DDRAM address. */
void clcd_print(const char *str, unsigned char addr);

/** @brief  Sends Clear Display command and waits ≥ 2 ms. */
void clcd_clear(void);


/* =========================================================================
 * SECTION 4 — DS1307 RTC  (I²C addr 0x68, registers in BCD format)
 * ========================================================================= */

// I²C address bytes (7-bit addr 0x68 shifted + R/W bit)
#define SLAVE_WRITE     0xD0U   // Write
#define SLAVE_READ      0xD1U   // Read

// Register addresses
#define SEC_ADDRESS     0x00U   // Seconds  (bit 7 = Clock Halt flag)
#define MIN_ADDRESS     0x01U   // Minutes
#define HOUR_ADDRESS    0x02U   // Hours (24-hour mode)

/** @brief  Clears the DS1307 Clock Halt bit so the oscillator runs. Call once at startup. */
void init_rtc(void);

/** @brief  Reads one BCD byte from a DS1307 register. */
unsigned char ds1307_i2c_read(unsigned char address);

/** @brief  Writes one BCD byte to a DS1307 register. */
void ds1307_i2c_write(unsigned char data, unsigned char address);


/* =========================================================================
 * SECTION 5 — Digital Keypad  6-switch active-low on PORTB (RB0–RB5)
 * ========================================================================= */

// Switch masks — the corresponding bit is LOW when pressed
#define SW1             0x3EU   // RB0 — crash / ignition
#define SW2             0x3DU   // RB1 — gear up
#define SW3             0x3BU   // RB2 — gear down
#define SW4             0x37U   // RB3
#define SW5             0x2FU   // RB4
#define SW6             0x1FU   // RB5

#define KEYPAD_PORT     PORTB
#define ALL_RELEASED    0x3FU   // All 6 switch bits high = no key pressed

#define LEVEL           1       // Continuous — state returned every call
#define EDGE            0       // Single-fire — state returned once per press

/** @brief  Configures PORTB as input and enables pull-ups. */
void init_digital_keypad(void);

/** @brief  Reads keypad state. trigger_method: LEVEL or EDGE. Returns SWx mask or ALL_RELEASED. */
unsigned char read_digital_keypad(unsigned char trigger_method);


/* =========================================================================
 * SECTION 6 — ADC  10-bit, right-justified (AN0–AN7 on PORTA/PORTE)
 * ========================================================================= */

#define CHANNEL0    0   // AN0 — RA0, speed potentiometer
#define CHANNEL1    1   // AN1 — reserved

/** @brief  Configures the ADC module (Fosc/8, right-justified). Does NOT start a conversion. */
void initADC(void);

/** @brief  Starts a blocking 10-bit conversion on the given channel. Returns 0–1023. */
unsigned short read_adc(unsigned char channel);


/* =========================================================================
 * ADD NEW PERIPHERAL SECTIONS BELOW THIS LINE
 * Template:
 *   #define YOUR_MACRO  value   // brief comment
 *   / ** @brief One-line description. * /
 *   return_type function_name(param_type param);
 * ========================================================================= */

#endif /* BLACKBOX_DRIVERS_H */