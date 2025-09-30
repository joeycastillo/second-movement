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

#ifndef PLANETARY_HOUR_FACE_H_
#define PLANETARY_HOUR_FACE_H_

#include "movement.h"
#include "location.h"

typedef struct {
    uint8_t current_planetary_hour;
    uint8_t current_zodiac_sign;
    uint8_t longLatToUse;            
    int16_t hour_offset;   // 0 = current hour, +1 = next hour, -1 = previous hour, etc.
    watch_date_time_t hour_offset_expires;
    location_state_t location_state; 
} planetary_hour_state_t;

void planetary_hour_face_setup(uint8_t watch_face_index, void **context_ptr);
void planetary_hour_face_activate(void *context);
bool planetary_hour_face_loop(movement_event_t event, void *context);
void planetary_hour_face_resign(void *context);

#define planetary_hour_face ((const watch_face_t){ \
    planetary_hour_face_setup, \
    planetary_hour_face_activate, \
    planetary_hour_face_loop, \
    planetary_hour_face_resign, \
    NULL, \
})

#endif // PLANETARY_HOUR_FACE_H_
