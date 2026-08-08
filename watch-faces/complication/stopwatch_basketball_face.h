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
 * Stopwatch Basketball
 *
 * Alarm: start/stop. Last digit decides the possession.
 * Light: reset game (ignored while timer is running).
 * Alarm long press: also reset game.
 *
 * 0-2: 2-pointer | 3: 3-pointer | 4-5: Miss | 6-7: Foul (2 FT)
 * 8-9: Turnover
 *
 * Foul shooting: two separate stops; even = +1, odd = +0 each.
 * 4 quarters, 10 possessions per player each quarter.
 */

#define SWBB_POSSESSIONS_PER_QUARTER 10
#define SWBB_QUARTERS 4

typedef enum {
    SWBB_PHASE_OPEN = 0,
    SWBB_PHASE_FOUL_1,
    SWBB_PHASE_FOUL_2,
    SWBB_PHASE_OVER,
} swbb_phase_t;

typedef enum {
    SWBB_RESULT_NONE = 0,
    SWBB_RESULT_2PT,
    SWBB_RESULT_3PT,
    SWBB_RESULT_MISS,
    SWBB_RESULT_FOUL,
    SWBB_RESULT_TURNOVER,
    SWBB_RESULT_FT_MAKE,
    SWBB_RESULT_FT_MISS,
} swbb_result_t;

typedef struct {
    swg_timer_t timer;
    uint8_t score[2];
    uint8_t possessions[2];
    uint8_t quarter;
    uint8_t current_player;
    swbb_phase_t phase;
    swbb_result_t last_result;
} stopwatch_basketball_state_t;

void stopwatch_basketball_face_setup(uint8_t watch_face_index, void **context_ptr);
void stopwatch_basketball_face_activate(void *context);
bool stopwatch_basketball_face_loop(movement_event_t event, void *context);
void stopwatch_basketball_face_resign(void *context);

#define stopwatch_basketball_face ((const watch_face_t){ \
    stopwatch_basketball_face_setup, \
    stopwatch_basketball_face_activate, \
    stopwatch_basketball_face_loop, \
    stopwatch_basketball_face_resign, \
    NULL, \
})
