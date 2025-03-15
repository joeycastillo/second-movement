/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
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

#include "pins.h"

#ifdef HAS_ACCELEROMETER

/*
 * ACTIVITY LOGGING
 *
 * This watch face works with Movement's built-in count of accelerometer
 * waekeups to log activity over time. It is very much a work in progress,
 * and these notes will expand as the functionality is fleshed out.
 */

#include "movement.h"
#include "watch.h"

#define ACTIVITY_LOGGING_NUM_DATA_POINTS (100)

typedef struct {
    uint8_t display_index;  // the index we are displaying on screen
    uint8_t ts_ticks;       // when the user taps the LIGHT button, we show the timestamp for a few ticks.
} activity_logging_state_t;

void activity_logging_face_setup(uint8_t watch_face_index, void ** context_ptr);
void activity_logging_face_activate(void *context);
bool activity_logging_face_loop(movement_event_t event, void *context);
void activity_logging_face_resign(void *context);
movement_watch_face_advisory_t activity_logging_face_advise(void *context);

#define activity_logging_face ((const watch_face_t){ \
    activity_logging_face_setup, \
    activity_logging_face_activate, \
    activity_logging_face_loop, \
    activity_logging_face_resign, \
    NULL, \
})

#endif // HAS_TEMPERATURE_SENSOR
