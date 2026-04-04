/*
 * MIT License
 *
 * Copyright (c) 2025 Second Movement
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to do so, subject to the
 * following conditions:
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

#ifndef FESK_LITE_DEMO_FACE_H_
#define FESK_LITE_DEMO_FACE_H_

#include "movement.h"

void fesk_lite_demo_face_setup(uint8_t watch_face_index, void **context_ptr);
void fesk_lite_demo_face_activate(void *context);
bool fesk_lite_demo_face_loop(movement_event_t event, void *context);
void fesk_lite_demo_face_resign(void *context);

#define fesk_lite_demo_face ((watch_face_t){ \
    fesk_lite_demo_face_setup, \
    fesk_lite_demo_face_activate, \
    fesk_lite_demo_face_loop, \
    fesk_lite_demo_face_resign, \
    NULL, \
})

#endif // FESK_LITE_DEMO_FACE_H_
