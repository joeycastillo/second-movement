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
 * Blind 10-Second Count
 *
 * Estimate 10.00 seconds without looking. Display is blank while running.
 * Closest to 10.00 wins the point. First to 5 points wins.
 * On reveal, error shows as SS:HH Er (e.g. 07:61 = 7.61s from target).
 *
 * Alarm: start/stop.
 * Light: reset match (ignored while timer is running).
 * Alarm long press: also reset.
 */

#define STOPWATCH_BLIND_TEN_POINTS_TO_WIN 5

typedef struct {
    swg_timer_t timer;
    uint8_t points[2];
    uint32_t last_delta;
    uint32_t pending_delta[2];
    uint32_t last_time;
    uint8_t current_player;
    bool has_pending;
    bool match_over;
    bool show_reveal;
} stopwatch_blind_ten_state_t;

void stopwatch_blind_ten_face_setup(uint8_t watch_face_index, void **context_ptr);
void stopwatch_blind_ten_face_activate(void *context);
bool stopwatch_blind_ten_face_loop(movement_event_t event, void *context);
void stopwatch_blind_ten_face_resign(void *context);

#define stopwatch_blind_ten_face ((const watch_face_t){ \
    stopwatch_blind_ten_face_setup, \
    stopwatch_blind_ten_face_activate, \
    stopwatch_blind_ten_face_loop, \
    stopwatch_blind_ten_face_resign, \
    NULL, \
})
