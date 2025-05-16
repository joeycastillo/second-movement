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

/*
 * ACTIVITY LOGGING
 *
 * This watch face works with Movement's built-in count of accelerometer
 * waekeups to log activity over time.
 *
 * Default behavior is to show the last 100 data points. Format is:
 *  - Top left is display title (LOG or AC for Activity)
 *  - Top right is index backwards in the data log.
 *  - Bottom left is the number of orientation changes in the five minutes logged. (currently non-functional)
 *  - Bottom right is number of stationary minutes (0 to 5)
 *
 * A short press of the Light button reveals the time (bottom row) and date (top right) of the data point.
 * The display will update to say "AT" the time and date, or "T+D" on custom LCD.
 *
 * A short press of the Alarm button moves backwards in the data log.
 *
 * A long press of the Alarm button initiates a rapid dump of the full activity log buffer. Works best on custom LCD:
 *  - Top left is the index backwards in the data log.
 *  - Top right is the number of stationary minutes (0 to 5)
 *  - Bottom row should be viewed as two three digit numbers:
 *    - Positions 0, 1 and 2 contain the number of orientation changes in the five minutes logged.
 *    - Positions 3, 4 and 5 contain the temperature (256 = 25.6 degrees C)
 */

#include "movement.h"
#include "watch.h"

#define ACTIVITY_LOGGING_NUM_DATA_POINTS (100)

typedef struct {
    uint8_t display_index;  // the index we are displaying on screen
    uint8_t ts_ticks;       // when the user taps the LIGHT button, we show the timestamp for a few ticks.
    int16_t data_dump_idx;  // for dumping the full activity log on long press of Alarm
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
