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

#ifndef BLACKJACK_FACE_H_
#define BLACKJACK_FACE_H_

#include "movement.h"

/*
 * Blackjack face
 * ======================
 *
 * Simple blackjack game.
 *
 * Aces are 11 unless you'd but, and if so, they become 1.
 * King, Queen, and jack are all 10 points.
 * Dealer deals to themselves until they get at least 17.
 * The game plays with one shuffled deck that gets reshuffled with every game.
 * 
 * Press either ALARM or LIGHT to begin playing.
 * Your score is in the Seconds position.
 * The dealer's score is in the Top-Right position.
 * The dealer's last-shown card is in the Top-Left position.
 * Your cards are in the Bottom row. From left to right, they are oldest to newest. Up to four cards will be dislayed.
 * 
 * To hit, press the ALARM button.
 * To stand, press the LIGHT button.
 * If you're at 21, you cannoy hit, since we just assume it's a mispress on the button.
 * 
 * Once you stand, the dealer will deal out to themselves once per second (or immidietly when you press the LIGHT or ALARM buttons).
 * The game results are:
 * WIN: You have a higher score than the dealer, but no more than 21. Or the dealer's score is over 21.
 * LOSE: Your score is lower than the dealer's.
 * BUST: Your score is above 21.
 * TIE: Your score matches the dealer's final score
 * 
 * On a watch that has the accelerometer, long-pressing the ALARM button will turn on the ability to play by tapping.
 * The SIGNAL indicator will display when tapping is enabled.
 * Tapping once will behave like the ALARM button and hit.
 * Tapping twice behave like the LIGHT button and stand.
 * 
 * | Cards   |                          |
 * |---------|--------------------------|
 * | Value   |2|3|4|5|6|7|8|9|10|J|Q|K|A|
 * | Display |2|3|4|5|6|7|8|9| 0|-|=|≡|a|
 * If you're using a custom display, Ace will display as 'A', not 'a'
 */



typedef struct {
    bool tap_control_on;
    uint16_t games_played;
    uint16_t games_won;
} blackjack_face_state_t;

void blackjack_face_setup(uint8_t watch_face_index, void ** context_ptr);
void blackjack_face_activate(void *context);
bool blackjack_face_loop(movement_event_t event, void *context);
void blackjack_face_resign(void *context);

#define blackjack_face ((const watch_face_t){ \
    blackjack_face_setup, \
    blackjack_face_activate, \
    blackjack_face_loop, \
    blackjack_face_resign, \
    NULL, \
})

#endif // blackjack_FACE_H_
