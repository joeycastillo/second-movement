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

#ifndef HEXA_DECIMAL_FACE_H_
#define HEXA_DECIMAL_FACE_H_

/*
 * MARS TIME face
 *
 * This watch face is dedicated to Martian timekeeping.
 * It has several modes, and can display either a time or a date.
 *
 * Pressing the ALARM button cycles through different time zones on Mars:
 *   MC - Mars Coordinated Time, the time at Airy-0 Crater on the Martian prime meridian
 *   ZH - Local mean solar time for the Zhurong rover
 *   PE - LMST for the Perseverance rover
 *   IN - LMST for the Insight lander
 *   CU - LMST for the Curiosity rover
 *
 * Press the LIGHT button to toggle between displaying time and date:
 *   MC S - the Mars Sol Date, Martian days since December 29, 1873
 *   ZH Sol - Mission sol for the Zhurong rover
 *   PE Sol - Mission sol for the Perseverance rover
 *   IN S - Mission sol for the InSight lander
 *   CU S - Mission sol for the Curiosity rover
 *
 * Note that where the mission sol is below 1000, this watch face displays
 * the word “Sol” on the bottom line. When the mission sol is over 1000, the
 * word “Sol” will not fit and so it displays a stylized letter S at the top
 * right.
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
} hexa_decimal_state_t;

void hexa_decimal_face_setup(uint8_t watch_face_index, void ** context_ptr);
void hexa_decimal_face_activate(void *context);
bool hexa_decimal_face_loop(movement_event_t event, void *context);
void hexa_decimal_face_resign(void *context);

#define hexa_decimal_face ((const watch_face_t){ \
    hexa_decimal_face_setup, \
    hexa_decimal_face_activate, \
    hexa_decimal_face_loop, \
    hexa_decimal_face_resign, \
    NULL, \
})

#endif // HEXA_DECIMAL_FACE_H_

