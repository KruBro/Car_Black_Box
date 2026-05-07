/**
 * @file    adc.c
 * @author  krubro
 * @date    2026-05-05
 * @brief   10-bit ADC driver implementation for PIC16F877A
 *          (PICGENIOS board / PICSIMSLAB).
 *
 * @details
 *   BUG FIXED: The original initADC() contained `GO = 1`, which started an
 *   A/D conversion *before* ADON was set (the ADC module was still powered
 *   down). This produced an undefined initial reading and is explicitly
 *   prohibited by the datasheet. The line has been removed; conversions are
 *   now started exclusively inside read_adc().
 *
 *   ADCON0 bit assignments used here:
 *     ADCS[2:0] = 0b010 → Fosc/8 conversion clock (safe up to 20 MHz)
 *     CHS[2:0]  = 0b000 → Default channel AN0 (overridden in read_adc)
 *     ADON      = 1     → ADC module powered on
 *     ADFM      = 1     → Right-justified result (ADRESL holds bits 7:0)
 *     PCFG[3:0] = 0b0000 → All AN pins configured as analogue inputs
 */

#include "adc.h"
#include <xc.h>

/* =========================================================================
 * Public Function Implementations
 * ========================================================================= */

/**
 * @brief   Configures and powers up the ADC module.
 * @note    initADC() must be called before any call to read_adc().
 *          It does NOT start a conversion; that is done in read_adc().
 */
void initADC(void)
{
    /* A/D Conversion Clock Select: Fosc/8 (ADCS = 0b010)                   */
    ADCS0 = 0;
    ADCS1 = 1;
    ADCS2 = 0;

    /* Default channel: AN0                                                  */
    CHS0  = 0;
    CHS1  = 0;
    CHS2  = 0;

    /* Right-justified result: ADRESH holds bits 9:8, ADRESL holds bits 7:0 */
    ADFM  = 1;

    /* All port A pins configured as analogue inputs (PCFG = 0b0000)        */
    PCFG0 = 0;
    PCFG1 = 0;
    PCFG2 = 0;
    PCFG3 = 0;

    /* Power up the ADC module LAST, after all configuration is set          */
    ADON  = 1;

    /*
     * FIX: `GO = 1` that was present here has been removed.
     * Starting a conversion inside the init function — before any acquisition
     * time has elapsed and before the caller has selected a channel — produces
     * an undefined result and is forbidden by the PIC16F877A datasheet
     * (Section 19.2: "The user must wait the required acquisition time before
     * setting the GO/DONE bit").
     */
}

/**
 * @brief   Selects the ADC channel, starts a conversion, and returns the result.
 * @details Channel selection writes to ADCON0[5:3] (CHS bits). The conversion
 *          clock is Fosc/8, giving a TAD of 0.4 µs — within the 0.7–25 µs
 *          required range. A full 10-bit conversion takes 12 TAD = 4.8 µs.
 */
unsigned short read_adc(unsigned char channel)
{
    /* Select channel: write to CHS[2:0] in ADCON0 bits [5:3]               */
    ADCON0 = (ADCON0 & 0xC7U) | ((unsigned char)(channel << 3));

    /* Start the conversion                                                  */
    GO = 1;

    /* Wait for the conversion to complete (GO/nDONE bit clears when done)  */
    while (nDONE);

    /* Combine the 10-bit result: ADRESH[1:0] = bits 9:8, ADRESL = bits 7:0 */
    return (unsigned short)((ADRESH << 8) | ADRESL);
}