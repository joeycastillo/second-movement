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

#include <stdlib.h>
#include <string.h>
#include "float_demo_face.h"
#include "watch.h"

void float_demo_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) *context_ptr = malloc(sizeof(float));
}

void float_demo_face_activate(void *context) {
    float *value = (float *)context;
    *value = -110;
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "FLOAT", "FL");
    movement_request_tick_frequency(16);
}

bool float_demo_face_loop(movement_event_t event, void *context) {
    float *value = (float *)context;
    switch (event.event_type) {
        case EVENT_TICK:
            *value = (*value) + 0.31;
            // fall through
        case EVENT_ACTIVATE:
            watch_display_float_with_best_effort(*value, "#F");
            break;
        case EVENT_ALARM_BUTTON_UP:
            *value = -100;
            movement_request_tick_frequency(16);
            break;
        case EVENT_ALARM_LONG_PRESS:
            *value = -10.85;
            movement_request_tick_frequency(1);
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void float_demo_face_resign(void *context) {
    (void) context;
}
