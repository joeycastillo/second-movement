/*
 * MIT License
 *
 * Copyright (c) 2025 Mark Schlosser
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

#ifndef TCG_LIFE_COUNTER_FACE_H_
#define TCG_LIFE_COUNTER_FACE_H_

/*
 * TCG_LIFE_COUNTER face
 *
 * TCG Life Counter face is designed to track player life totals in a two-player trading card game. Two life totals will be displayed on the screen, separated by a colon. The left counter is controlled by short-pressing LIGHT. The right counter is controlled by short-pressing ALARM.
 * Life counters each begin at twenty and the face begins in decrement mode. This means the associated player's life total will decrement by a value of one each time LIGHT or ALARM is pressed. Once the face is changed to increment mode, the associated player's life total will increase by one each time LIGHT or ALARM is pressed.
 *
 * Usage:
 * Short-press LIGHT to decrement or increment (determined by mode) left counter (player 1). Clamps to 0-999.
 * Short-press ALARM to decrement or increment (determined by mode) right counter (player 2). Clamps to 0-999.
 * Long-press LIGHT to toggle mode to decrement or increment mode, indicated by a "d" or "I" character in the top right of LCD.
 * Long-press ALARM to reset mode to decrement mode and both life counters to current default value. If the life values displayed by the face are each equal to the current default life value, and the face is also set to decrement mode, this action will advance to the next set of default life values (currently twenty or forty). The initial default life value is configured to twenty.
 */

#include "movement.h"

typedef struct {
    uint16_t life_values[2];
    bool increment_mode_on;
    uint8_t default_idx;
} tcg_life_counter_state_t;

void tcg_life_counter_face_setup(uint8_t watch_face_index, void ** context_ptr);
void tcg_life_counter_face_activate(void *context);
bool tcg_life_counter_face_loop(movement_event_t event, void *context);
void tcg_life_counter_face_resign(void *context);

void print_tcg_life_counter(tcg_life_counter_state_t *state);

#define tcg_life_counter_face ((const watch_face_t){ \
    tcg_life_counter_face_setup, \
    tcg_life_counter_face_activate, \
    tcg_life_counter_face_loop, \
    tcg_life_counter_face_resign, \
    NULL, \
})

#endif // TCG_LIFE_COUNTER_FACE_H_
