/*
 * MIT License
 *
 * Copyright (c) 2025 Daniel Bergman
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

/*
 * ISH FACE: A deliberately vague watch face that displays approximate time with 
 * three configurable vagueness levels. Perfect for vacation mode when precise time isn't needed.
 *
 * Vagueness levels:
 *   1: Hour (e.g., "09" or "14") - switches at the 30-minute mark
 *   2: Half Hour (e.g., "13:3o", "14:0o") - switches at the 15-minute mark, o instead of 0 to signify vagueness
 *   3: Quarter (e.g., "13:45") - rounds to nearest quarter hour
 *
 * Press ALARM to cycle levels. We honor the 24h clock mode setting but don't show the AM/PM indicator.
 */

typedef struct {
    uint8_t vagueness_level; // 1=hour, 2=half hour, 3=quarter
    uint8_t last_displayed_minute; // Last minute when we updated the display
} ish_face_state_t;

void ish_face_setup(uint8_t watch_face_index, void ** context_ptr);
void ish_face_activate(void *context);
bool ish_face_loop(movement_event_t event, void *context);
void ish_face_resign(void *context);

#define ish_face ((const watch_face_t){ \
    ish_face_setup, \
    ish_face_activate, \
    ish_face_loop, \
    ish_face_resign, \
    NULL, \
})
