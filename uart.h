/**
 * @file    uart.h
 * @author  krubro
 * @date    2026-05-05
 * @brief   Public API for the USART driver on PIC16F877A (PICGENIOS board).
 *
 * @details
 *   All functions are prefixed `uart_` to avoid conflicts with the C standard
 *   library identifiers `getchar`, `putchar`, and `puts` (declared in
 *   <stdio.h>). Using the same names as stdlib functions causes undefined
 *   behaviour at link time and must never be done.
 *
 *   The PIC16F877A features a standard USART (not EUSART). The same register
 *   names (SPEN, SYNC, CREN, BRGH, SPBRG, TXEN, TXIF, RCIF, RCREG, TXREG,
 *   OERR, FERR) apply and are handled identically.
 *
 *   FOSC is intentionally NOT defined here; it is provided by main_config.h,
 *   which is the single source of truth for the oscillator frequency.
 */

#ifndef UART_H
#define UART_H

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/**
 * @brief   Initialises the USART in asynchronous mode.
 * @param[in] baud  Desired baud rate in bits per second (e.g. 9600).
 * @note    Requires FOSC to be defined before including this header
 *          (provided by main_config.h).
 */
void initUART(unsigned long baud);

/**
 * @brief   Receives a single byte from the UART RX buffer.
 * @details Blocks until a byte is available. Handles overrun and framing
 *          errors by resetting CREN and returning 0 respectively.
 * @return  Received byte, or 0 on a framing error.
 */
unsigned char uart_getchar(void);

/**
 * @brief   Transmits a single byte over UART.
 * @details Blocks until the TXREG shift register is empty.
 * @param[in] ch  Byte to transmit.
 */
void uart_putchar(unsigned char ch);

/**
 * @brief   Transmits a null-terminated string over UART.
 * @param[in] str  Pointer to the string to send. Must be null-terminated.
 */
void uart_puts(const char *str);

#endif /* UART_H */