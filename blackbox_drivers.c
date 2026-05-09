/**
 * @file blackbox_drivers.c
 * @author krubro
 * @date 2026-05-09
 * @brief Unified peripheral driver implementation for the Car Black Box.
 *
 * Change log:
 *   [2026-05-09] uart_getchar() and uart_data_ready() commented out.
 *                Both functions are unused in production code; at -O0 they
 *                consume ~50 program words. See blackbox_drivers.h for the
 *                full rationale and re-enable instructions.
 */

#include "main_config.h"

/* =========================================================================
 * SECTION 1 — USART
 * ========================================================================= */

void initUART(unsigned long baud)
{
    SPEN  = 1;
    SYNC  = 0;
    CREN  = 1;
    BRGH  = 1;
    SPBRG = (unsigned char)((FOSC / (16UL * baud)) - 1UL);
    TXEN  = 1;
}

/*
 * FIX [2026-05-09] — Dead Code Removal
 * ─────────────────────────────────────────────────────────────────────────────
 * uart_getchar() and uart_data_ready() are commented out because:
 *
 *   1. The compiler (XC8) reported both as unused functions.
 *   2. At -O0 they consume approximately 40–60 ROM words the PIC16F877A
 *      cannot spare.
 *   3. The production firmware never calls them: all output is one-way UART
 *      telemetry via uart_putchar() / uart_puts(); user input comes from the
 *      hardware keypad.
 *
 * The non-blocking implementation is preserved in comments so it can be
 * reinstated cleanly for a future UART command interface.
 *
 * To re-enable:
 *   1. Uncomment the two function bodies below.
 *   2. Uncomment the two prototypes in blackbox_drivers.h.
 *   3. Re-audit call-stack depth — uart_getchar adds one level.
 * ─────────────────────────────────────────────────────────────────────────────
 *
 * unsigned char uart_getchar(void)
 * {
 *     // Non-blocking: clears overrun error, returns 0 if no byte is waiting.
 *     // Check uart_data_ready() first when distinguishing "no data" from a
 *     // genuine 0x00 byte in the stream.
 *     if (OERR) { CREN = 0; CREN = 1; }
 *     if (!RCIF) return 0;
 *     if (FERR) { (void)RCREG; return 0; }
 *     return RCREG;
 * }
 *
 * unsigned char uart_data_ready(void)
 * {
 *     return RCIF;
 * }
 */

void uart_putchar(unsigned char ch)
{
    while (!TXIF);
    TXREG = ch;
}

void uart_puts(const char *str)
{
    while (*str != '\0')
    {
        if (*str == '\n') uart_putchar('\r');
        uart_putchar((unsigned char)*str);
        str++;
    }
}

/* =========================================================================
 * SECTION 2 — I2C (MSSP Master)
 * ========================================================================= */

static void i2c_wait_for_idle(void)
{
    while (R_nW || (SSPCON2 & 0x1F));
}

void initI2C(unsigned long baud)
{
    SSPM3 = 1; SSPM2 = 0; SSPM1 = 0; SSPM0 = 0;
    SSPADD  = (unsigned char)((FOSC / (4UL * baud)) - 1UL);
    TRISC3  = 1;
    TRISC4  = 1;
    SSPEN   = 1;
}

void i2c_start(void)        { i2c_wait_for_idle(); SEN  = 1; }
void i2c_repeat_start(void) { i2c_wait_for_idle(); RSEN = 1; }
void i2c_stop(void)         { i2c_wait_for_idle(); PEN  = 1; }

int i2c_write(unsigned char data)
{
    i2c_wait_for_idle();
    SSPBUF = data;
    i2c_wait_for_idle();
    return !ACKSTAT;
}

unsigned char i2c_read(unsigned char ack)
{
    unsigned char data;
    i2c_wait_for_idle();
    RCEN  = 1;
    i2c_wait_for_idle();
    data  = SSPBUF;
    ACKDT = (ack == 1) ? 1 : 0;
    ACKEN = 1;
    return data;
}

/* =========================================================================
 * SECTION 3 — CLCD (HD44780 16x2, 8-bit parallel)
 * ========================================================================= */

