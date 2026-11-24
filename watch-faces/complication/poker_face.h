/*
 * MIT License
 *
 * Copyright (c) 2026 David Volovskiy
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
 * Poker face
 * ======================
 *
 * Simple poker game.
 *
 * Very simple 5-card poker.
 * There's no bets, just one round of wapping to make the best hand you can.
 * All cards are the same color, so flushes don't occur.
 * 
 * When you start a game, you get dealt five cards. Your cursor will be on the right-most poistion.
 * The top-right will show your current hand rank ('1P' for 1-Pair, '3K' for three-of-a-kind)
 * You can then scroll left or right to select a card to swap. When you select the card, it'll be raised
 * up so as to make it clear you want to swap it.
 * If you select the empty right-most position, or long-press the select button, you'll swap the selected cards.
 * Immedietly when you swap, the dealer will swap their cards and the top-right will show their rank.
 * The top-left will show if you won that round ('W' for Win, 'L' for Lose, 'T' for Tie).
 * If you want to see the opponent's full hand, you can long-press the select button and it'll show their hand and rank.
 * Long-pressing the select button will go back to showing your hand and rank.
 * If you want more then one round of being able to swap cards, then change BETTING_ROUNDS to a larger number.
 * 
 * Controls:
 *   MODE: Move Left. Long press or pressing when not in a game goes to the next face.
 *   ALARM: Move Right. Long press on title shows winrate percentage. Long-pressing when looking at the winrate resets it.
 *   LIGHT: Select card to swap. Selecting the slot after the cards or long pressing swaps the selected cards.
 *          Long press on title shows winrate percentage.
 * 
 * | Cards   |                          |
 * |---------|--------------------------|
 * | Value   |2|3|4|5|6|7|8|9|10|J|Q|K|A|
 * | Display |2|3|4|5|6|7|8|9| 0|J|q|K|a|
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
