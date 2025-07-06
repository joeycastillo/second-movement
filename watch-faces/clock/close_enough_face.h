/*
 * MIT License
 *
 * Copyright (c) 2025 Ruben Nic
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
 * CLOSE ENOUGH CLOCK FACE
 *
 * Displays the current time; but only in periods of 5.
 * Some examples:
 * - 5:10 is "10 past 5" displayed as "10 P 5"
 * - 5:45 is "15 to 6" displayed as "15 2 6"
 * - 6:00 is "6 o'clock" displayed as "6 OC"
 */

typedef struct {
    int prev_five_minute_period;
    int prev_min_checked;
    uint8_t last_battery_check;
    bool battery_low;
} close_enough_state_t;

void close_enough_face_setup(uint8_t watch_face_index, void ** context_ptr);
void close_enough_face_activate(void *context);
bool close_enough_face_loop(movement_event_t event, void *context);
void close_enough_face_resign(void *context);

#define close_enough_face ((const watch_face_t){ \
    close_enough_face_setup, \
    close_enough_face_activate, \
    close_enough_face_loop, \
    close_enough_face_resign, \
    NULL, \
})
