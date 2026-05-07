/**
 * @file    blackbox_drivers.c
 * @author  krubro
 * @date    2026-05-05
 * @brief   Unified peripheral driver implementation for the Car Black Box.
 *
 * @details
 *   Target hardware : PIC16F877A on PICGENIOS board (PICSIMSLAB)
 *   Compiler        : XC8
 *   Crystal         : 20 MHz (HS mode)
 *
 *   This file consolidates the implementations for every on-board peripheral:
 *
 *     Section 1 — USART  (serial communication, RC6/RC7)
 *     Section 2 — I²C    (MSSP master, RC3/RC4)
 *     Section 3 — CLCD   (HD44780 16×2 LCD, PORTD + RE1/RE2)
 *     Section 4 — DS1307 (RTC over I²C)
 *     Section 5 — Keypad (6-switch active-low, PORTB)
 *     Section 6 — ADC    (10-bit, AN0–AN7)
 *
 *   All implementations are unchanged from the individually reviewed and
 *   bug-fixed originals. Bug history is recorded in LOG.md.
 *
 *   When adding a new peripheral:
 *     1. Add its section to blackbox_drivers.h (macros + prototypes).
 *     2. Add its implementation below the last section in this file,
 *        following the same header and comment style.
 */

#include "main_config.h"        /* FOSC, _XTAL_FREQ, xc.h                   */

/* =========================================================================
 * SECTION 1 — USART
 * Asynchronous serial, 8-N-1. RC6 = TX, RC7 = RX.
 *
 * Baud rate formula (BRGH=1, SYNC=0):
 *   SPBRG = (FOSC / (16 × baud)) − 1
 *
 * BUG HISTORY:
 *   Original functions were named getchar / putchar / puts, conflicting with
 *   the C standard library. Renamed to uart_getchar / uart_putchar / uart_puts.
 * ========================================================================= */

/**
 * @brief   Initialises the USART in asynchronous 8-N-1 mode.
 *
 *   Register summary:
 *     SPEN = 1 — Enable serial port (RC6/TX, RC7/RX become serial pins).
 *     SYNC = 0 — Asynchronous mode.
 *     CREN = 1 — Enable continuous receive.
 *     BRGH = 1 — High-speed baud rate mode.
 *     TXEN = 1 — Enable transmitter.
 *     SPBRG    — Baud Rate Generator register.
 *
 *   These register names are identical on the PIC16F877A USART and the
 *   PIC18 EUSART — no code differences are required between the two.
 */
void initUART(unsigned long baud)
{
    SPEN  = 1;                                              /* Enable serial port pins  */
    SYNC  = 0;                                              /* Asynchronous mode        */
    CREN  = 1;                                              /* Continuous receive       */
    BRGH  = 1;                                              /* High baud rate mode      */
    SPBRG = (unsigned char)((FOSC / (16UL * baud)) - 1UL); /* Baud divider             */
    TXEN  = 1;                                              /* Enable transmitter       */
}

/**
 * @brief   Receives one byte from the USART RX buffer, blocking until available.
 *
 *   Error handling:
 *     OERR (Overrun) — Clears by toggling CREN; sends debug marker 'X'.
 *     FERR (Framing) — Clears by reading RCREG; returns 0.
 */
unsigned char uart_getchar(void)
{
    if (OERR)
    {
        uart_putchar('X');      /* Debug: overrun marker on serial monitor   */
        CREN = 0;
        CREN = 1;
    }

    if (FERR)
    {
        uart_putchar('F');      /* Debug: framing error marker               */
        (void)RCREG;            /* Consume bad byte to clear FERR            */
        return 0;
    }

    while (!RCIF);              /* Block until a byte is in the RX buffer    */
    return RCREG;
}

/**
 * @brief   Transmits one byte over USART, blocking until the shift register
 *          is ready.
 */
void uart_putchar(unsigned char ch)
{
    while (!TXIF);              /* Wait until TX shift register is empty     */
    TXREG = ch;
}

/**
 * @brief   Transmits a null-terminated string byte-by-byte over USART.
 */
void uart_puts(const char *str)
{
    while (*str != '\0')
    {
        uart_putchar((unsigned char)*str);
        str++;
    }
}


/* =========================================================================
 * SECTION 2 — I²C (MSSP Master)
 * RC3 = SCL, RC4 = SDA. Standard mode 100 kHz default.
 *
 * SSPADD formula:  SSPADD = (FOSC / (4 × SCL_freq)) − 1
 *
 * BUG HISTORY:
 *   i2c_wait_for_idle() was incorrectly declared static in i2c.h (public
 *   header). Moved to this file as a file-scoped static — it is an internal
 *   implementation detail and must not be visible to callers.
 * ========================================================================= */

