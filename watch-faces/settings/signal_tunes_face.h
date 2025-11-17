/*
 * MIT License
 *
 * Copyright (c) 2022-2024 Joey Castillo
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

/*
 * SIGNAL TUNES face.
 *
 * Signal Tunes face allows configuration of distinct audio tunes for the
 * hourly chime, alarm, and countdown timer.
 *
 * LIGHT button cycles through signal slot (chime, alarm, timer).
 *
 * ALARM button cycles through available signal tunes.
 *
 */

#include "movement.h"

void signal_tunes_face_setup(uint8_t watch_face_index, void ** context_ptr);
void signal_tunes_face_activate(void *context);
bool signal_tunes_face_loop(movement_event_t event, void *context);
void signal_tunes_face_resign(void *context);

#define signal_tunes_face ((const watch_face_t){ \
    signal_tunes_face_setup, \
    signal_tunes_face_activate, \
    signal_tunes_face_loop, \
    signal_tunes_face_resign, \
    NULL, \
})
