/*
 * MIT License
 *
 * Copyright (c) 2025 Claude
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
 * LUX RX DEMO FACE
 *
 * Receives text via the OPT3001 visible light sensor using the lux_rx 6-bit protocol.
 *
 * Display:
 *   WAIT   — idle, waiting for START symbol
 *   RECV   — frame received OK (green LED), shows received text
 *   FAIL   — CRC error (red LED)
 *
 * Buttons:
 *   ALARM short: reset receiver (after done/error), re-init (during idle)
 *   LIGHT:       suppressed (interferes with sensor)
 *   MODE:        next face
 */

#include "lux_rx.h"

typedef struct {
    lux_rx_t rx;
    lux_rx_status_t last_status;
} lux_rx_demo_context_t;

void lux_rx_demo_face_setup(uint8_t watch_face_index, void ** context_ptr);
void lux_rx_demo_face_activate(void *context);
bool lux_rx_demo_face_loop(movement_event_t event, void *context);
void lux_rx_demo_face_resign(void *context);

#define lux_rx_demo_face ((const watch_face_t){ \
    lux_rx_demo_face_setup, \
    lux_rx_demo_face_activate, \
    lux_rx_demo_face_loop, \
    lux_rx_demo_face_resign, \
    NULL, \
})
