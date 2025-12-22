/*
 * MIT License
 *
 * Copyright (c) 2025 Ola Kåre Risa
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

#ifndef DOZENAL_FACE_H_
#define DOZENAL_FACE_H_

/*
 * HEXA DECIMAL FACE
 *
 * This watch face is based on Joey Castillo's clock_face but displays time 
 * and date in hexadecimal format instead of decimal. 
 *
 * Hexadecimal digits above 9 are normally displayed in uppercase (A, C, E, F), 
 * but 11 and 13 are shown in lowercase ('b' and 'd') to improve clarity on 
 * the 7-segment display.
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
} dozenal_state_t;

void dozenal_face_setup(uint8_t watch_face_index, void ** context_ptr);
void dozenal_face_activate(void *context);
bool dozenal_face_loop(movement_event_t event, void *context);
void dozenal_face_resign(void *context);

#define dozenal_face ((const watch_face_t){ \
    dozenal_face_setup, \
    dozenal_face_activate, \
    dozenal_face_loop, \
    dozenal_face_resign, \
    NULL, \
})

#endif // DOZENAL_FACE_H_

