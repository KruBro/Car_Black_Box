/**
 * @file blackbox_drivers.h
 * @author krubro
 * @date 2026-05-09
 * @brief Unified peripheral driver API for the Car Black Box project.
 *
 * @note Include main_config.h in application code, not this file directly.
 *       FOSC and _XTAL_FREQ must be defined before this header is processed.
 */

#ifndef BLACKBOX_DRIVERS_H
#define BLACKBOX_DRIVERS_H

#include <xc.h>

/* =========================================================================
 * SECTION 1 — USART (RC6 = TX, RC7 = RX)
 * ========================================================================= */

void initUART(unsigned long baud);


void uart_putchar(unsigned char ch);
void uart_puts(const char *str);

/* =========================================================================
 * SECTION 2 — I2C MSSP Master (RC3 = SCL, RC4 = SDA)
 * ========================================================================= */

void          initI2C(unsigned long baud);
void          i2c_start(void);
void          i2c_stop(void);
void          i2c_repeat_start(void);
int           i2c_write(unsigned char data);
unsigned char i2c_read(unsigned char ack);

/* =========================================================================
 * SECTION 3 — CLCD HD44780 16x2, 8-bit parallel (PORTD, RE1=EN, RE2=RS)
 * ========================================================================= */

#define CLCD_DATA_PORT_DDR  TRISD
#define CLCD_RS_DDR         TRISE2
#define CLCD_EN_DDR         TRISE1
#define CLCD_DATA_PORT      PORTD
#define CLCD_RS             RE2
#define CLCD_EN             RE1

#define INST_MODE   0
#define DATA_MODE   1

#define HI   1
#define LOW  0

#define LINE1(x)  (0x80U + (x))
#define LINE2(x)  (0xC0U + (x))

#define EIGHT_BIT_MODE              0x33U
#define TWO_LINES_5x8_8_BIT_MODE    0x38U
#define CLEAR_DISP_SCREEN           0x01U
#define DISP_ON_AND_CURSOR_OFF      0x0CU

void init_clcd(void);
void clcd_putch(const char data, unsigned char addr);
void clcd_print(const char *str, unsigned char addr);
void clcd_clear(void);

/* =========================================================================
 * SECTION 4 — DS1307 RTC (I2C addr 0x68, registers in BCD format)
 * ========================================================================= */

#define SLAVE_WRITE     0xD0U
#define SLAVE_READ      0xD1U
#define SEC_ADDRESS     0x00U
#define MIN_ADDRESS     0x01U
#define HOUR_ADDRESS    0x02U

void          init_rtc(void);
unsigned char ds1307_i2c_read(unsigned char address);
void          ds1307_i2c_write(unsigned char data, unsigned char address);

/* =========================================================================
 * SECTION 5 — Digital Keypad 6-switch active-low on PORTB (RB0-RB5)
 * ========================================================================= */

#define SW1     0x3EU
#define SW2     0x3DU
#define SW3     0x3BU
#define SW4     0x37U
#define SW5     0x2FU
#define SW6     0x1FU

#define KEYPAD_PORT     PORTB
#define ALL_RELEASED    0x3FU

#define LEVEL   1
#define EDGE    0

void          init_digital_keypad(void);
unsigned char read_digital_keypad(unsigned char trigger_method);

/* =========================================================================
 * SECTION 6 — ADC 10-bit, right-justified (AN0-AN7 on PORTA/PORTE)
 * ========================================================================= */

#define CHANNEL0    0
#define CHANNEL1    1

void           initADC(void);
unsigned short read_adc(unsigned char channel);

#endif /* BLACKBOX_DRIVERS_H */