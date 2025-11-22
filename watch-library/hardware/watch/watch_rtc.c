/*
 * MIT License
 *
 * Copyright (c) 2020 Joey Castillo
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
#include <limits.h>

#include "watch_rtc.h"
#include "watch_private.h"
#include "watch_utility.h"

static const uint32_t RTC_OSC_DIV = 10;
static const uint32_t RTC_OSC_HZ = 1 << RTC_OSC_DIV; // 2^10 = 1024
static const uint32_t RTC_PRESCALER_DIV = 3;
static const uint32_t RTC_CNT_HZ = RTC_OSC_HZ >> RTC_PRESCALER_DIV; // 1024 / 2^3 = 128
static const uint32_t RTC_CNT_SUBSECOND_MASK = RTC_CNT_HZ - 1;
static const uint32_t RTC_CNT_DIV = RTC_OSC_DIV - RTC_PRESCALER_DIV; // 7
static const uint32_t RTC_CNT_TICKS_PER_MINUTE = RTC_CNT_HZ * 60;
static const uint32_t RTC_CNT_TICKS_PER_HOUR = RTC_CNT_TICKS_PER_MINUTE * 60;

static const uint32_t RTC_COMP_GRACE_PERIOD = 4;

static const int TB_BKUP_REG = 7;

#define WATCH_RTC_N_COMP_CB 8

typedef struct {
    volatile uint32_t counter;
    volatile watch_cb_t callback;
    volatile bool enabled;
} comp_cb_t;

volatile uint32_t scheduled_comp_counter;

watch_cb_t tick_callbacks[8];
comp_cb_t comp_callbacks[WATCH_RTC_N_COMP_CB];
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

    for (uint8_t index = 0; index < WATCH_RTC_N_COMP_CB; ++index) {
        comp_callbacks[index].counter = 0;
        comp_callbacks[index].callback = NULL;
        comp_callbacks[index].enabled = false;
    }

    scheduled_comp_counter = 0;

    NVIC_ClearPendingIRQ(RTC_IRQn);
    NVIC_EnableIRQ(RTC_IRQn);
}

void watch_rtc_set_date_time(rtc_date_time_t date_time) {
    watch_rtc_set_unix_time(watch_utility_date_time_to_unix_time(date_time, 0));
}

rtc_date_time_t watch_rtc_get_date_time(void) {
    static struct {
        unix_timestamp_t timestamp;
        rtc_date_time_t datetime;
    } cached_date_time = {.datetime.reg=0, .timestamp=0};

    unix_timestamp_t timestamp = watch_rtc_get_unix_time();

    if (timestamp != cached_date_time.timestamp) {
        cached_date_time.timestamp = timestamp;
        cached_date_time.datetime = watch_utility_date_time_from_unix_time(timestamp, 0);
    }

    return cached_date_time.datetime;
}

void watch_rtc_set_unix_time(unix_timestamp_t unix_time) {
    /* unix_time = time_backup + counter / RTC_CNT_HZ - 0.5
     *
     * Because of the way the hardware is designed, the periodic interrupts fire at the subsecond tick values
     * according to the table below (for a 128Hz counter).
     * since the 1Hz periodic interrupt is the most important, we shift the conversion from counter to timestamp by 64 ticks,
     * so that the second changes at the top of the 1Hz interrupt. Hence the 0.5 factor in the equation above.
     * 1Hz:   64
     * 2Hz:   32, 96
     * 4Hz:   16, 48, 80, 112
     * 8Hz:   8, 24, 40, 56, 72, 88, 104, 120
     * 16Hz:  4, 12, 20, ..., 124
     * 32Hz:  2, 6, 10, ..., 126
     * 64Hz:  1, 3, 5, ..., 127
     * 128Hz: 0, 1, 2, ..., 127
    */
    rtc_counter_t counter = rtc_get_counter();
    unix_timestamp_t tb = unix_time - (counter >> RTC_CNT_DIV) - ((counter & RTC_CNT_SUBSECOND_MASK) >> (RTC_CNT_DIV - 1)) + 1;
    watch_store_backup_data(tb, TB_BKUP_REG);
}

