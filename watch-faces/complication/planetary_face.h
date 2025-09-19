/*
 * MIT License
 *
 * Copyright (c) 2025 Your Name
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

#ifndef PLANETARY_FACE_H_
#define PLANETARY_FACE_H_

#include "movement.h"

typedef struct {
    uint8_t current_planetary_hour;
    uint8_t current_zodiac_sign;
} planetary_state_t;

void planetary_face_setup(uint8_t watch_face_index, void **context_ptr);
void planetary_face_activate(void *context);
bool planetary_face_loop(movement_event_t event, void *context);
void planetary_face_resign(void *context);

#define planetary_face ((const watch_face_t){ \
    planetary_face_setup, \
    planetary_face_activate, \
    planetary_face_loop, \
    planetary_face_resign, \
    NULL, \
})

#endif // PLANETARY_FACE_H_
