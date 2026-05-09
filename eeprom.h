/**
 * @file    eeprom.h
 * @brief   AT24C02/AT24C04 external EEPROM — circular log driver.
 *
 *   Record format (11 ASCII bytes per entry, no null terminator stored):
 *
 *     "HHMMSSGxSSS"  →  e.g. "235959G3085"
 *      ^^       hours   (2 digits, BCD decoded)
 *        ^^     minutes (2 digits, BCD decoded)
 *          ^^   seconds (2 digits, BCD decoded)
 *            ^  literal 'G' prefix
 *             ^ gear digit (0=R, 1=N, 2=1st … 5=4th) — or 'C' on crash
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
 *     Addr 0xF0–0xF3  PIN password (4 bytes, ASCII '0' or '1')
 *
 *   Total log used: 2 + (10 × 11) = 112 bytes.
 *   Password region: 4 bytes at 0xF0 — safely outside the 112-byte log ring.
 */

#ifndef EEPROM_H
#define EEPROM_H

#include "main_config.h"

/* -------------------------------------------------------------------------
 * EEPROM chip selection
 * ------------------------------------------------------------------------- */
#define EEPROM_TYPE         4           // 2 = AT24C02 (256 B), 4 = AT24C04 (512 B)

// I2C address bytes (A2=A1=A0=0 on PICGENIOS)
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
 * Password storage — 4 bytes at a fixed address outside the log ring.
 * The log ring ends at 0x72 (slot 9 last byte). 0xF0 gives a safe margin.
 * ------------------------------------------------------------------------- */
#define EEPROM_PASS_ADDR    0x00F0U     // Address of the 4-byte PIN (ASCII '0'/'1')

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/** @brief  Writes one LOG_ENTRY to the next circular slot. Returns 1 on success. */
unsigned char eeprom_write_log(const LOG_ENTRY *entry);

/** @brief  Reads entry at logical index into a null-terminated 12-byte string. */
void eeprom_read_log(unsigned char index, char *buf);

/** @brief  Returns the number of valid log entries (0–10). */
unsigned char eeprom_get_entry_count(void);

/** @brief  Erases the log by resetting head and count to zero. */
unsigned char eeprom_clear_log(void);

// Low-level byte access — used by log driver and login_load_password / set_password
unsigned char eeprom_write_byte(unsigned int addr, unsigned char data);
unsigned char eeprom_read_byte(unsigned int addr);

#endif /* EEPROM_H */