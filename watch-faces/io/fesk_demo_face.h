/*
 * MIT License
 *
 * Copyright (c) 2024 Second Movement Project
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

#ifndef FESK_DEMO_FACE_H_
#define FESK_DEMO_FACE_H_

/*
 * FESK DEMO FACE
 *
 * A simple demonstration watch face for sending FESK messages.
 *
 * Controls:
 * - ALARM button: Start transmission when idle, cancel when transmitting.
 *
 * - ALARM long press: Play debug melody. This can be used to see if your
 *   receiver is getting similar frequencies as we are expecting it to be.
 *
 * To decode FESK messages, you can use the fesk_rt webapp which is available
 * at https://github.com/eiriksm/fesk-rt.
 */

#include "movement.h"

void fesk_demo_face_setup(uint8_t watch_face_index, void **context_ptr);
void fesk_demo_face_activate(void *context);
bool fesk_demo_face_loop(movement_event_t event, void *context);
void fesk_demo_face_resign(void *context);

#define fesk_demo_face ((watch_face_t){ \
    fesk_demo_face_setup, \
    fesk_demo_face_activate, \
    fesk_demo_face_loop, \
    fesk_demo_face_resign, \
    NULL, \
})

#endif // FESK_DEMO_FACE_H_
