/**
 * @file    ds1307.h
 * @author  krubro
 * @date    2026-05-05
 * @brief   Public API for the DS1307 Real-Time Clock driver.
 *
 * @details
 *   The DS1307 communicates over I²C at address 0x68 (7-bit). It stores
 *   seconds, minutes, and hours in BCD format in registers 0x00–0x02.
 *
 *   Register map (used by this driver):
 *     0x00  Seconds  — bit 7 = CH (Clock Halt), bits 6:0 = BCD seconds
 *     0x01  Minutes  — bits 6:0 = BCD minutes
 *     0x02  Hours    — bit 6 = 12/24h mode, bits 5:0 / 4:0 = BCD hours
 */

#ifndef DS1307_H
#define DS1307_H

/* -------------------------------------------------------------------------
 * I²C Address Bytes (7-bit address 0x68 shifted left + R/W bit)
 * ------------------------------------------------------------------------- */
#define SLAVE_WRITE     0xD0    /**< 0x68 << 1 | 0 — address + write bit    */
#define SLAVE_READ      0xD1    /**< 0x68 << 1 | 1 — address + read  bit    */

/* -------------------------------------------------------------------------
 * DS1307 Register Addresses
 * ------------------------------------------------------------------------- */
#define SEC_ADDRESS     0x00    /**< Seconds register (also contains CH bit) */
#define MIN_ADDRESS     0x01    /**< Minutes register                        */
#define HOUR_ADDRESS    0x02    /**< Hours register (24-hour mode assumed)   */

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/**
 * @brief   Starts the DS1307 oscillator by clearing the Clock Halt (CH) bit.
 * @details Reads the seconds register, masks bit 7 to 0, and writes it back.
 *          Must be called after power-on to ensure the RTC is running.
 */
void init_rtc(void);

/**
 * @brief   Reads one byte from a DS1307 register.
 * @param[in] address  Register address (e.g. SEC_ADDRESS, MIN_ADDRESS).
 * @return   BCD-encoded register value.
 */
unsigned char ds1307_i2c_read(unsigned char address);

/**
 * @brief   Writes one byte to a DS1307 register.
 * @param[in] data     Value to write (BCD-encoded).
 * @param[in] address  Destination register address.
 */
void ds1307_i2c_write(unsigned char data, unsigned char address);

#endif /* DS1307_H */