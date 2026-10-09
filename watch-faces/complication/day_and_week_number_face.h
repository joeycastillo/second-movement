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

#ifndef DAY_AND_WEEK_NUMBER_FACE_H_
#define DAY_AND_WEEK_NUMBER_FACE_H_

/*
 * DAY_AND_WEEK_NUMBER FACE
 *
 * Displays the current week and day number for the year in the lower digits as 'ndddww'.
 * The upper digits show the weekday and the month day number.
 *
 */

#include "movement.h"

typedef struct {
    struct {
        watch_date_time_t previous;
    } date_time;
    uint8_t watch_face_index;
} day_and_week_number_face_state_t;

void day_and_week_number_face_setup(uint8_t watch_face_index, void ** context_ptr);
void day_and_week_number_face_activate(void *context);
bool day_and_week_number_face_loop(movement_event_t event, void *context);
void day_and_week_number_face_resign(void *context);
movement_watch_face_advisory_t day_and_week_number_face_advise(void *context);

#define day_and_week_number_face ((const watch_face_t) { \
    day_and_week_number_face_setup, \
    day_and_week_number_face_activate, \
    day_and_week_number_face_loop, \
    day_and_week_number_face_resign, \
    day_and_week_number_face_advise, \
})

#endif // DAY_AND_WEEK_NUMBER_FACE_H_
