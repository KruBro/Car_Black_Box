/**
 * @file    i2c.h
 * @author  krubro
 * @date    2026-05-05
 * @brief   Public API for the I²C Master driver (PIC16F877A MSSP module,
 *          PICGENIOS board / PICSIMSLAB).
 *
 * @details
 *   Provides start/stop/repeat-start conditions plus single-byte write and
 *   read primitives. Only the symbols that external callers need are declared
 *   here; `i2c_wait_for_idle` is a private implementation detail and is
 *   therefore NOT declared in this header (it is static inside i2c.c).
 *
 *   FOSC is intentionally NOT defined here; it is provided by main_config.h.
 */

#ifndef I2C_H
#define I2C_H

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/**
 * @brief   Initialises the MSSP module as an I²C master.
 * @param[in] baud  Desired SCL clock frequency in Hz (e.g. 100000 for 100 kHz).
 * @note    Configures RC3 (SCL) and RC4 (SDA) as inputs; the MSSP hardware
 *          controls the lines after SSPEN is set.
 */
void initI2C(unsigned long baud);

/**
 * @brief   Issues an I²C START condition on the bus.
 * @details Waits for bus idle before asserting the condition.
 */
void i2c_start(void);

/**
 * @brief   Issues an I²C STOP condition on the bus.
 * @details Waits for bus idle before asserting the condition.
 */
void i2c_stop(void);

/**
 * @brief   Issues an I²C REPEATED START condition on the bus.
 * @details Used to change direction (write→read) without releasing the bus.
 */
void i2c_repeat_start(void);

/**
 * @brief   Writes one byte to the I²C bus and returns the ACK status.
 * @param[in] data  Byte to transmit (address or data).
 * @return   1 if the slave ACKed, 0 if NACKed.
 */
int i2c_write(unsigned char data);

/**
 * @brief   Reads one byte from the I²C bus and sends ACK or NACK.
 * @param[in] ack  Pass 1 to send NACK (last byte of a read), 0 to send ACK.
 * @return   Byte received from the slave.
 * @note    Always pass ack=1 when reading the final (or only) byte of a
 *          transaction, as required by the I²C specification.
 */
unsigned char i2c_read(unsigned char ack);

#endif /* I2C_H */