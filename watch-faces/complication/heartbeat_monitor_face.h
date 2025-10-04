/*
 * MIT License
 *
 * Copyright (c) 2025 spipm
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

// Hardcode your birth year
#define BIRTH_YEAR 1989

#define LOW_RATE_UPPER 55 // 50–60%
#define CENTER_RATE_UPPER 75 // 60-70%

typedef struct {
    uint8_t tap_count;
    uint32_t tap_times[4];
    uint8_t bpm;
} heartbeat_monitor_state_t;

void heartbeat_monitor_face_setup(uint8_t watch_face_index, void ** context_ptr);
void heartbeat_monitor_face_activate(void *context);
bool heartbeat_monitor_face_loop(movement_event_t event, void *context);
void heartbeat_monitor_face_resign(void *context);

#define heartbeat_monitor_face ((const watch_face_t){ \
    heartbeat_monitor_face_setup, \
    heartbeat_monitor_face_activate, \
    heartbeat_monitor_face_loop, \
    heartbeat_monitor_face_resign, \
    NULL, \
})
