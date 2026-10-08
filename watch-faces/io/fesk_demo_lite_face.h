/*
 * MIT License
 *
 * Copyright (c) 2025 Eirik S. Morland
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
#include "fesk_session.h"

/*
 * The very simplest demo of using the FESK transmission library in a watch face.
 *
 * Press the Alarm button to start a FESK transmission of the text "test".
 */

typedef struct {
    fesk_session_t session;
} fesk_demo_lite_state_t;

void fesk_demo_lite_face_setup(uint8_t watch_face_index, void ** context_ptr);
void fesk_demo_lite_face_activate(void *context);
bool fesk_demo_lite_face_loop(movement_event_t event, void *context);
void fesk_demo_lite_face_resign(void *context);

#define fesk_demo_lite_face ((const watch_face_t){ \
    fesk_demo_lite_face_setup, \
    fesk_demo_lite_face_activate, \
    fesk_demo_lite_face_loop, \
    fesk_demo_lite_face_resign, \
    NULL, \
})
