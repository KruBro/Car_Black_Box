/**
 * @file    uart.c
 * @author  krubro
 * @date    2026-05-05
 * @brief   USART driver implementation for PIC16F877A (PICGENIOS / PICSIMSLAB).
 *
 * @details
 *   Configures the USART for asynchronous 8-N-1 operation.
 *   The PIC16F877A has a standard USART (not the Enhanced EUSART found on
 *   PIC18 devices), but the register names used here (SPEN, SYNC, CREN, BRGH,
 *   SPBRG, TXEN) are identical, so no code changes are required.
 *   Functions are prefixed `uart_` to avoid name collisions with the
 *   C standard library (<stdio.h>: getchar, putchar, puts).
 *
 *   Baud rate formula (BRGH=1, SYNC=0):
 *     SPBRG = (FOSC / (16 * baud)) - 1
 */

#include "main_config.h"   /* Provides FOSC */
#include "uart.h"
#include <xc.h>

/* =========================================================================
 * Public Function Implementations
 * ========================================================================= */

/**
 * @brief   Initialises the USART in asynchronous mode.
 *
 *   Register summary:
 *     SPEN  = 1  — Enable serial port (RC6/TX, RC7/RX become serial pins).
 *     SYNC  = 0  — Asynchronous mode.
 *     CREN  = 1  — Enable continuous receive.
 *     BRGH  = 1  — High-speed baud rate mode.
 *     TXEN  = 1  — Enable transmitter.
 *     SPBRG      — Baud Rate Generator register.
 *
 *   These register names are identical on the PIC16F877A USART and the
 *   PIC18 EUSART — no code differences are required.
 */
void initUART(unsigned long baud)
{
    SPEN  = 1;                                  /* Enable serial port pins  */
    SYNC  = 0;                                  /* Asynchronous mode        */
    CREN  = 1;                                  /* Continuous receive enable */
    BRGH  = 1;                                  /* High baud rate mode      */
    SPBRG = (unsigned char)((FOSC / (16UL * baud)) - 1UL); /* Baud divider */
    TXEN  = 1;                                  /* Enable transmitter       */
}

/**
 * @brief   Receives one byte from the UART hardware RX buffer.
 *
 *   Error handling:
 *     OERR (Overrun)  — Clears by toggling CREN; sends 'X' as a debug marker.
 *     FERR (Framing)  — Clears by reading RCREG; returns 0.
 */
unsigned char uart_getchar(void)
{
    if (OERR)
    {
        /* Overrun: reset the continuous-receive enable to clear the error   */
        uart_putchar('X');          /* Debug marker visible on serial monitor */
        CREN = 0;
        CREN = 1;
    }

    if (FERR)
    {
        /* Framing error: consume the bad byte to clear FERR                 */
        uart_putchar('F');          /* Debug marker visible on serial monitor */
        (void)RCREG;                /* Dummy read — result intentionally ignored */
        return 0;
    }

    while (!RCIF);                  /* Block until a byte is available       */
    return RCREG;
}

/**
 * @brief   Transmits one byte over UART, blocking until the shift register
 *          is ready to accept a new byte.
 */
void uart_putchar(unsigned char ch)
{
    while (!TXIF);                  /* Wait until TX shift register is empty */
    TXREG = ch;
}

/**
 * @brief   Transmits every character of a null-terminated string over UART.
 */
void uart_puts(const char *str)
{
    while (*str != '\0')
    {
        uart_putchar((unsigned char)*str);
        str++;
    }
}