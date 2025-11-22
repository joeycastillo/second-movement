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
#include <limits.h>

#include "watch_rtc.h"
#include "watch_main_loop.h"
#include "watch_utility.h"

#include <emscripten.h>
#include <emscripten/html5.h>

static const uint32_t RTC_CNT_HZ = 128;
static const uint32_t RTC_CNT_SUBSECOND_MASK = RTC_CNT_HZ - 1;
static const uint32_t RTC_CNT_DIV = 7;
static const uint32_t RTC_CNT_TICKS_PER_MINUTE = RTC_CNT_HZ * 60;
static const uint32_t RTC_CNT_TICKS_PER_HOUR = RTC_CNT_TICKS_PER_MINUTE * 60;

static uint32_t counter_interval;
static uint32_t counter;
static uint32_t reference_timestamp;

#define WATCH_RTC_N_COMP_CB 8

typedef struct {
    volatile uint32_t counter;
    volatile watch_cb_t callback;
    volatile bool enabled;
} comp_cb_t;

static double time_offset = 0;
watch_cb_t tick_callbacks[8];
comp_cb_t comp_callbacks[WATCH_RTC_N_COMP_CB];

static uint32_t scheduled_comp_counter;

static long alarm_interval_id = -1;
static long alarm_timeout_id = -1;
static double alarm_interval;
watch_cb_t alarm_callback;
watch_cb_t btn_alarm_callback;
watch_cb_t a2_callback;
watch_cb_t a4_callback;

static void _watch_increase_counter(void *userData);
static void _watch_process_periodic_callbacks(void);
static void _watch_process_comp_callbacks(void);

bool _watch_rtc_is_enabled(void) {
    return counter_interval;
}

void _watch_rtc_init(void) {
    for (uint8_t index = 0; index < 8; ++index) {
        tick_callbacks[index] = NULL;
    }

    for (uint8_t index = 0; index < WATCH_RTC_N_COMP_CB; ++index) {
        comp_callbacks[index].counter = 0;
        comp_callbacks[index].callback = NULL;
        comp_callbacks[index].enabled = false;
    }

    scheduled_comp_counter = 0;
    counter = 0;
    counter_interval = 0;

    watch_rtc_set_date_time(watch_get_init_date_time());
    watch_rtc_enable(true);
}

void watch_rtc_set_date_time(rtc_date_time_t date_time) {
    watch_rtc_set_unix_time(watch_utility_date_time_to_unix_time(date_time, 0));
}

rtc_date_time_t watch_rtc_get_date_time(void) {
    return watch_utility_date_time_from_unix_time(watch_rtc_get_unix_time(), 0);
}

void watch_rtc_set_unix_time(unix_timestamp_t unix_time) {
    // unix_time = time_backup + counter / RTC_CNT_HZ - 0.5
    rtc_counter_t counter = watch_rtc_get_counter();
    reference_timestamp = unix_time - (counter >> RTC_CNT_DIV) - ((counter & RTC_CNT_SUBSECOND_MASK) >> (RTC_CNT_DIV - 1)) + 1;
}

unix_timestamp_t watch_rtc_get_unix_time(void) {
    // unix_time = time_backup + counter / RTC_CNT_HZ - 0.5
    rtc_counter_t counter = watch_rtc_get_counter();
    return reference_timestamp + (counter >> RTC_CNT_DIV) + ((counter & RTC_CNT_SUBSECOND_MASK) >> (RTC_CNT_DIV - 1)) - 1;
}

rtc_counter_t watch_rtc_get_counter(void) {
    return counter;
}

uint32_t watch_rtc_get_frequency(void) {
    return RTC_CNT_HZ;
}

uint32_t watch_rtc_get_ticks_per_minute(void) {
    return RTC_CNT_TICKS_PER_MINUTE;
}

