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

#include "watch_rtc.h"
#include "watch_main_loop.h"
#include "watch_utility.h"

#include <emscripten.h>
#include <emscripten/html5.h>

static double time_offset = 0;
static long tick_callbacks[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };

static long alarm_interval_id = -1;
static long alarm_timeout_id = -1;
static double alarm_interval;
watch_cb_t alarm_callback;
watch_cb_t btn_alarm_callback;
watch_cb_t a2_callback;
watch_cb_t a4_callback;

bool _watch_rtc_is_enabled(void) {
    return true;
}

void _watch_rtc_init(void) {
#if EMSCRIPTEN
    // Shifts the timezone so our local time is converted to UTC and set
    int32_t time_zone_offset = EM_ASM_INT({
        return -new Date().getTimezoneOffset() * 60;
    });
#endif
#ifdef BUILD_YEAR
    watch_date_time_t date_time = watch_get_init_date_time();
#else
    watch_date_time_t date_time = watch_rtc_get_date_time();
#endif
    watch_rtc_set_date_time(watch_utility_date_time_convert_zone(date_time, time_zone_offset, 0));
}

void watch_rtc_set_date_time(watch_date_time_t date_time) {
    time_offset = EM_ASM_DOUBLE({
        const year = 2020 + (($0 >> 26) & 0x3f);
        const month = ($0 >> 22) & 0xf;
        const day = ($0 >> 17) & 0x1f;
        const hour = ($0 >> 12) & 0x1f;
        const minute = ($0 >> 6) & 0x3f;
        const second = $0 & 0x3f;
        const date = new Date(year, month - 1, day, hour, minute, second);
        return date - Date.now();
    }, date_time.reg);
}

watch_date_time_t watch_rtc_get_date_time(void) {
    watch_date_time_t retval;
    retval.reg = EM_ASM_INT({
        const date = new Date(Date.now() + $0);
        return date.getSeconds() |
            (date.getMinutes() << 6) |
            (date.getHours() << 12) |
            (date.getDate() << 17) |
            ((date.getMonth() + 1) << 22) |
            ((date.getFullYear() - 2020) << 26);
    }, time_offset);
    return retval;
}

rtc_date_time_t watch_get_init_date_time(void) {
    rtc_date_time_t date_time = {0};

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

static void watch_invoke_periodic_callback(void *userData) {
    watch_cb_t callback = userData;
    callback();
    resume_main_loop();
}

void watch_rtc_register_periodic_callback(watch_cb_t callback, uint8_t frequency) {
    // we told them, it has to be a power of 2.
    if (__builtin_popcount(frequency) != 1) return;

    // this left-justifies the period in a 32-bit integer.
    uint32_t tmp = (frequency & 0xFF) << 24;
    // now we can count the leading zeroes to get the value we need.
    // 0x01 (1 Hz) will have 7 leading zeros for PER7. 0xF0 (128 Hz) will have no leading zeroes for PER0.
    uint8_t per_n = __builtin_clz(tmp);

    double interval = 1000.0 / frequency; // in msec

    if (tick_callbacks[per_n] != -1) emscripten_clear_interval(tick_callbacks[per_n]);
    tick_callbacks[per_n] = emscripten_set_interval(watch_invoke_periodic_callback, interval, (void *)callback);
}

void watch_rtc_disable_periodic_callback(uint8_t frequency) {
    if (__builtin_popcount(frequency) != 1) return;
    uint8_t per_n = __builtin_clz((frequency & 0xFF) << 24);
    if (tick_callbacks[per_n] != -1) {
        emscripten_clear_interval(tick_callbacks[per_n]);
        tick_callbacks[per_n] = -1;
    }
}

void watch_rtc_disable_matching_periodic_callbacks(uint8_t mask) {
    for (int i = 0; i < 8; i++) {
        if (tick_callbacks[i] != -1 && (mask & (1 << i)) != 0) {
            emscripten_clear_interval(tick_callbacks[i]);
            tick_callbacks[i] = -1;
        }
    }
}

void watch_rtc_disable_all_periodic_callbacks(void) {
    watch_rtc_disable_matching_periodic_callbacks(0xFF);
}

static void watch_invoke_alarm_interval_callback(void *userData) {
    if (alarm_callback) alarm_callback();
}

static void watch_invoke_alarm_callback(void *userData) {
    if (alarm_callback) alarm_callback();
    alarm_interval_id = emscripten_set_interval(watch_invoke_alarm_interval_callback, alarm_interval, NULL);
}

void watch_rtc_register_alarm_callback(watch_cb_t callback, watch_date_time_t alarm_time, rtc_alarm_match_t mask) {
    watch_rtc_disable_alarm_callback();

    switch (mask) {
        case ALARM_MATCH_DISABLED:
            return;
        case ALARM_MATCH_SS:
            alarm_interval = 60 * 1000;
            break;
        case ALARM_MATCH_MMSS:
            alarm_interval = 60 * 60 * 1000;
            break;
        case ALARM_MATCH_HHMMSS:
            alarm_interval = 60 * 60 * 60 * 1000;
            break;
    }

    double timeout = EM_ASM_DOUBLE({
        const now = Date.now();
        const date = new Date(now);

        const hour = ($0 >> 12) & 0x1f;
        const minute = ($0 >> 6) & 0x3f;
        const second = $0 & 0x3f;

        if ($1 == 1) { // SS
            if (second < date.getSeconds()) date.setMinutes(date.getMinutes() + 1);
            date.setSeconds(second);
        } else if ($1 == 2) { // MMSS
            if (second < date.getSeconds()) date.setMinutes(date.getMinutes() + 1);
            if (minute < date.getMinutes()) date.setHours(date.getHours() + 1);
            date.setMinutes(minute, second);
        } else if ($1 == 3) { // HHMMSS
            if (second < date.getSeconds()) date.setMinutes(date.getMinutes() + 1);
            if (minute < date.getMinutes()) date.setHours(date.getHours() + 1);
            if (hour < date.getHours()) date.setDate(date.getDate() + 1);
            date.setHours(hour, minute, second);
        } else {
            throw 'Invalid alarm match mask';
        }

        return date - now;
    }, alarm_time.reg, mask);

    alarm_callback = callback;
    alarm_timeout_id = emscripten_set_timeout(watch_invoke_alarm_callback, timeout, NULL);
}

void watch_rtc_disable_alarm_callback(void) {
    alarm_callback = NULL;
    alarm_interval = 0;

    if (alarm_timeout_id != -1) {
        emscripten_clear_timeout(alarm_timeout_id);
        alarm_timeout_id = -1;
    }

    if (alarm_interval_id != -1) {
        emscripten_clear_interval(alarm_interval_id);
        alarm_interval_id = -1;
    }
}

void watch_rtc_enable(bool en)
{
    //Not simulated
}

void watch_rtc_freqcorr_write(int16_t value, int16_t sign)
{
    //Not simulated
}