/**
 * @brief   Blocks until the MSSP module has no ongoing bus activity.
 * @details Checks R_nW (Receive/Transmit in progress) and SSPCON2[4:0]
 *          (SEN, RSEN, PEN, RCEN, ACKEN — all must be 0 before proceeding).
 */
static void i2c_wait_for_idle(void)
{
    while (R_nW || (SSPCON2 & 0x1F));
}

/**
 * @brief   Initialises the MSSP module as an I²C master.
 */
void initI2C(unsigned long baud)
{
    SSPM3 = 1;      /* SSPM = 0b1000 — I²C Master mode                     */
    SSPM2 = 0;
    SSPM1 = 0;
    SSPM0 = 0;

    SSPADD = (unsigned char)((FOSC / (4UL * baud)) - 1UL);

    TRISC3 = 1;     /* RC3 (SCL) must be input; MSSP controls the line       */
    TRISC4 = 1;     /* RC4 (SDA) must be input; MSSP controls the line       */

    SSPEN  = 1;     /* Enable Synchronous Serial Port                        */
}

/**
 * @brief   Issues an I²C START condition (SDA low while SCL high).
 */
void i2c_start(void)
{
    i2c_wait_for_idle();
    SEN = 1;        /* Hardware auto-clears SEN when START is complete       */
}

/**
 * @brief   Issues an I²C REPEATED START condition.
 */
void i2c_repeat_start(void)
{
    i2c_wait_for_idle();
    RSEN = 1;       /* Hardware auto-clears RSEN when complete               */
}

/**
 * @brief   Issues an I²C STOP condition (SDA high while SCL high).
 */
void i2c_stop(void)
{
    i2c_wait_for_idle();
    PEN = 1;        /* Hardware auto-clears PEN when STOP is complete        */
}

/**
 * @brief   Writes one byte to the I²C bus and returns the slave ACK status.
 * @return  1 if slave ACKed (ACKSTAT=0), 0 if NACKed.
 */
int i2c_write(unsigned char data)
{
    i2c_wait_for_idle();
    SSPBUF = data;
    i2c_wait_for_idle();        /* Wait for byte to finish shifting out      */
    return !ACKSTAT;
}

/**
 * @brief   Reads one byte from the I²C bus, then sends ACK or NACK.
 *
 *   ACK/NACK convention:
 *     ack = 0 → ACK  (master expects more bytes)
 *     ack = 1 → NACK (this is the last byte; releases the slave)
 *
 *   Always use ack=1 for the final (or only) byte of any read transaction.
 *   This is mandated by the I²C specification and the DS1307 datasheet.
 */
unsigned char i2c_read(unsigned char ack)
{
    unsigned char data;

    i2c_wait_for_idle();
    RCEN = 1;                   /* Enable receiver; auto-cleared when done   */
    i2c_wait_for_idle();

    data = SSPBUF;

    ACKDT = (ack == 1) ? 1 : 0; /* 1 = NACK, 0 = ACK                       */
    ACKEN = 1;                  /* Initiate the ACK/NACK sequence            */

    return data;
}


/* =========================================================================
 * SECTION 3 — CLCD (HD44780 16×2, 8-bit parallel)
 * Data: PORTD [RD0–RD7]. Control: RE1 (EN), RE2 (RS).
 *
 * BUG HISTORY:
 *   clcd_clear() used an empty for-loop as a delay, which the XC8 compiler
 *   eliminates at -O1 or higher. Replaced with __delay_ms(2) — optimisation-
 *   safe and precisely meets the HD44780's ≥ 1.52 ms clear-time requirement.
 * ========================================================================= */

/**
 * @brief   Sends one byte to the LCD as either a command or character data.
 * @param[in] byte  Byte to send.
 * @param[in] mode  INST_MODE (0) for command, DATA_MODE (1) for data.
 */
static void clcd_write(unsigned char byte, unsigned char mode)
{
    CLCD_RS        = mode;      /* Select instruction or data register       */
    CLCD_DATA_PORT = byte;      /* Place byte on 8-bit data bus              */

    CLCD_EN = HI;               /* Pulse EN high (data latched on fall)      */
    __delay_us(100);
    CLCD_EN = LOW;

    __delay_us(4100);           /* Allow command to execute (≥ 4.1 ms)      */
}

/**
 * @brief   Executes the HD44780 three-write power-on initialisation sequence.
 * @details Guarantees a known controller state regardless of Vcc rise time,
 *          per the HD44780 application note.
 */
