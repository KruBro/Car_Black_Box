/**
 * @file    adc.h
 * @author  krubro
 * @date    2026-05-05
 * @brief   Public API for the 10-bit ADC driver on PIC16F877A
 *          (PICGENIOS board / PICSIMSLAB).
 *
 * @details
 *   The PIC16F877A has a 10-bit successive-approximation ADC with up to 8
 *   analogue channels (AN0–AN7). This driver configures it for right-justified
 *   results and an Fosc/8 conversion clock.
 *
 *   Result range: 0 (0 V) to 1023 (Vdd). At Vdd = 5 V, 1 LSB ≈ 4.9 mV.
 */

#ifndef ADC_H
#define ADC_H

/* -------------------------------------------------------------------------
 * Channel Definitions
 * ------------------------------------------------------------------------- */
#define CHANNEL0    0   /**< AN0 — RA0, used for speed potentiometer        */
#define CHANNEL1    1   /**< AN1 — RA1, reserved for future sensor          */

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/**
 * @brief   Initialises the ADC module.
 * @details Configures: Fosc/8 clock, right-justified result, all pins
 *          analogue (PCFG = 0b0000). Does NOT start a conversion.
 */
void initADC(void);

/**
 * @brief   Performs a blocking 10-bit ADC conversion on the selected channel.
 * @param[in] channel  Analogue channel number (CHANNEL0, CHANNEL1, …).
 * @return   10-bit result (0–1023), right-justified.
 */
unsigned short read_adc(unsigned char channel);

#endif /* ADC_H */