rtc_date_time_t watch_get_init_date_time(void) {
    rtc_date_time_t date_time = {0};

    int32_t time_zone_offset = EM_ASM_INT({
        return new Date().getTimezoneOffset() * 60 * 1000; // ms
    });

    date_time.reg = EM_ASM_INT({
        const date = new Date(Date.now() + $0);
        return date.getSeconds() |
            (date.getMinutes() << 6) |
            (date.getHours() << 12) |
            (date.getDate() << 17) |
            ((date.getMonth() + 1) << 22) |
            ((date.getFullYear() - 2020) << 26);
    }, time_zone_offset);

#ifdef BUILD_YEAR
    date_time.unit.year = BUILD_YEAR;
#endif
#ifdef BUILD_MONTH
    date_time.unit.month = BUILD_MONTH;
#endif
#ifdef BUILD_DAY
    date_time.unit.day = BUILD_DAY;
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

static void _watch_increase_counter(void *userData) {
    (void) userData;

    counter += 1;
    // Fire the periodic callbacks that match this counter
    _watch_process_periodic_callbacks();
    // Fire the comp callbacks that match this counter
    _watch_process_comp_callbacks();

    resume_main_loop();
}

static void _watch_process_periodic_callbacks(void) {
    /* It looks weird but it follows the way the hardware triggers periodic interrupts.
     * For 128hz counter periodic interrupts fire at these tick values:
     * 1Hz:   64
     * 2Hz:   32, 96
     * 4Hz:   16, 48, 80, 112
     * 8Hz:   8, 24, 40, 56, 72, 88, 104, 120
     * 16Hz:  4, 12, 20, ..., 124
     * 32Hz:  2, 6, 10, ..., 126
     * 64Hz:  1, 3, 5, ..., 127
     * 128Hz: 0, 1, 2, ..., 127
     * 
     * Which means that only one periodic interrupt can fire for a given counter value
     * (except 128Hz which can always fire)
     */

    uint32_t freq = watch_rtc_get_frequency();
    uint32_t subsecond_mask = freq - 1;
    uint32_t subseconds = counter & subsecond_mask;

    // Find the firs non-zero bit in the counter, which can be used to determine the appropriate period (see table above).
    uint8_t per_n = 0;

    for (uint8_t i = 0; i < 7; i++) {
        if (subseconds & (1 << i)) {
            per_n = i + 1;
            break;
        }
    }

    if (tick_callbacks[per_n]) {
        tick_callbacks[per_n]();
    }

    // 128Hz is always a match
    if (per_n != 0 && tick_callbacks[0]) {
        tick_callbacks[0]();
    }
}

static void _watch_process_comp_callbacks(void) {
    // In hardware the interrupt fires one tick after the matching counter
    if (counter == (scheduled_comp_counter + 1)) {
        for (uint8_t index = 0; index < WATCH_RTC_N_COMP_CB; ++index) {
            if (comp_callbacks[index].enabled && scheduled_comp_counter == comp_callbacks[index].counter) {
                comp_callbacks[index].enabled = false;
                comp_callbacks[index].callback();
            }
        }

        watch_rtc_schedule_next_comp();
    }
}

void watch_rtc_register_periodic_callback(watch_cb_t callback, uint8_t frequency) {
    // we told them, it has to be a power of 2.
    if (__builtin_popcount(frequency) != 1) return;

    // this left-justifies the period in a 32-bit integer.
    uint32_t tmp = (frequency & 0xFF) << 24;
    // now we can count the leading zeroes to get the value we need.
    // 0x01 (1 Hz) will have 7 leading zeros for PER7. 0xF0 (128 Hz) will have no leading zeroes for PER0.
    uint8_t per_n = __builtin_clz(tmp);

    tick_callbacks[per_n] = callback;
}

void watch_rtc_disable_periodic_callback(uint8_t frequency) {
    if (__builtin_popcount(frequency) != 1) return;
    uint8_t per_n = __builtin_clz((frequency & 0xFF) << 24);
    tick_callbacks[per_n] = NULL;
}

void watch_rtc_disable_matching_periodic_callbacks(uint8_t mask) {
    for (int i = 0; i < 8; i++) {
        if (tick_callbacks[i] && (mask & (1 << i)) != 0) {
            tick_callbacks[i] = NULL;
        }
    }
}

void watch_rtc_disable_all_periodic_callbacks(void) {
    watch_rtc_disable_matching_periodic_callbacks(0xFF);
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

void watch_rtc_schedule_next_comp(void) {
    rtc_counter_t curr_counter = watch_rtc_get_counter();
    // If there is already a pending comp interrupt for this very tick, let it fire
    // And this function will be called again as soon as the interrupt fires.
    if (curr_counter == scheduled_comp_counter) {
        return;
    }

    // The soonest we can schedule is the next tick
    curr_counter +=1;

    bool schedule_any = false;
    rtc_counter_t comp_counter;
    rtc_counter_t min_diff = UINT_MAX;

    for (uint8_t index = 0; index < WATCH_RTC_N_COMP_CB; ++index) {
        // rtc_counter_t diff = 
        if (comp_callbacks[index].enabled) {
            rtc_counter_t diff = comp_callbacks[index].counter - curr_counter;
            if (diff <= min_diff) {
                min_diff = diff;
                comp_counter = comp_callbacks[index].counter;
                schedule_any = true;
            }
        }
    }

    if (schedule_any) {
        scheduled_comp_counter = comp_counter;
    } else {
        scheduled_comp_counter = curr_counter - 2;
    }
}

void watch_rtc_enable(bool en)
{
    // Nothing to do cases
    if ((en && counter_interval) || (!en && !counter_interval)) {
        return;
    }

    if (en) {
        // Very bad way to keep time, but okay way to emulates the hardware.
        double ms = 1000.0 / (double)RTC_CNT_HZ; // in msec
        counter_interval = emscripten_set_interval(_watch_increase_counter, ms, NULL);
    } else {
        emscripten_clear_interval(counter_interval);
        counter_interval = 0;
    }
}

void watch_rtc_freqcorr_write(int16_t value, int16_t sign)
{
    //Not simulated
}
