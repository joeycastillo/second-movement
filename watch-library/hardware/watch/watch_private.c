/*
 * MIT License
 *
 * Copyright (c) 2020 Joey Castillo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "watch_private.h"
#include "tcc.h"
#include "tc.h"
#include "usb.h"

void _watch_init(void) {
    // disable debugger hot-plugging
    HAL_GPIO_SWCLK_pmuxdis();
    HAL_GPIO_SWCLK_off();

    // RAM should be back-biased in STANDBY
    PM->STDBYCFG.bit.BBIASHS = 1;

    // Use switching regulator for lower power consumption.
    SUPC->VREG.bit.SEL = 1;

    // per Microchip datasheet clarification DS80000782,
    // work around silicon erratum 1.7.2, which causes the microcontroller to lock up on leaving standby:
    // request that the voltage regulator run in standby, and also that it switch to PL0.
    SUPC->VREG.bit.RUNSTDBY = 1;
    SUPC->VREG.bit.STDBYPL0 = 1;
    while(!SUPC->STATUS.bit.VREGRDY); // wait for voltage regulator to become ready

    /** TODO: check the battery voltage...
    watch_enable_adc();
    uint16_t battery_voltage = watch_get_vcc_voltage();
    watch_disable_adc();
    // ...because we can enable the more efficient low power regulator if the system voltage is > 2.5V
    // still, enable LPEFF only if the battery voltage is comfortably above this threshold.
    if (battery_voltage >= 2700) {
        SUPC->VREG.bit.LPEFF = 1;
    } else {
        SUPC->VREG.bit.LPEFF = 0;
    }
 */

    // set up the brownout detector (low battery warning)
    NVIC_DisableIRQ(SYSTEM_IRQn);
    NVIC_ClearPendingIRQ(SYSTEM_IRQn);
    NVIC_EnableIRQ(SYSTEM_IRQn);
    SUPC->BOD33.bit.ENABLE = 0;     // BOD33 must be disabled to change its configuration
    SUPC->BOD33.bit.VMON = 0;       // Monitor VDD in active and standby mode
    SUPC->BOD33.bit.ACTCFG = 1;     // Enable sampling mode when active
    SUPC->BOD33.bit.RUNSTDBY = 1;   // Enable sampling mode in standby
    SUPC->BOD33.bit.STDBYCFG = 1;   // Run in standby
    SUPC->BOD33.bit.RUNBKUP = 0;    // Don't run in backup mode
    SUPC->BOD33.bit.PSEL = 0x9;     // Check battery level every second (we'll change this before entering sleep)
    SUPC->BOD33.bit.LEVEL = 34;     // Detect brownout at 2.6V (1.445V + level * 34mV)
    SUPC->BOD33.bit.ACTION = 0x2;   // Generate an interrupt when BOD33 is triggered
    SUPC->BOD33.bit.HYST = 0;       // Disable hysteresis
    while(!SUPC->STATUS.bit.B33SRDY); // wait for BOD33 to sync

    // Enable interrupt on BOD33 detect
    SUPC->INTENSET.bit.BOD33DET = 1;
    SUPC->BOD33.bit.ENABLE = 1;

    /// start the real-time clock
    _watch_rtc_init();

    /// set up callbacks for low energy mode
    btn_alarm_callback = NULL;
    a2_callback = NULL;
    a4_callback = NULL;
}

static inline void _watch_wait_for_entropy() {
    while (!TRNG->INTFLAG.bit.DATARDY);
}

static inline void watch_disable_TRNG(void) {
    // per Microchip datasheet clarification DS80000782,
    // silicon erratum 1.16.1 indicates that the TRNG may leave internal components powered after being disabled.
    // the workaround is to disable the TRNG by clearing the control register, twice.
    TRNG->CTRLA.bit.ENABLE = 0;
    TRNG->CTRLA.bit.ENABLE = 0;
}

// this function is called by arc4random to get entropy for random number generation.
int getentropy(void *buf, size_t buflen);

// let's use the SAM L22's true random number generator to seed the PRNG!
int getentropy(void *buf, size_t buflen) {
    MCLK->APBCMASK.bit.TRNG_ = 1;
    TRNG->CTRLA.bit.ENABLE = 1;

    size_t i = 0;
    while(i < buflen / 4) {
        _watch_wait_for_entropy();
        ((uint32_t *)buf)[i++] = TRNG->DATA.reg;
    }

    // but what if they asked for an awkward number of bytes?
    if (buflen % 4) {
        // all good: let's fill in one, two or three bytes at the end of the buffer.
        _watch_wait_for_entropy();
        uint32_t last_little_bit = TRNG->DATA.reg;
        for(size_t j = 0; j <= (buflen % 4); j++) {
            ((uint8_t *)buf)[i * 4 + j] = (last_little_bit >> (j * 8)) & 0xFF;
        }
    }

    watch_disable_TRNG();
    MCLK->APBCMASK.bit.TRNG_ = 0;

    return 0;
}

