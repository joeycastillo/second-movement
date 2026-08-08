/*
 * MIT License
 *
 * Copyright (c) 2026 Giannis Prekas
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
#include "stopwatch_game_timing.h"

/*
 * Bullseye (Target 10.00 seconds)
 *
 * Stop as close to 10.00s as possible (not 1:00).
 * Score = |elapsed - 10.00| in hundredths of a second.
 * Example: stop at 2.39 -> error 7.61s -> shows 07:61 Er
 * 5 rounds alternating P1/P2. Lowest cumulative total wins.
 *
 * Alarm: start/stop.
 * Light: reset match (ignored while timer is running).
 * Alarm long press: also reset.
 */

#define STOPWATCH_BULLSEYE_ROUNDS 5

typedef struct {
    swg_timer_t timer;
    uint16_t total[2];
    uint16_t last_delta;
    uint8_t round;
    uint8_t current_player;
    bool match_over;
} stopwatch_bullseye_state_t;

void stopwatch_bullseye_face_setup(uint8_t watch_face_index, void **context_ptr);
void stopwatch_bullseye_face_activate(void *context);
bool stopwatch_bullseye_face_loop(movement_event_t event, void *context);
void stopwatch_bullseye_face_resign(void *context);

#define stopwatch_bullseye_face ((const watch_face_t){ \
    stopwatch_bullseye_face_setup, \
    stopwatch_bullseye_face_activate, \
    stopwatch_bullseye_face_loop, \
    stopwatch_bullseye_face_resign, \
    NULL, \
})
