/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
 * Copyright (c) 2025 Alessandro Genova
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

#include <stddef.h>
#include "rtc32.h"
#include "sam.h"

rtc_cb_t _rtc_callback = NULL;

#if defined(_SAMD21_) || defined(_SAMD11_)
#define CTRLREG (RTC->MODE0.CTRL)
#define MODE_SETTING (RTC_MODE0_CTRL_MODE_COUNT32_Val) // Mode 0 Count32
#define PRESCALER_SETTING (RTC_MODE0_CTRL_PRESCALER_DIV8_Val)
#else
#define CTRLREG (RTC->MODE0.CTRLA)
#define MODE_SETTING (RTC_MODE0_CTRLA_MODE_COUNT32_Val) // Mode 0 Count32
#define PRESCALER_SETTING (RTC_MODE0_CTRLA_PRESCALER_DIV8_Val)
#endif

bool rtc_is_enabled(void) {
    return CTRLREG.bit.ENABLE;
}

static void _rtc_sync(void) {
#if defined(_SAMD21_) || defined(_SAMD11_)
    while (RTC->MODE0.STATUS.bit.SYNCBUSY);
#else
    while (RTC->MODE0.SYNCBUSY.reg & RTC_MODE0_SYNCBUSY_MASK);
#endif
}

void rtc_init(void) {
#if defined(_SAMD21_) || defined(_SAMD11_)
    // enable the RTC
    PM->APBAMASK.reg |= PM_APBAMASK_RTC;
    // clock RTC with GCLK3 (prescaled 1024 Hz output from the external crystal)
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_GEN(3) | GCLK_CLKCTRL_ID(RTC_GCLK_ID) | GCLK_CLKCTRL_CLKEN;
#else
    MCLK->APBAMASK.reg |= MCLK_APBAMASK_RTC;
#endif

    // if (rtc_is_enabled()) return; // don't reset the RTC if it's already set up.
    // Reset everything, once things are stabilized we can think about preserving some state
    CTRLREG.bit.ENABLE = 0;

    _rtc_sync();
    CTRLREG.bit.SWRST = 1;
    _rtc_sync();

    CTRLREG.bit.MODE = MODE_SETTING;
    CTRLREG.bit.PRESCALER = PRESCALER_SETTING;

#if defined(_SAML21_) || defined(_SAML22_) || defined(_SAMD51_)
    CTRLREG.bit.COUNTSYNC = 1;
#endif

    RTC->MODE0.INTENSET.reg = RTC_MODE0_INTENSET_OVF;
}

void rtc_enable(void) {
    if (rtc_is_enabled()) return;
    CTRLREG.bit.ENABLE = 1;
    _rtc_sync();
}

void rtc_set_counter(rtc_counter_t counter) {
    // // syncing before and after was found to increase reliability on Sensor Watch
    _rtc_sync();
    RTC->MODE0.COUNT.reg = counter;
    _rtc_sync();
}

rtc_counter_t rtc_get_counter(void) {
    rtc_counter_t counter;

#if defined(_SAML21_) || defined(_SAML22_) || defined(_SAMD51_)
    CTRLREG.bit.COUNTSYNC = 1;
#endif
    _rtc_sync();
    counter = RTC->MODE0.COUNT.reg;

    return counter;
}

void rtc_enable_compare_interrupt(uint32_t compare_time) {
    RTC->MODE0.COMP[0].reg = compare_time;
    _rtc_sync();
    RTC->MODE0.INTENSET.reg = RTC_MODE0_INTENSET_CMP0;
    // NVIC_ClearPendingIRQ(RTC_IRQn);
    // NVIC_EnableIRQ(RTC_IRQn);
}

void rtc_configure_callback(rtc_cb_t callback) {
    _rtc_callback = callback;
}

void rtc_disable_compare_interrupt(void){
    RTC->MODE0.INTENCLR.reg = RTC_MODE0_INTENCLR_CMP0;
    // NVIC_ClearPendingIRQ(RTC_IRQn);
    // NVIC_DisableIRQ(RTC_IRQn);
}

void irq_handler_rtc(void);

void irq_handler_rtc(void) {
    uint16_t int_cause = (uint16_t)RTC->MODE0.INTFLAG.reg;
    RTC->MODE0.INTFLAG.reg = RTC_MODE0_INTFLAG_MASK;
    (void)RTC->MODE0.INTFLAG.reg;

    /* Invoke registered Callback function */
    if (_rtc_callback != NULL) {
        _rtc_callback(int_cause);
    }

    // NVIC_ClearPendingIRQ(RTC_IRQn);
}