static void init_display_controller(void)
{
    __delay_ms(30);             /* ≥ 15 ms after Vcc stable (using 30 ms)   */

    clcd_write(EIGHT_BIT_MODE, INST_MODE);  /* Step 1: first function set   */
    __delay_us(4100);                        /* ≥ 4.1 ms                     */

    clcd_write(EIGHT_BIT_MODE, INST_MODE);  /* Step 2: second function set  */
    __delay_us(100);                         /* ≥ 100 µs                     */

    clcd_write(EIGHT_BIT_MODE, INST_MODE);  /* Step 3: third function set   */
    __delay_us(1);

    clcd_write(TWO_LINES_5x8_8_BIT_MODE, INST_MODE); /* 2-line, 5×8, 8-bit */
    __delay_us(100);

    clcd_write(CLEAR_DISP_SCREEN, INST_MODE);         /* Clear display       */
    __delay_us(500);

    clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);    /* Display ON          */
    __delay_us(100);
}

/**
 * @brief   Configures PORTD and PORTE pins, then runs the HD44780 init sequence.
 */
void init_clcd(void)
{
    CLCD_DATA_PORT_DDR = 0x00;  /* PORTD: all output (data bus)              */
    CLCD_RS_DDR        = 0;     /* RE2: output (RS)                          */
    CLCD_EN_DDR        = 0;     /* RE1: output (EN)                          */

    init_display_controller();
}

/**
 * @brief   Writes a single character at the given DDRAM address.
 */
void clcd_putch(const char data, unsigned char addr)
{
    clcd_write(addr, INST_MODE);
    clcd_write((unsigned char)data, DATA_MODE);
}

/**
 * @brief   Writes a null-terminated string starting at the given DDRAM address.
 */
void clcd_print(const char *str, unsigned char addr)
{
    clcd_write(addr, INST_MODE);

    while (*str != '\0')
    {
        clcd_write((unsigned char)*str, DATA_MODE);
        str++;
    }
}

/**
 * @brief   Sends Clear Display and waits 2 ms for the HD44780 to complete it.
 *
 * @note    FIX: Original used `for(i=0;i<50;i++);` — eliminated by optimiser.
 *          __delay_ms(2) is unconditionally safe and meets the 1.52 ms minimum.
 */
void clcd_clear(void)
{
    clcd_write(CLEAR_DISP_SCREEN, INST_MODE);
    __delay_ms(2);
}


/* =========================================================================
 * SECTION 4 — DS1307 Real-Time Clock
 * Communicates over I²C. 7-bit address: 0x68.
 * Registers: 0x00 (sec), 0x01 (min), 0x02 (hour) — BCD encoded.
 *
 * BUG HISTORY:
 *   ds1307_i2c_read() called i2c_read(0) — sending ACK — on a single-byte
 *   read. The I²C spec requires NACK after the last byte. Sending ACK caused
 *   the DS1307 to attempt further transmission, corrupting the bus.
 *   Fixed to i2c_read(1).
 * ========================================================================= */

/**
 * @brief   Clears the Clock Halt (CH) bit so the DS1307 oscillator runs.
 * @details The DS1307 powers up with CH=1 (halted) if no backup battery was
 *          ever connected. This function reads, masks bit 7, and writes back.
 */
void init_rtc(void)
{
    unsigned char sec_reg;

    sec_reg = ds1307_i2c_read(SEC_ADDRESS);
    sec_reg = sec_reg & 0x7FU;          /* Clear CH bit (bit 7) → run mode  */
    ds1307_i2c_write(sec_reg, SEC_ADDRESS);
}

/**
 * @brief   Reads one byte from a DS1307 register.
 *
 *   Transaction:
 *     START → SLAVE_WRITE → address → REPEATED_START
 *           → SLAVE_READ  → read(NACK) → STOP
 */
unsigned char ds1307_i2c_read(unsigned char address)
{
    unsigned char data;

    i2c_start();
    i2c_write(SLAVE_WRITE);
    i2c_write(address);
    i2c_repeat_start();
    i2c_write(SLAVE_READ);

    /* FIX: i2c_read(1) — NACK. Required by I²C spec on the last byte.     */
    data = i2c_read(1);

    i2c_stop();
    return data;
}

/**
 * @brief   Writes one byte to a DS1307 register.
 *
 *   Transaction:
 *     START → SLAVE_WRITE → address → data → STOP
 */
void ds1307_i2c_write(unsigned char data, unsigned char address)
{
    i2c_start();
    i2c_write(SLAVE_WRITE);
    i2c_write(address);
    i2c_write(data);
    i2c_stop();
}


/* =========================================================================
 * SECTION 5 — Digital Keypad (6-switch active-low, PORTB RB0–RB5)
 *
 * EDGE mode algorithm:
 *   `once` starts at 1 (armed). On a press: debounce → set once=0 → return
 *   key state. Subsequent calls return ALL_RELEASED until the key is released,
 *   at which point once is reset to 1.
 * ========================================================================= */