void _watch_enable_tcc(void) {
    // set up the TCC with a 1 MHz clock, but there's a trick:
    if (USB->DEVICE.CTRLA.bit.ENABLE) {
        // if USB is enabled, we are running an 8 MHz clock, so we divide by 8.
        tcc_init(0, GENERIC_CLOCK_0, TCC_PRESCALER_DIV8);
    } else {
        // otherwise it's 4 Mhz and we divide by 4.
        tcc_init(0, GENERIC_CLOCK_0, TCC_PRESCALER_DIV4);
    }
    // We're going to use normal PWM mode, which means period is controlled by PER, and duty cycle is controlled by
    // each compare channel's value:
    //  * Buzzer tones are set by setting PER to the desired period for a given frequency, and CC[1] to half of that
    //    period (i.e. a square wave with a 50% duty cycle).
    //  * LEDs on CC[0] CC[2] and CC[3] can be set to any value from 0 (off) to PER (fully on).
    tcc_set_wavegen(0, TCC_WAVEGEN_NORMAL_PWM);
#ifdef WATCH_INVERT_LED_POLARITY
    // invert all channels, we'll flip the buzzer back in just a moment.
    // this is easier than writing a maze of #ifdefs.
    tcc_set_channel_polarity(0, 4, TCC_CHANNEL_POLARITY_INVERTED);
    tcc_set_channel_polarity(0, 5, TCC_CHANNEL_POLARITY_INVERTED);
    tcc_set_channel_polarity(0, 6, TCC_CHANNEL_POLARITY_INVERTED);
    tcc_set_channel_polarity(0, 7, TCC_CHANNEL_POLARITY_INVERTED);
#endif // WATCH_INVERT_LED_POLARITY
    tcc_set_channel_polarity(0, WATCH_BUZZER_TCC_CHANNEL, TCC_CHANNEL_POLARITY_NORMAL);

    // Set the period to 1 kHz to start.
    tcc_set_period(0, 1000, false);

    // Set the duty cycle of all pins to 0: LED's off, buzzer not buzzing.
    tcc_set_cc(0, (WATCH_BUZZER_TCC_CHANNEL) % 4, 0, false);
    tcc_set_cc(0, (WATCH_RED_TCC_CHANNEL) % 4, 0, false);
#ifdef WATCH_GREEN_TCC_CHANNEL
    tcc_set_cc(0, (WATCH_GREEN_TCC_CHANNEL) % 4, 0, false);
#endif
#ifdef WATCH_BLUE_TCC_CHANNEL
    tcc_set_cc(0, (WATCH_BLUE_TCC_CHANNEL) % 4, 0, false);
#endif
    // Enable the TCC
    tcc_enable(0);

    // enable LED PWM pins (the LED driver assumes if the TCC is on, the pins are enabled)
    HAL_GPIO_RED_out();
    HAL_GPIO_RED_pmuxen(HAL_GPIO_PMUX_TCC_ALT);
#ifdef WATCH_GREEN_TCC_CHANNEL
    HAL_GPIO_GREEN_out();
    HAL_GPIO_GREEN_pmuxen(HAL_GPIO_PMUX_TCC_ALT);
#endif
#ifdef WATCH_BLUE_TCC_CHANNEL
    HAL_GPIO_BLUE_out();
    HAL_GPIO_BLUE_pmuxen(HAL_GPIO_PMUX_TCC_ALT);
#endif
}

void _watch_disable_tcc(void) {
    // disable all PWM pins
    HAL_GPIO_BUZZER_pmuxdis();
    HAL_GPIO_BUZZER_off();
    HAL_GPIO_RED_pmuxdis();
    HAL_GPIO_RED_off();
#ifdef WATCH_GREEN_TCC_CHANNEL
    HAL_GPIO_GREEN_pmuxdis();
    HAL_GPIO_GREEN_off();
#endif
#ifdef WATCH_BLUE_TCC_CHANNEL
    HAL_GPIO_BLUE_pmuxdis();
    HAL_GPIO_BLUE_off();
#endif
    tcc_disable(0);
}

void _watch_enable_usb(void) {
    usb_init();
    usb_enable();
}