static void clcd_write(unsigned char byte, unsigned char mode)
{
    CLCD_RS         = mode;
    CLCD_DATA_PORT  = byte;
    CLCD_EN         = HI;
    __delay_us(100);
    CLCD_EN         = LOW;
    __delay_us(4100);
}

static void init_display_controller(void)
{
    __delay_ms(30);
    clcd_write(EIGHT_BIT_MODE,            INST_MODE); __delay_us(4100);
    clcd_write(EIGHT_BIT_MODE,            INST_MODE); __delay_us(100);
    clcd_write(EIGHT_BIT_MODE,            INST_MODE); __delay_us(1);
    clcd_write(TWO_LINES_5x8_8_BIT_MODE,  INST_MODE); __delay_us(100);
    clcd_write(CLEAR_DISP_SCREEN,         INST_MODE); __delay_us(500);
    clcd_write(DISP_ON_AND_CURSOR_OFF,    INST_MODE); __delay_us(100);
}

void init_clcd(void)
{
    CLCD_DATA_PORT_DDR = 0x00;
    CLCD_RS_DDR        = 0;
    CLCD_EN_DDR        = 0;
    init_display_controller();
}

void clcd_putch(const char data, unsigned char addr)
{
    clcd_write(addr, INST_MODE);
    clcd_write((unsigned char)data, DATA_MODE);
}

void clcd_print(const char *str, unsigned char addr)
{
    clcd_write(addr, INST_MODE);
    while (*str != '\0')
    {
        clcd_write((unsigned char)*str, DATA_MODE);
        str++;
    }
}

void clcd_clear(void)
{
    clcd_write(CLEAR_DISP_SCREEN, INST_MODE);
    __delay_ms(2);
}

/* =========================================================================
 * SECTION 4 — DS1307 RTC
 * ========================================================================= */

void init_rtc(void)
{
    unsigned char sec_reg = ds1307_i2c_read(SEC_ADDRESS);
    ds1307_i2c_write(sec_reg & 0x7FU, SEC_ADDRESS); /* Clear CH bit */
}

unsigned char ds1307_i2c_read(unsigned char address)
{
    unsigned char data;
    i2c_start();
    i2c_write(SLAVE_WRITE);
    i2c_write(address);
    i2c_repeat_start();
    i2c_write(SLAVE_READ);
    data = i2c_read(1);
    i2c_stop();
    return data;
}

void ds1307_i2c_write(unsigned char data, unsigned char address)
{
    i2c_start();
    i2c_write(SLAVE_WRITE);
    i2c_write(address);
    i2c_write(data);
    i2c_stop();
}

/* =========================================================================
 * SECTION 5 — Digital Keypad
 * ========================================================================= */

void init_digital_keypad(void)
{
    TRISB      = 0xFF;
    KEYPAD_PORT = 0xFF;
}

unsigned char read_digital_keypad(unsigned char trigger_method)
{
    static unsigned char once = 1;

    if (trigger_method == LEVEL)
        return (unsigned char)(KEYPAD_PORT & ALL_RELEASED);

    if ((KEYPAD_PORT & ALL_RELEASED) != ALL_RELEASED && once)
    {
        for (unsigned int i = 0U; i < 500U; i++);
        if ((KEYPAD_PORT & ALL_RELEASED) != ALL_RELEASED)
        {
            once = 0;
            return (unsigned char)(KEYPAD_PORT & ALL_RELEASED);
        }
    }
    else if ((KEYPAD_PORT & ALL_RELEASED) == ALL_RELEASED)
    {
        once = 1;
    }

    return ALL_RELEASED;
}

/* =========================================================================
 * SECTION 6 — ADC
 * ========================================================================= */

void initADC(void)
{
    ADCS0 = 0; ADCS1 = 1; ADCS2 = 0;
    CHS0  = 0; CHS1  = 0; CHS2  = 0;
    ADFM  = 1;
    PCFG0 = 0; PCFG1 = 0; PCFG2 = 0; PCFG3 = 0;
    ADON  = 1;
}

unsigned short read_adc(unsigned char channel)
{
    ADCON0 = (ADCON0 & 0xC7U) | ((unsigned char)(channel << 3));
    GO     = 1;
    while (nDONE);
    return (unsigned short)((ADRESH << 8) | ADRESL);
}