/**
 * @brief   Initialises PORTB as input and drives the data latch high to
 *          enable the on-chip weak pull-ups.
 */
void init_digital_keypad(void)
{
    TRISB       = 0xFF;         /* All PORTB pins: input                     */
    KEYPAD_PORT = 0xFF;         /* Data latch high → enable weak pull-ups    */
}

/**
 * @brief   Reads the keypad in either LEVEL or EDGE trigger mode.
 * @return  Masked PORTB value, or ALL_RELEASED (0x3F) if no key is active.
 */
unsigned char read_digital_keypad(unsigned char trigger_method)
{
    static unsigned char once = 1;  /* 1 = armed; 0 = edge consumed         */

    if (trigger_method == LEVEL)
    {
        return (unsigned char)(KEYPAD_PORT & ALL_RELEASED);
    }
    else if (trigger_method == EDGE)
    {
        if ((KEYPAD_PORT & ALL_RELEASED) != ALL_RELEASED && once)
        {
            /* Key appears pressed — debounce (~25 µs at 20 MHz)             */
            for (unsigned int i = 0U; i < 500U; i++);

            if ((KEYPAD_PORT & ALL_RELEASED) != ALL_RELEASED)
            {
                once = 0;
                return (unsigned char)(KEYPAD_PORT & ALL_RELEASED);
            }
        }
        else if ((KEYPAD_PORT & ALL_RELEASED) == ALL_RELEASED)
        {
            once = 1;           /* Key released — re-arm the edge detector   */
        }
    }

    return ALL_RELEASED;
}


/* =========================================================================
 * SECTION 6 — ADC (10-bit, PIC16F877A built-in)
 * Fosc/8 clock. Right-justified result. All pins analogue (PCFG=0b0000).
 *
 * BUG HISTORY:
 *   initADC() contained `GO = 1`, starting a conversion before ADON was set.
 *   The ADC module was still powered down, producing an undefined result.
 *   Removed; conversions are started exclusively in read_adc().
 * ========================================================================= */

/**
 * @brief   Configures and powers up the ADC module. Does NOT start a conversion.
 *
 *   ADCON0 settings applied:
 *     ADCS[2:0] = 0b010 → Fosc/8 (TAD = 0.4 µs at 20 MHz — within spec)
 *     CHS[2:0]  = 0b000 → AN0 default (overridden per call in read_adc)
 *     ADFM      = 1     → Right-justified result
 *     PCFG[3:0] = 0b0000 → All AN pins as analogue inputs
 *     ADON      = 1     → Power up (set LAST, after all config is applied)
 */
void initADC(void)
{
    /* Conversion clock: Fosc/8 (ADCS = 0b010)                              */
    ADCS0 = 0;
    ADCS1 = 1;
    ADCS2 = 0;

    /* Default channel: AN0                                                  */
    CHS0 = 0;
    CHS1 = 0;
    CHS2 = 0;

    ADFM  = 1;          /* Right-justified (ADRESL = bits 7:0)               */

    /* All PORTA pins configured as analogue inputs                          */
    PCFG0 = 0;
    PCFG1 = 0;
    PCFG2 = 0;
    PCFG3 = 0;

    ADON  = 1;          /* Power on ADC — MUST be last                       */

    /*
     * FIX: `GO = 1` removed from here.
     * Triggering a conversion before ADON=1 (and before any acquisition time
     * has elapsed) produces an undefined result — forbidden by the PIC16F877A
     * datasheet, Section 11.1.
     */
}

/**
 * @brief   Selects the channel, starts a conversion, and returns the 10-bit result.
 *
 *   Channel selection: writes to ADCON0[5:3] (CHS[2:0]).
 *   Conversion time:   12 TAD = 12 × 0.4 µs = 4.8 µs at 20 MHz.
 */
unsigned short read_adc(unsigned char channel)
{
    /* Select channel without disturbing other ADCON0 bits                   */
    ADCON0 = (ADCON0 & 0xC7U) | ((unsigned char)(channel << 3));

    GO = 1;             /* Start conversion                                  */

    while (nDONE);      /* Wait — nDONE clears when conversion completes     */

    return (unsigned short)((ADRESH << 8) | ADRESL);
}


/* =========================================================================
 * ADD NEW PERIPHERAL SECTIONS BELOW THIS LINE
 *
 * Follow the same pattern:
 *   1. Section header comment block describing the peripheral and pins.
 *   2. Private static helpers (if any) before the public functions.
 *   3. Public functions exactly matching the prototypes in blackbox_drivers.h.
 * ========================================================================= */