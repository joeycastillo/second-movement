/* SPDX-License-Identifier: MIT */

/*
 * MIT License
 *
 * Copyright © 2021-2022 Joey Castillo <joeycastillo@utexas.edu> <jose.castillo@gmail.com>
 * Copyright © 2022 Alexsander Akers <me@a2.io>
 * Copyright © 2022 TheOnePerson <a.nebinger@web.de>
 * Copyright © 2023 Alex Utter <ooterness@gmail.com>
 * Copyright © 2024 Matheus Afonso Martins Moreira <matheus.a.m.moreira@gmail.com>
 * Copyright © 2025 Alessandro Genova
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

#ifndef STOCK_CLOCK_FACE_H_
#define STOCK_CLOCK_FACE_H_

/*
 * STOCK CLOCK FACE
 *
 * This is almost identical to clock_face, but it has two significant differences.
 * - it delegates the playing of the hourly chime to another face, merely displaying the bell indicator
 * - it toggles between 12/24H time display when pressing the alarm button
 * 
 * These changes effectively make this face identical to the stock F91W clock face, hence the name.
 *
 */

#include "movement.h"

typedef struct {
    struct {
        watch_date_time_t previous;
    } date_time;
    uint8_t last_battery_check;
    uint8_t watch_face_index;
    bool battery_low;
} stock_clock_state_t;

void stock_clock_face_setup(uint8_t watch_face_index, void ** context_ptr);
void stock_clock_face_activate(void *context);
bool stock_clock_face_loop(movement_event_t event, void *context);
void stock_clock_face_resign(void *context);

#define stock_clock_face ((const watch_face_t) { \
    stock_clock_face_setup, \
    stock_clock_face_activate, \
    stock_clock_face_loop, \
    stock_clock_face_resign, \
    NULL, \
})

#endif // CLOCK_FACE_H_
