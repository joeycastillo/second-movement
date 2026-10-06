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
 * Casio Soccer / Stopwatch Football
 *
 * Alarm: start/stop the stopwatch. Last digit decides the play.
 * Light: reset match (ignored while timer is running).
 * Alarm long press: also reset match.
 *
 * Open play (last digit):
 *  0,9 Goal | 1 Penalty | 2 Corner | 3 Free kick | 4 Miss
 *  5 Save | 6 Yellow | 7 Red (opponent PK) | 8 Offside
 *
 * Set pieces resolved on the next stop:
 *  Penalty: even Goal, odd Miss
 *  Corner: 1-3 Goal, else Cleared
 *  Free kick: 1-2 Goal, else Blocked
 *
 * Match: 2 halves, 10 turns per player each. Tied FT -> 5 PK each.
 *
 * Display: P1/P2, H1/H2/PK, score, phase code (GL/PN/CN/FK/MS/SV/YC/RC/OS).
 */

#define STOPWATCH_FOOTBALL_TURNS_PER_HALF 10
#define STOPWATCH_FOOTBALL_PK_ATTEMPTS 5

typedef enum {
    SOC_PHASE_OPEN = 0,
    SOC_PHASE_PENALTY,
    SOC_PHASE_CORNER,
    SOC_PHASE_FREE_KICK,
    SOC_PHASE_RED_PK,
    SOC_PHASE_SHOOTOUT,
    SOC_PHASE_MATCH_OVER,
} stopwatch_football_phase_t;

typedef enum {
    SOC_RESULT_NONE = 0,
    SOC_RESULT_GOAL,
    SOC_RESULT_PENALTY,
    SOC_RESULT_CORNER,
    SOC_RESULT_FREE_KICK,
    SOC_RESULT_MISS,
    SOC_RESULT_SAVE,
    SOC_RESULT_YELLOW,
    SOC_RESULT_RED,
    SOC_RESULT_OFFSIDE,
    SOC_RESULT_CLEARED,
    SOC_RESULT_BLOCKED,
    SOC_RESULT_PK_GOAL,
    SOC_RESULT_PK_MISS,
} stopwatch_football_result_t;

typedef struct {
    swg_timer_t timer;
    uint8_t score[2];
    uint8_t turns[2];
    uint8_t half;
    uint8_t current_player;
    stopwatch_football_phase_t phase;
    stopwatch_football_result_t last_result;
    uint8_t pk_made[2];
    uint8_t pk_taken[2];
} stopwatch_football_state_t;

void stopwatch_football_face_setup(uint8_t watch_face_index, void **context_ptr);
void stopwatch_football_face_activate(void *context);
bool stopwatch_football_face_loop(movement_event_t event, void *context);
void stopwatch_football_face_resign(void *context);

#define stopwatch_football_face ((const watch_face_t){ \
    stopwatch_football_face_setup, \
    stopwatch_football_face_activate, \
    stopwatch_football_face_loop, \
    stopwatch_football_face_resign, \
    NULL, \
})
