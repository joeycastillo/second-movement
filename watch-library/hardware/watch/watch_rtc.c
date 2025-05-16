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

#include <stddef.h>

#include "watch_rtc.h"
#include "watch_private.h"

watch_cb_t tick_callbacks[8];
watch_cb_t alarm_callback;
watch_cb_t btn_alarm_callback;
watch_cb_t a2_callback;
watch_cb_t a4_callback;

void watch_rtc_callback(uint16_t interrupt_status);

bool _watch_rtc_is_enabled(void) {
    return rtc_is_enabled();
}

void _watch_rtc_init(void) {
    rtc_init();
#ifdef STATIC_FREQCORR
    watch_rtc_freqcorr_write(STATIC_FREQCORR, 0);
#endif
    rtc_enable();
    rtc_configure_callback(watch_rtc_callback);
}

void watch_rtc_set_date_time(rtc_date_time_t date_time) {
    rtc_set_date_time(date_time);
}

rtc_date_time_t watch_rtc_get_date_time(void) {
    return rtc_get_date_time();
}

void watch_rtc_register_tick_callback(watch_cb_t callback) {
    watch_rtc_register_periodic_callback(callback, 1);
}

void watch_rtc_disable_tick_callback(void) {
    watch_rtc_disable_periodic_callback(1);
}

void watch_rtc_register_periodic_callback(watch_cb_t callback, uint8_t frequency) {
    // we told them, it has to be a power of 2.
    if (__builtin_popcount(frequency) != 1) return;

    // this left-justifies the period in a 32-bit integer.
    uint32_t tmp = (frequency & 0xFF) << 24;
    // now we can count the leading zeroes to get the value we need.
    // 0x01 (1 Hz) will have 7 leading zeros for PER7. 0x80 (128 Hz) will have no leading zeroes for PER0.
    uint8_t per_n = __builtin_clz(tmp);

    // this also maps nicely to an index for our list of tick callbacks.
    tick_callbacks[per_n] = callback;

    NVIC_ClearPendingIRQ(RTC_IRQn);
    NVIC_EnableIRQ(RTC_IRQn);
    RTC->MODE2.INTENSET.reg = 1 << per_n;
}

void watch_rtc_disable_periodic_callback(uint8_t frequency) {
    if (__builtin_popcount(frequency) != 1) return;
    uint8_t per_n = __builtin_clz((frequency & 0xFF) << 24);
    RTC->MODE2.INTENCLR.reg = 1 << per_n;
}

void watch_rtc_disable_matching_periodic_callbacks(uint8_t mask) {
    RTC->MODE2.INTENCLR.reg = mask;
}

void watch_rtc_disable_all_periodic_callbacks(void) {
    watch_rtc_disable_matching_periodic_callbacks(0xFF);
}

void watch_rtc_register_alarm_callback(watch_cb_t callback, rtc_date_time_t alarm_time, rtc_alarm_match_t mask) {
    RTC->MODE2.Mode2Alarm[0].ALARM.reg = alarm_time.reg;
    RTC->MODE2.Mode2Alarm[0].MASK.reg = mask;
    RTC->MODE2.INTENSET.reg = RTC_MODE2_INTENSET_ALARM0;
    alarm_callback = callback;
    NVIC_ClearPendingIRQ(RTC_IRQn);
    NVIC_EnableIRQ(RTC_IRQn);
    RTC->MODE2.INTENSET.reg = RTC_MODE2_INTENSET_ALARM0;
}

void watch_rtc_disable_alarm_callback(void) {
    RTC->MODE2.INTENCLR.reg = RTC_MODE2_INTENCLR_ALARM0;
}

void watch_rtc_callback(uint16_t interrupt_status) {
    uint16_t interrupt_enabled = RTC->MODE2.INTENSET.reg;

    if ((interrupt_status & interrupt_enabled) & RTC_MODE2_INTFLAG_PER_Msk) {
        // handle the tick callback first, it's what we do the most.
        // start from PER7, the 1 Hz tick.
        for(int8_t i = 7; i >= 0; i--) {
            if ((interrupt_status & interrupt_enabled) & (1 << i)) {
                if (tick_callbacks[i] != NULL) {
                    tick_callbacks[i]();
                }
                RTC->MODE2.INTFLAG.reg = 1 << i;
//                break; Uncertain if this fix is requried. We were discussing in discord. Might slightly increase power consumption. 
            }
        }
    } else if ((interrupt_status & interrupt_enabled) & RTC_MODE2_INTFLAG_TAMPER) {
        // handle the extwake interrupts next.
        uint8_t reason = RTC->MODE2.TAMPID.reg;
        if (reason & RTC_TAMPID_TAMPID2) {
            if (btn_alarm_callback != NULL) btn_alarm_callback();
        } else if (reason & RTC_TAMPID_TAMPID1) {
            if (a2_callback != NULL) a2_callback();
        } else if (reason & RTC_TAMPID_TAMPID0) {
            if (a4_callback != NULL) a4_callback();
        }
        RTC->MODE2.TAMPID.reg = reason;
        RTC->MODE2.INTFLAG.reg = RTC_MODE2_INTFLAG_TAMPER;
    } else if ((interrupt_status & interrupt_enabled) & RTC_MODE2_INTFLAG_ALARM0) {
        // finally handle the alarm.
        if (alarm_callback != NULL) {
            alarm_callback();
        }
        RTC->MODE2.INTFLAG.reg = RTC_MODE2_INTFLAG_ALARM0;
    }
}

void watch_rtc_enable(bool en) {
    // Writing it twice - as it's quite dangerous operation.
    // If write fails - we might hang with RTC off, which means no recovery possible
    while (RTC->MODE2.SYNCBUSY.reg);
    RTC->MODE2.CTRLA.bit.ENABLE = en ? 1 : 0;
    while (RTC->MODE2.SYNCBUSY.reg);
    RTC->MODE2.CTRLA.bit.ENABLE = en ? 1 : 0;
    while (RTC->MODE2.SYNCBUSY.reg);
}

void watch_rtc_freqcorr_write(int16_t value, int16_t sign) {
    RTC_FREQCORR_Type data;

    data.bit.VALUE = value;
    data.bit.SIGN = sign;

    RTC->MODE2.FREQCORR.reg = data.reg; // Setting correction in single write operation

    // We do not sycnronize. We are not in a hurry
}

