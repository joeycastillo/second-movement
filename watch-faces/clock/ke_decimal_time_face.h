/*
 * MIT License
 *
 * Copyright (c) 2025 Joey Castillo
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

#include "movement.h"

/*
 * Kè (Decimal Time)
 *
 * This watch face is an optional replacement for the standard Clock face.
 * Like the standard Clock face, it displays the time and weekday at the top,
 * but the main line shows the percentage of the day that has passed, with midnight
 * represented by 0% and 11:59 PM as 99.9%.
 *
 * Name comes from here: https://en.wikipedia.org/wiki/Traditional_Chinese_timekeeping#One-hundredth_of_a_day:_kè
 *
 */

typedef struct {
    uint8_t previous_day;
    uint32_t previous_time;
} ke_decimal_time_state_t;

void ke_decimal_time_face_setup(uint8_t watch_face_index, void ** context_ptr);
void ke_decimal_time_face_activate(void *context);
bool ke_decimal_time_face_loop(movement_event_t event, void *context);
void ke_decimal_time_face_resign(void *context);

#define ke_decimal_time_face ((const watch_face_t){ \
    ke_decimal_time_face_setup, \
    ke_decimal_time_face_activate, \
    ke_decimal_time_face_loop, \
    ke_decimal_time_face_resign, \
    NULL, \
})
