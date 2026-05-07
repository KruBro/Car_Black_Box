/**
 * @file    ds1307.c
 * @author  krubro
 * @date    2026-05-05
 * @brief   DS1307 Real-Time Clock driver implementation.
 *
 * @details
 *   BUG FIXED: The original code called i2c_read(0) — sending ACK — on a
 *   single-byte read. The I²C specification requires the master to send NACK
 *   after the last (or only) received byte so the slave stops driving SDA.
 *   Sending ACK caused the DS1307 to attempt to clock out another byte,
 *   stretching the bus and corrupting subsequent transactions.
 *   Fixed to i2c_read(1) (NACK).
 *
 *   I²C transaction for a register read:
 *     START → SLAVE_WRITE → reg_address → REPEATED_START
 *           → SLAVE_READ  → read byte (NACK) → STOP
 *
 *   I²C transaction for a register write:
 *     START → SLAVE_WRITE → reg_address → data_byte → STOP
 */

#include "main_config.h"
#include "ds1307.h"
#include "i2c.h"
#include <xc.h>

/* =========================================================================
 * Public Function Implementations
 * ========================================================================= */

/**
 * @brief   Clears the Clock Halt (CH) bit in the DS1307 seconds register.
 * @details The DS1307 powers up with CH=1 (oscillator halted) if the backup
 *          battery was never connected. This function ensures the oscillator
 *          is running before time is read.
 */
void init_rtc(void)
{
    unsigned char sec_reg;

    sec_reg = ds1307_i2c_read(SEC_ADDRESS);
    sec_reg = sec_reg & 0x7F;               /* Clear bit 7 (CH = 0 → run)  */
    ds1307_i2c_write(sec_reg, SEC_ADDRESS);
}

/**
 * @brief   Reads one byte from the specified DS1307 register.
 *
 *   Transaction sequence:
 *     1. START
 *     2. Send SLAVE_WRITE + register address  (sets the internal pointer)
 *     3. REPEATED START                        (switch to read mode)
 *     4. Send SLAVE_READ
 *     5. Read one byte, sending NACK          (signals end of read)
 *     6. STOP
 */
unsigned char ds1307_i2c_read(unsigned char address)
{
    unsigned char data;

    i2c_start();
    i2c_write(SLAVE_WRITE);
    i2c_write(address);
    i2c_repeat_start();
    i2c_write(SLAVE_READ);

    /*
     * FIX: Pass 1 (NACK) — not 0 (ACK).
     * The I²C spec mandates NACK from the master after the last byte of a
     * read to release the slave. Passing 0 (ACK) incorrectly told the DS1307
     * to continue transmitting, corrupting the bus.
     */
    data = i2c_read(1);     /* 1 = NACK — correct for a single-byte read    */

    i2c_stop();
    return data;
}

/**
 * @brief   Writes one byte to the specified DS1307 register.
 *
 *   Transaction sequence:
 *     1. START
 *     2. Send SLAVE_WRITE + register address
 *     3. Send data byte
 *     4. STOP
 */
void ds1307_i2c_write(unsigned char data, unsigned char address)
{
    i2c_start();
    i2c_write(SLAVE_WRITE);
    i2c_write(address);
    i2c_write(data);
    i2c_stop();
}