unix_timestamp_t watch_rtc_get_unix_time(void) {
    // unix_time = time_backup + counter / RTC_CNT_HZ - 0.5
    rtc_counter_t counter = rtc_get_counter();
    unix_timestamp_t tb = watch_get_backup_data(TB_BKUP_REG);
    return tb + (counter >> RTC_CNT_DIV) + ((counter & RTC_CNT_SUBSECOND_MASK) >> (RTC_CNT_DIV - 1)) - 1;
}

rtc_counter_t watch_rtc_get_counter(void) {
    return rtc_get_counter();
}

uint32_t watch_rtc_get_frequency(void) {
    return RTC_CNT_HZ;
}

uint32_t watch_rtc_get_ticks_per_minute(void) {
    return RTC_CNT_TICKS_PER_MINUTE;
}

rtc_date_time_t watch_get_init_date_time(void) {
    rtc_date_time_t date_time;
#ifdef BUILD_YEAR
    date_time.unit.year = BUILD_YEAR;
#else
    date_time.unit.year = 5;
#endif
#ifdef BUILD_MONTH
    date_time.unit.month = BUILD_MONTH;
#else
    date_time.unit.month = 1;
#endif
#ifdef BUILD_DAY
    date_time.unit.day = BUILD_DAY;
#else
    date_time.unit.day = 1;
#endif
#ifdef BUILD_HOUR
    date_time.unit.hour = BUILD_HOUR;
#endif
#ifdef BUILD_MINUTE
    date_time.unit.minute = BUILD_MINUTE;
#endif
    return date_time;
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

    // NVIC_ClearPendingIRQ(RTC_IRQn);
    // NVIC_EnableIRQ(RTC_IRQn);
    RTC->MODE0.INTENSET.reg = 1 << per_n;
}

void watch_rtc_disable_periodic_callback(uint8_t frequency) {
    if (__builtin_popcount(frequency) != 1) return;
    uint8_t per_n = __builtin_clz((frequency & 0xFF) << 24);
    RTC->MODE0.INTENCLR.reg = 1 << per_n;
}

void watch_rtc_disable_matching_periodic_callbacks(uint8_t mask) {
    RTC->MODE0.INTENCLR.reg = mask;
}

void watch_rtc_disable_all_periodic_callbacks(void) {
    watch_rtc_disable_matching_periodic_callbacks(0xFF);
}

void watch_rtc_schedule_next_comp(void) {
    rtc_counter_t curr_counter = watch_rtc_get_counter();

    // We want to ensure we never miss any registered callbacks,
    // so if a callback counter has just passed but didn't fire, give it a chance to fire.
    rtc_counter_t lax_curr_counter = curr_counter - RTC_COMP_GRACE_PERIOD;

    bool schedule_any = false;
    rtc_counter_t comp_counter;
    rtc_counter_t min_diff = UINT_MAX;

    for (uint8_t index = 0; index < WATCH_RTC_N_COMP_CB; ++index) {
        if (comp_callbacks[index].enabled) {
            rtc_counter_t diff = comp_callbacks[index].counter - lax_curr_counter;
            if (diff <= min_diff) {
                min_diff = diff;
                comp_counter = comp_callbacks[index].counter;
                schedule_any = true;
            }
        }
    }

    if (schedule_any) {
        // If we are changing the comp counter at the front of the line, don't schedule a comp interrupt for a counter that is too close to now
        if (comp_counter != scheduled_comp_counter) {
            rtc_counter_t earliest_comp_counter = curr_counter + RTC_COMP_GRACE_PERIOD;
            if ((earliest_comp_counter - lax_curr_counter) > (comp_counter - lax_curr_counter)) {
                comp_counter = earliest_comp_counter;
            }
            scheduled_comp_counter = comp_counter;
            rtc_enable_compare_interrupt(comp_counter);
        }
    } else {
        scheduled_comp_counter = lax_curr_counter - RTC_COMP_GRACE_PERIOD;
        rtc_disable_compare_interrupt();
    }
}

void watch_rtc_register_comp_callback(watch_cb_t callback, rtc_counter_t counter, uint8_t index) {
    if (index >= WATCH_RTC_N_COMP_CB) {
        return;
    }

    comp_callbacks[index].counter = counter;
    comp_callbacks[index].callback = callback;
    comp_callbacks[index].enabled = true;

    watch_rtc_schedule_next_comp();
}

