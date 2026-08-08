/*
 * MIT License
 *
 * Copyright (c) 2026 Giannis Prekas
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

#include <stdio.h>
#include "stopwatch_game_timing.h"

void swg_timer_reset(swg_timer_t *timer) {
    timer->start_counter = 0;
    timer->stop_counter = 0;
    timer->running = false;
}

void swg_timer_start(swg_timer_t *timer) {
    timer->start_counter = watch_rtc_get_counter();
    timer->stop_counter = timer->start_counter;
    timer->running = true;
}

void swg_timer_stop(swg_timer_t *timer) {
    timer->stop_counter = watch_rtc_get_counter();
    timer->running = false;
}

uint32_t swg_timer_elapsed_ticks(const swg_timer_t *timer) {
    if (timer->running) {
        return watch_rtc_get_counter() - timer->start_counter;
    }
    return timer->stop_counter - timer->start_counter;
}

uint32_t swg_ticks_to_hundredths(uint32_t ticks) {
    uint32_t freq = watch_rtc_get_frequency();
    if (freq == 0) {
        freq = 128;
    }
    return ticks * 100 / freq;
}

uint32_t swg_timer_elapsed_hundredths(const swg_timer_t *timer) {
    return swg_ticks_to_hundredths(swg_timer_elapsed_ticks(timer));
}

uint8_t swg_last_digit(uint32_t hundredths) {
    return (uint8_t)(hundredths % 10);
}

uint8_t swg_last_two_digits(uint32_t hundredths) {
    return (uint8_t)(hundredths % 100);
}

void swg_format_hundredths(uint32_t hundredths, char *buf6) {
    uint32_t total_seconds = hundredths / 100;
    uint32_t frac = hundredths % 100;
    uint32_t minutes = (total_seconds / 60) % 100;
    uint32_t seconds = total_seconds % 60;
    sprintf(buf6, "%02lu%02lu%02lu",
            (unsigned long)minutes,
            (unsigned long)seconds,
            (unsigned long)frac);
}