/*
 * MIT License
 *
 * Copyright (c) 2025 Eirik S. Morland.
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
 * A DESCRIPTION OF YOUR WATCH FACE
 *
 * and a description of how use it
 *
 */

typedef enum {
    UT_NONE = 0,
    UT_CHIRPING,
    UT_TRANSMITTING,
} uptime_mode_t;

typedef struct {
    uint32_t boot_time;
    uptime_mode_t mode;
    fesk_session_t session;
    fesk_session_config_t config;
} uptime_state_t;

void uptime_face_setup(uint8_t watch_face_index, void ** context_ptr);
void uptime_face_activate(void *context);
bool uptime_face_loop(movement_event_t event, void *context);
void uptime_face_resign(void *context);

#define uptime_face ((const watch_face_t){ \
    uptime_face_setup, \
    uptime_face_activate, \
    uptime_face_loop, \
    uptime_face_resign, \
    NULL, \
})
