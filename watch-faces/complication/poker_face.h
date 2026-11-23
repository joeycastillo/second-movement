/*
 * MIT License
 *
 * Copyright (c) 2023 Chris Ellis
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

#ifndef POKER_FACE_H_
#define POKER_FACE_H_

#include "movement.h"

/*
 * Blackjack face
 * ======================
 *
 * Simple poker game.
 *
 * 
 * 
 * | Cards   |                          |
 * |---------|--------------------------|
 * | Value   |2|3|4|5|6|7|8|9|
 * | Display |2|3|4|5|6|7|8|9|
 */



typedef struct {
    uint16_t games_played;
    uint16_t games_won;
} poker_face_state_t;

void poker_face_setup(uint8_t watch_face_index, void ** context_ptr);
void poker_face_activate(void *context);
bool poker_face_loop(movement_event_t event, void *context);
void poker_face_resign(void *context);

#define poker_face ((const watch_face_t){ \
    poker_face_setup, \
    poker_face_activate, \
    poker_face_loop, \
    poker_face_resign, \
    NULL, \
})

#endif // poker_FACE_H_
