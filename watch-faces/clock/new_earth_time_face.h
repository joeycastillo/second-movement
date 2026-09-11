/* SPDX-License-Identifier: MIT */

/*
 * MIT License
 *
 * Copyright © 2021-2022 Joey Castillo <joeycastillo@utexas.edu> <jose.castillo@gmail.com>
 * Copyright © 2022 Alexsander Akers <me@a2.io>
 * Copyright © 2022 TheOnePerson <a.nebinger@web.de>
 * Copyright © 2023 Alex Utter <ooterness@gmail.com>
 * Copyright © 2024 Matheus Afonso Martins Moreira <matheus.a.m.moreira@gmail.com>
 * Copyright © 2026 Chris Gillings <git@ned-ludd.com>
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

#ifndef NEW_EARTH_TIME_FACE_H_
#define NEW_EARTH_TIME_FACE_H_

/*
 * NEW_EARTH_TIME FACE
 *
 * Displays the current New Earth Time
 *
 *   ref. https://newearthtime.net
 *
 */

#include "movement.h"

typedef struct {
    struct {
        watch_date_time_t previous;
    } date_time;
    uint8_t last_battery_check;
    uint8_t watch_face_index;
    bool time_signal_enabled;
    bool battery_low;
} new_earth_time_state_t;

void new_earth_time_face_setup(uint8_t watch_face_index, void ** context_ptr);
void new_earth_time_face_activate(void *context);
bool new_earth_time_face_loop(movement_event_t event, void *context);
void new_earth_time_face_resign(void *context);
movement_watch_face_advisory_t new_earth_time_face_advise(void *context);

#define new_earth_time_face ((const watch_face_t) { \
    new_earth_time_face_setup, \
    new_earth_time_face_activate, \
    new_earth_time_face_loop, \
    new_earth_time_face_resign, \
    new_earth_time_face_advise, \
})

#endif // NEW_EARTH_TIME_FACE_H_