void watch_rtc_register_comp_callback_no_schedule(watch_cb_t callback, rtc_counter_t counter, uint8_t index) {
    if (index >= WATCH_RTC_N_COMP_CB) {
        return;
    }

    comp_callbacks[index].counter = counter;
    comp_callbacks[index].callback = callback;
    comp_callbacks[index].enabled = true;
}

void watch_rtc_disable_comp_callback(uint8_t index) {
    if (index >= WATCH_RTC_N_COMP_CB) {
        return;
    }

    comp_callbacks[index].enabled = false;

    watch_rtc_schedule_next_comp();
}

void watch_rtc_disable_comp_callback_no_schedule(uint8_t index) {
    if (index >= WATCH_RTC_N_COMP_CB) {
        return;
    }

    comp_callbacks[index].enabled = false;
}

void watch_rtc_callback(uint16_t interrupt_cause) {
    // First read all relevant registers, to ensure no changes occurr during the callbacks
    rtc_counter_t curr_counter = watch_rtc_get_counter();
    uint16_t interrupt_enabled = (uint16_t)RTC->MODE0.INTENSET.reg;

    if ((interrupt_cause & interrupt_enabled) & RTC_MODE0_INTFLAG_PER_Msk) {
        // handle the tick callback first, it's what we do the most.
        // start from PER7, the 1 Hz tick.
        for(int8_t i = 7; i >= 0; i--) {
            if ((interrupt_cause & interrupt_enabled) & (1 << i)) {
                if (tick_callbacks[i] != NULL) {
                    tick_callbacks[i]();
                }
            }
        }
    }

    if ((interrupt_cause & interrupt_enabled) & RTC_MODE0_INTFLAG_TAMPER) {
        // handle the extwake interrupts next.
        uint8_t reason = RTC->MODE0.TAMPID.reg;
        if (reason & RTC_TAMPID_TAMPID2) {
            if (btn_alarm_callback != NULL) btn_alarm_callback();
        } else if (reason & RTC_TAMPID_TAMPID1) {
            if (a2_callback != NULL) a2_callback();
        } else if (reason & RTC_TAMPID_TAMPID0) {
            if (a4_callback != NULL) a4_callback();
        }
        RTC->MODE0.TAMPID.reg = reason;
    }

    if ((interrupt_cause & interrupt_enabled) & RTC_MODE0_INTFLAG_CMP0) {
        for (uint8_t index = 0; index < WATCH_RTC_N_COMP_CB; ++index) {
            if (comp_callbacks[index].enabled &&
                (curr_counter - comp_callbacks[index].counter) < (RTC_COMP_GRACE_PERIOD * 4)
            ) {
                comp_callbacks[index].enabled = false;
                comp_callbacks[index].callback();
            }
        }
        watch_rtc_schedule_next_comp();
    }

    if ((interrupt_cause & interrupt_enabled) & RTC_MODE0_INTFLAG_OVF) {
        // Handle the overflow of the counter. All we need to do is reset the reference time.
        unix_timestamp_t tb = watch_get_backup_data(TB_BKUP_REG);
        watch_store_backup_data(tb + (UINT_MAX >> RTC_CNT_DIV), TB_BKUP_REG);
    }
}

void watch_rtc_enable(bool en) {
    // Writing it twice - as it's quite dangerous operation.
    // If write fails - we might hang with RTC off, which means no recovery possible
    while (RTC->MODE0.SYNCBUSY.reg);
    RTC->MODE0.CTRLA.bit.ENABLE = en ? 1 : 0;
    while (RTC->MODE0.SYNCBUSY.reg);
    RTC->MODE0.CTRLA.bit.ENABLE = en ? 1 : 0;
    while (RTC->MODE0.SYNCBUSY.reg);
}

void watch_rtc_freqcorr_write(int16_t value, int16_t sign) {
    RTC_FREQCORR_Type data;

    data.bit.VALUE = value;
    data.bit.SIGN = sign;

    RTC->MODE0.FREQCORR.reg = data.reg; // Setting correction in single write operation

    // We do not sycnronize. We are not in a hurry
}
