/*
 * MIT License
 *
 * Copyright (c) 2025 Alessandro Genova
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
 * RTCCOUNT FACE
 * 
 * A test face to inspect some metrics of the rtc-counter32 mode.
 */

#include "movement.h"

void rtccount_face_setup(uint8_t watch_face_index, void ** context_ptr);
void rtccount_face_activate(void *context);
bool rtccount_face_loop(movement_event_t event, void *context);
void rtccount_face_resign(void *context);
movement_watch_face_advisory_t rtccount_face_advise(void *context);

#define rtccount_face ((const watch_face_t){ \
    rtccount_face_setup, \
    rtccount_face_activate, \
    rtccount_face_loop, \
    rtccount_face_resign, \
    rtccount_face_advise, \
})
