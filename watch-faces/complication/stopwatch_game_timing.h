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

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "watch_rtc.h"

#define SWG_UI_TICK_HZ 16
#define SWG_TARGET_10_HUNDREDTHS 1000

typedef struct {
    rtc_counter_t start_counter;
    rtc_counter_t stop_counter;
    bool running;
} swg_timer_t;

void swg_timer_reset(swg_timer_t *timer);
void swg_timer_start(swg_timer_t *timer);
void swg_timer_stop(swg_timer_t *timer);
uint32_t swg_timer_elapsed_ticks(const swg_timer_t *timer);
uint32_t swg_ticks_to_hundredths(uint32_t ticks);
uint32_t swg_timer_elapsed_hundredths(const swg_timer_t *timer);
uint8_t swg_last_digit(uint32_t hundredths);
uint8_t swg_last_two_digits(uint32_t hundredths);
void swg_format_hundredths(uint32_t hundredths, char *buf6);
