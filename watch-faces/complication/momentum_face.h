/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
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
 * Momentum Zone Face
 *
 * Displays metrics relevant to the Momentum phase (ramping up):
 * - View 0: Wake Momentum (WK) - Primary metric (ramp to alertness)
 * - View 1: Sleep Debt (SD) - Residual sleep pressure
 * - View 2: Temperature (TE) - Current temperature in °C
 *
 * Zone indicator "MO" shown in top-left
 * ALARM button cycles through metric views
 * LIGHT button illuminates LED
 * 
 * Zone weights: SD=20, EM=20, WK=30, NRG=10, CMF=10
 */

// Metric view indices for Momentum zone
typedef enum {
    MOMENTUM_VIEW_WK = 0,  // Wake Momentum (primary)
    MOMENTUM_VIEW_SD = 1,  // Sleep Debt
    MOMENTUM_VIEW_EM = 2,  // Emotional
    MOMENTUM_VIEW_COUNT = 3
} momentum_view_index_t;

typedef struct {
    uint8_t view_index;     // 0-2, cycles through metrics (LEGACY - kept for compatibility)
    int8_t prev_sd;         // Previous Sleep Debt value (signed, range -60 to +120)
    uint8_t prev_other[2];  // Previous values for other metrics (WK, EM)
    
    // Word display system
    uint8_t display_mode;   // 0=word1, 1=word2, 2=stats
    char selected_words[2][9];  // Two selected words (8 chars + null terminator)
    uint8_t streak_days;    // Consecutive days viewed
    uint8_t last_check_day; // Day of year for streak tracking (1-366)
} momentum_face_state_t;

void momentum_face_setup(uint8_t watch_face_index, void **context_ptr);
void momentum_face_activate(void *context);
bool momentum_face_loop(movement_event_t event, void *context);
void momentum_face_resign(void *context);

#define momentum_face ((const watch_face_t){ \
    momentum_face_setup, \
    momentum_face_activate, \
    momentum_face_loop, \
    momentum_face_resign, \
    NULL \
})
