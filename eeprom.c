/**
 * @file    eeprom.c
 * @brief   AT24C02/AT24C04 circular log driver — 10-slot, 11-byte ASCII records.
 *
 *   Circular buffer rules:
 *     - head  = slot index (0–9) where the NEXT write will go.
 *     - count = how many valid entries exist (0–10, never exceeds 10).
 *     - When count == 10, the buffer is full. head keeps advancing and
 *       silently overwrites the oldest slot — no special handling needed.
 *     - Read order: oldest = slot[(head - count + 10) % 10], newest = slot[(head-1+10)%10]
 *
 *   Record encoding ("235959G3085"):
 *     bytes 0-1  hours   BCD → two ASCII digits
 *     bytes 2-3  minutes BCD → two ASCII digits
 *     bytes 4-5  seconds BCD → two ASCII digits
 *     byte  6    literal 'G'
 *     byte  7    gear digit: '0'=Reverse '1'=Neutral '2'–'5'=G1–G4, 'C'=crash
 *     bytes 8-10 speed 0–100 → three ASCII digits
 */

#include "main_config.h"

/* =========================================================================
 * Private — chip addressing
 * ========================================================================= */

/* For AT24C04: bit 1 of the device address carries the 9th address bit (A8) */
static unsigned char get_device_addr(unsigned int addr, unsigned char base)
{
#if EEPROM_TYPE == 4
    return (unsigned char)(base | (unsigned char)((addr >> 7) & 0x02U));
#else
    return base;
#endif
}

/* =========================================================================
 * Private — ACK polling (waits for the EEPROM to finish an internal write)
 * ========================================================================= */

static unsigned char ack_poll(unsigned char dev_addr)
{
    unsigned char attempts = 0;
    while (attempts < EEPROM_POLL_TIMEOUT)
    {
        i2c_start();
        if (i2c_write(dev_addr)) { i2c_stop(); return 1; }
        i2c_stop();
        attempts++;
        __delay_us(100);
    }
    return 0;
}

/* =========================================================================
 * Public — low-level byte access
 * ========================================================================= */

unsigned char eeprom_write_byte(unsigned int addr, unsigned char data)
{
    unsigned char dev = get_device_addr(addr, EEPROM_WRITE_BASE);
    i2c_start();
    i2c_write(dev);
    i2c_write((unsigned char)(addr & 0xFFU));
    i2c_write(data);
    i2c_stop();
    return ack_poll(dev);
}

unsigned char eeprom_read_byte(unsigned int addr)
{
    unsigned char data;
    unsigned char dev_w = get_device_addr(addr, EEPROM_WRITE_BASE);
    unsigned char dev_r = get_device_addr(addr, EEPROM_READ_BASE);
    i2c_start();
    i2c_write(dev_w);
    i2c_write((unsigned char)(addr & 0xFFU));
    i2c_repeat_start();
    i2c_write(dev_r);
    data = i2c_read(1);
    i2c_stop();
    return data;
}

/* =========================================================================
 * Private — header helpers (head and count live at bytes 0x00 and 0x01)
 * ========================================================================= */

static unsigned char read_head(void)  { return eeprom_read_byte(LOG_HEAD_ADDR);  }
static unsigned char read_count(void) { return eeprom_read_byte(LOG_COUNT_ADDR); }

/* =========================================================================
 * Private — record encoding
 *
 *   Packs a LOG_ENTRY into the 11-byte ASCII format and writes it to the
 *   physical EEPROM address that corresponds to slot_index.
 *
 *   Slot address = LOG_DATA_START + (slot_index * LOG_RECORD_SIZE)
 * ========================================================================= */

/* Converts a BCD byte (e.g. 0x23) into two ASCII chars ('2', '3') */
static void bcd_to_ascii(unsigned char bcd, char *hi, char *lo)
{
    *hi = (char)(((bcd >> 4) & 0x0FU) + '0');
    *lo = (char)((bcd & 0x0FU) + '0');
}

/* Converts a 0–100 integer to three ASCII chars ('0'–'9') */
static void speed_to_ascii(unsigned char spd, char *h, char *t, char *u)
{
    *h = (char)((spd / 100U) + '0');
    *t = (char)(((spd / 10U) % 10U) + '0');
    *u = (char)((spd % 10U) + '0');
}

