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

#ifndef RATEMETER_FACE_H_
#define RATEMETER_FACE_H_

/*
 * RATE METER face
 *
 * The rate meter shows the rate per minute at which the ALARM button is
 * being pressed. This is particularly useful in sports where cadence
 * tracking is useful. For instance, rowing coaches often use a dedicated
 * rate meter - clicking the rate button each time the crew puts their oars
 * in the water to see the rate (strokes per minute) on the rate meter.
 */

#include "movement.h"

typedef struct {
    int16_t rate;
    int16_t ticks;
} ratemeter_state_t;

void ratemeter_face_setup(uint8_t watch_face_index, void ** context_ptr);
void ratemeter_face_activate(void *context);
bool ratemeter_face_loop(movement_event_t event, void *context);
void ratemeter_face_resign(void *context);

#define ratemeter_face ((const watch_face_t){ \
    ratemeter_face_setup, \
    ratemeter_face_activate, \
    ratemeter_face_loop, \
    ratemeter_face_resign, \
    NULL, \
})

#endif // RATEMETER_FACE_H_
