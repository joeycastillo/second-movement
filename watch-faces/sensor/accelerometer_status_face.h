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

/*
 * Accelerometer Status / Settings
 *
 * Meant to be used in conjunction with the activity_logging_face. Shows the current
 * status of the accelerometer active/still status pin, and allows adjusting the
 * motion threshold via a long press of ALARM. Note that this will not work without
 * activity_logging_face in the lineup as activity_logging_face is the one that enables
 * background accelerometer sensing.
 */

#include "movement.h"
#include "watch.h"

typedef struct {
    uint8_t new_threshold;
    uint8_t threshold;
    bool is_setting;
} accel_interrupt_count_state_t;

void accelerometer_status_face_setup(uint8_t watch_face_index, void ** context_ptr);
void accelerometer_status_face_activate(void *context);
bool accelerometer_status_face_loop(movement_event_t event, void *context);
void accelerometer_status_face_resign(void *context);

#define accelerometer_status_face ((const watch_face_t){ \
    accelerometer_status_face_setup, \
    accelerometer_status_face_activate, \
    accelerometer_status_face_loop, \
    accelerometer_status_face_resign, \
    NULL, \
})