static unsigned char write_slot(unsigned char slot_index, const LOG_ENTRY *entry)
{
    char rec[LOG_RECORD_SIZE];  /* 11-byte record buffer — not null-terminated */
    unsigned int addr = LOG_DATA_START + ((unsigned int)slot_index * LOG_RECORD_SIZE);

    /* Pack the record */
    bcd_to_ascii(entry->hours,   &rec[0], &rec[1]);
    bcd_to_ascii(entry->minutes, &rec[2], &rec[3]);
    bcd_to_ascii(entry->seconds, &rec[4], &rec[5]);

    rec[6] = 'G';
    /* Gear character: crash overrides gear with 'C' */
    rec[7] = (entry->flags & FLAG_CRASH)
             ? 'C'
             : (char)('0' + (entry->gear & 0x07U));

    speed_to_ascii(entry->speed, &rec[8], &rec[9], &rec[10]);

    /* Write all 11 bytes — abort on first failure */
    unsigned char i;
    for (i = 0; i < LOG_RECORD_SIZE; i++)
    {
        if (!eeprom_write_byte(addr + i, (unsigned char)rec[i]))
            return 0;
    }
    return 1;
}

/* =========================================================================
 * Public — circular log API
 * ========================================================================= */

/**
 * @brief  Writes entry to slot[head], advances head, clamps count to 10.
 *
 *   Example — buffer full (count=10), head=3:
 *     Write to slot 3 (overwrites oldest).
 *     head becomes 4.  count stays 10.
 *
 *   Example — buffer not full (count=5), head=5:
 *     Write to slot 5.
 *     head becomes 6.  count becomes 6.
 */
unsigned char eeprom_write_log(const LOG_ENTRY *entry)
{
    unsigned char head  = read_head();
    unsigned char count = read_count();

    /* Clamp head in case EEPROM was never initialised (0xFF default) */
    if (head >= LOG_MAX_ENTRIES) head = 0;

    if (!write_slot(head, entry)) return 0;

    /* Advance head with wrap-around */
    head = (unsigned char)((head + 1U) % LOG_MAX_ENTRIES);

    /* Count climbs to 10 and then stays there */
    if (count < LOG_MAX_ENTRIES) count++;

    if (!eeprom_write_byte(LOG_HEAD_ADDR,  head))  return 0;
    if (!eeprom_write_byte(LOG_COUNT_ADDR, count)) return 0;
    return 1;
}

/**
 * @brief  Reads logical entry at `index` into a null-terminated 12-byte string.
 *
 *   index=0 → oldest entry   index=count-1 → newest entry
 *
 *   Physical slot mapping:
 *     oldest_slot = (head - count + 10) % 10
 *     physical    = (oldest_slot + index) % 10
 */
void eeprom_read_log(unsigned char index, char *buf)
{
    unsigned char head  = read_head();
    unsigned char count = read_count();
    unsigned char i;

    if (head  >= LOG_MAX_ENTRIES) head  = 0;
    if (count >  LOG_MAX_ENTRIES) count = LOG_MAX_ENTRIES;

    /* Map logical index to physical slot */
    unsigned char oldest = (unsigned char)((head - count + LOG_MAX_ENTRIES) % LOG_MAX_ENTRIES);
    unsigned char slot   = (unsigned char)((oldest + index) % LOG_MAX_ENTRIES);
    unsigned int  addr   = LOG_DATA_START + ((unsigned int)slot * LOG_RECORD_SIZE);

    /* Read 11 bytes then null-terminate */
    for (i = 0; i < LOG_RECORD_SIZE; i++)
        buf[i] = (char)eeprom_read_byte(addr + i);
    buf[LOG_RECORD_SIZE] = '\0';
}

unsigned char eeprom_get_entry_count(void)
{
    unsigned char count = read_count();
    return (count > LOG_MAX_ENTRIES) ? LOG_MAX_ENTRIES : count;
}

unsigned char eeprom_clear_log(void)
{
    if (!eeprom_write_byte(LOG_HEAD_ADDR,  0U)) return 0;
    if (!eeprom_write_byte(LOG_COUNT_ADDR, 0U)) return 0;
    return 1;
}