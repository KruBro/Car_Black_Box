/**
 * @file    i2c.c
 * @author  krubro
 * @date    2026-05-05
 * @brief   I²C Master driver implementation for PIC16F877A MSSP module
 *          (PICGENIOS board / PICSIMSLAB).
 *
 * @details
 *   All bus operations call the private helper `i2c_wait_for_idle()` to
 *   ensure the MSSP is not busy before writing control bits. This prevents
 *   collisions when back-to-back operations are issued without delay.
 *
 *   SSPADD formula (I²C master mode):
 *     SSPADD = (FOSC / (4 * SCL_freq)) - 1
 *
 *   Tested at 100 kHz (standard mode). Do not exceed 400 kHz without
 *   verifying pull-up resistor values and cable capacitance.
 */

#include "main_config.h"   /* Provides FOSC */
#include "i2c.h"
#include <xc.h>

/* =========================================================================
 * Private Helper
 * ========================================================================= */

/**
 * @brief   Blocks until the MSSP module reports no ongoing bus activity.
 * @details Checks:
 *            - R_nW  : Receive/Transmit in progress flag.
 *            - SSPCON2[4:0]: SEN, RSEN, PEN, RCEN, ACKEN — all must be 0.
 */
static void i2c_wait_for_idle(void)
{
    while (R_nW || (SSPCON2 & 0x1F));
}

/* =========================================================================
 * Public Function Implementations
 * ========================================================================= */

/**
 * @brief   Initialises the MSSP as an I²C master at the requested baud rate.
 */
void initI2C(unsigned long baud)
{
    SSPM3 = 1;      /* SSPM = 0b1000 — I²C Master mode                     */
    SSPM2 = 0;
    SSPM1 = 0;
    SSPM0 = 0;

    /* SCL frequency: SSPADD = (FOSC / (4 * baud)) - 1                      */
    SSPADD = (unsigned char)((FOSC / (4UL * baud)) - 1UL);

    /* RC3 (SCL) and RC4 (SDA) must be inputs; MSSP drives them low when needed */
    TRISC3 = 1;
    TRISC4 = 1;

    SSPEN  = 1;     /* Enable the Synchronous Serial Port                   */
}

/**
 * @brief   Issues an I²C START condition (SDA low while SCL high).
 */
void i2c_start(void)
{
    i2c_wait_for_idle();
    SEN = 1;        /* Hardware clears this bit automatically when done      */
}

/**
 * @brief   Issues an I²C REPEATED START condition.
 */
void i2c_repeat_start(void)
{
    i2c_wait_for_idle();
    RSEN = 1;       /* Hardware clears this bit automatically when done      */
}

/**
 * @brief   Issues an I²C STOP condition (SDA high while SCL high).
 */
void i2c_stop(void)
{
    i2c_wait_for_idle();
    PEN = 1;        /* Hardware clears this bit automatically when done      */
}

/**
 * @brief   Writes one byte to the I²C bus.
 * @return  1 if slave ACKed, 0 if slave NACKed.
 */
int i2c_write(unsigned char data)
{
    i2c_wait_for_idle();
    SSPBUF = data;
    i2c_wait_for_idle();    /* Wait for the byte to finish shifting out      */
    return !ACKSTAT;        /* ACKSTAT=0 means ACK received → return 1       */
}

/**
 * @brief   Reads one byte from the I²C bus.
 * @details Enables the receiver (RCEN), waits for the byte to be shifted in,
 *          reads SSPBUF, then sends the requested ACK or NACK.
 *
 *          IMPORTANT — ACK/NACK convention used here:
 *            ack = 0 → send ACK  (more bytes to read)
 *            ack = 1 → send NACK (this is the last byte; release the slave)
 *          This matches the I²C specification: the master NACKs the final byte.
 */
unsigned char i2c_read(unsigned char ack)
{
    unsigned char data;

    i2c_wait_for_idle();
    RCEN = 1;               /* Enable receiver; hardware clears RCEN when done */
    i2c_wait_for_idle();

    data  = SSPBUF;         /* Read received byte from hardware buffer       */

    ACKDT = (ack == 1) ? 1 : 0;    /* 1 = NACK, 0 = ACK                    */
    ACKEN = 1;              /* Initiate the ACK/NACK sequence                */

    return data;
}