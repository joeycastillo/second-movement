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
 * Circadian Score (CS) Face
 *
 * Displays overall Circadian Score (0-100) with drill-down to 5 component subscores:
 * - TI (Timing/SRI): Sleep Regularity Index (35% weight)
 * - DU (Duration): Sleep duration penalty (30% weight)
 * - EF (Efficiency): Sleep efficiency (20% weight)
 * - AH (Active Hours): Compliance with window (10% weight)
 * - LI (Light): Light exposure quality (5% weight)
 *
 * ALARM button: cycle through CS → TI → DU → EF → AH → LI → CS
 * LIGHT button: navigate historical nights (0-6 days ago)
 * MODE button: next face
 */

typedef enum {
    CSFACE_MODE_CS,    // Overall Circadian Score
    CSFACE_MODE_TI,    // Timing (SRI)
    CSFACE_MODE_DU,    // Duration
    CSFACE_MODE_EF,    // Efficiency
    CSFACE_MODE_AH,    // Active Hours
    CSFACE_MODE_LI,    // Light
    CSFACE_MODE_RI,    // Restlessness Index (Phase 4E)
    CSFACE_MODE_COUNT
} circadian_score_face_mode_t;

typedef struct {
    circadian_score_face_mode_t mode;
    uint8_t historical_night;  // 0 = aggregate, 1-7 = individual nights
} circadian_score_face_state_t;

void circadian_score_face_setup(uint8_t watch_face_index, void **context_ptr);
void circadian_score_face_activate(void *context);
bool circadian_score_face_loop(movement_event_t event, void *context);
void circadian_score_face_resign(void *context);

#define circadian_score_face ((const watch_face_t){ \
    circadian_score_face_setup, \
    circadian_score_face_activate, \
    circadian_score_face_loop, \
    circadian_score_face_resign, \
    NULL \
})
