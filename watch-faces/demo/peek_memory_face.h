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
 * PEEK FACE
 *
 * This watch face displays a location in memory in a given format.
 * Currently hard coded but would be cool to let user select it somehow.
 *
 * Only works with custom LCD. This is for debugging purposes only.
 */

#include "movement.h"

/// TODO: more formats, signed and unsigned decimal, etc.
typedef enum {
    PEEK_MEMORY_FORMAT_HEX = 0,
    PEEK_MEMORY_FORMAT_DATE
} peek_memory_format_t;

typedef struct {
    uint8_t format;
    void *location;
} peek_memory_state_t;

void peek_memory_face_setup(uint8_t watch_face_index, void ** context_ptr);
void peek_memory_face_activate(void *context);
bool peek_memory_face_loop(movement_event_t event, void *context);
void peek_memory_face_resign(void *context);

#define peek_memory_face ((const watch_face_t){ \
    peek_memory_face_setup, \
    peek_memory_face_activate, \
    peek_memory_face_loop, \
    peek_memory_face_resign, \
    NULL, \
})
