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
 * Stopwatch Blackjack (Target 100)
 *
 * Not the card blackjack face. Double-click (start/stop) adds last two digits
 * (00-99) to your bank. Alarm while stopped = Hit (another draw). Light = Stay.
 * Bust over 100 scores 0 for the round. Alternate P1/P2; higher under-100 wins.
 *
 * Alarm long press: reset match.
 */

typedef enum {
    SWBJ_NEED_DRAW = 0,
    SWBJ_DECIDE,
    SWBJ_ROUND_OVER,
    SWBJ_MATCH_OVER,
} swbj_phase_t;

typedef struct {
    swg_timer_t timer;
    uint8_t bank[2];
    uint8_t final_score[2];
    uint8_t last_draw;
    uint8_t points[2];
    uint8_t current_player;
    uint8_t players_finished;
    swbj_phase_t phase;
    bool busted;
} stopwatch_blackjack_state_t;

void stopwatch_blackjack_face_setup(uint8_t watch_face_index, void **context_ptr);
void stopwatch_blackjack_face_activate(void *context);
bool stopwatch_blackjack_face_loop(movement_event_t event, void *context);
void stopwatch_blackjack_face_resign(void *context);

#define stopwatch_blackjack_face ((const watch_face_t){ \
    stopwatch_blackjack_face_setup, \
    stopwatch_blackjack_face_activate, \
    stopwatch_blackjack_face_loop, \
    stopwatch_blackjack_face_resign, \
    NULL, \
})
