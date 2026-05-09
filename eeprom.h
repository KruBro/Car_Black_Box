/**
 * @file    eeprom.h
 * @brief   AT24C02/AT24C04 external EEPROM — circular log driver.
 *
 *   Record format (11 ASCII bytes per entry, no null terminator stored):
 *
 *     "HH MM SS Gx SSS"  →  e.g. "235959G3085"
 *      ^^         hours   (2 digits, BCD decoded)
 *        ^^       minutes (2 digits, BCD decoded)
 *          ^^     seconds (2 digits, BCD decoded)
 *            ^    literal 'G' prefix
 *             ^   gear digit (0=R, 1=N, 2=1st … 5=4th)  — or 'C' on crash
 *              ^^^ speed  (3 digits, 000–100)
 *
 *   Memory layout:
 *
 *     Addr 0x00  head  (1 byte — next write slot, 0–9)
 *     Addr 0x01  count (1 byte — valid entries,   0–10)
 *     Addr 0x02  [slot 0]  11 bytes
 *     Addr 0x0D  [slot 1]  11 bytes
 *     ...
 *     Addr 0x68  [slot 9]  11 bytes  (last byte at 0x72)
 *
 *   Total used: 2 + (10 * 11) = 112 bytes — fits any 24Cxx EEPROM.
 */

#ifndef EEPROM_H
#define EEPROM_H

#include "main_config.h"

/* -------------------------------------------------------------------------
 * EEPROM chip selection
 * ------------------------------------------------------------------------- */
#define EEPROM_TYPE         4           // 2 = AT24C02 (256 B), 4 = AT24C04 (512 B)

/* I2C address bytes (A2=A1=A0=0 on PICGENIOS) */
#define EEPROM_WRITE_BASE   0xA0U       // 0b10100000
#define EEPROM_READ_BASE    0xA1U       // 0b10100001

#define EEPROM_POLL_TIMEOUT 50U         // ACK poll retries after a write

/* -------------------------------------------------------------------------
 * Log layout constants
 * ------------------------------------------------------------------------- */
#define LOG_MAX_ENTRIES     10U         // Circular buffer depth
#define LOG_RECORD_SIZE     11U         // Bytes per entry (ASCII, no null)

#define LOG_HEAD_ADDR       0x00U       // Header byte 0: next-write slot index
#define LOG_COUNT_ADDR      0x01U       // Header byte 1: valid entry count
#define LOG_DATA_START      0x02U       // First record starts here

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/**
 * @brief  Writes one LOG_ENTRY to the next circular slot.
 *         Oldest entry is silently overwritten once 10 entries are stored.
 * @return 1 on success, 0 if an I2C write failed.
 */
unsigned char eeprom_write_log(const LOG_ENTRY *entry);

/**
 * @brief  Reads the entry at logical index 0 (oldest) … count-1 (newest).
 *         Fills the provided 12-byte buffer as a null-terminated string:
 *         e.g. "235959G3085\0"
 * @param[in]  index   0 = oldest entry, count-1 = newest.
 * @param[out] buf     Caller-supplied buffer, must be >= 12 bytes.
 */
void eeprom_read_log(unsigned char index, char *buf);

/** @brief  Returns the number of valid log entries currently stored (0–10). */
unsigned char eeprom_get_entry_count(void);

/** @brief  Erases the log by resetting head and count to zero. */
unsigned char eeprom_clear_log(void);

/* Low-level byte access — available for other drivers if needed */
unsigned char eeprom_write_byte(unsigned int addr, unsigned char data);
unsigned char eeprom_read_byte(unsigned int addr);

#endif /* EEPROM_H */