/*
 * MIT License
 *
 * Copyright (c) 2026 Alessandro Genova
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
 * Tennis Face
 *
 * Keep track of tennis scores. Main menu lets you choose between four modes:
 *
 * SET - Standard set with tennis scoring (0, 15, 30, 40, Ad). First to 6
 *   games with a 2-game lead wins. At 6-6 a tiebreak game is played
 *   automatically (first to 7 points, win by 2).
 *
 * POINTS - Simple point counter with tiebreak serve rules. Useful for
 *   standalone tiebreaks or any point-based scoring.
 *
 * CU SET (Custom Set) - Like a set but with no automatic win condition.
 *   Games continue indefinitely. A tiebreak game can be started or ended
 *   manually with a really long press of the alarm button. In custom mode
 *   the tiebreak game has no automatic win condition, really long press
 *   the alarm button to end the tiebreak and the match.
 *
 * SAVED - Browse the last 5 saved matches. Press alarm to resume an
 *   unfinished match.
 *
 * Controls (menu):
 *   Light: cycle through modes
 *   Alarm: select mode
 *
 * Controls (scoring modes):
 *   Light: award point to Player 1
 *   Alarm: award point to Player 2
 *   Light long press: save match and return to menu
 *   Alarm long press: toggle serve indicator
 *
 * Controls (saved mode):
 *   Light: browse through saved matches
 *   Alarm: resume an unfinished match
 *   Light long press: return to menu
 *
 * The serve indicator switches automatically after each game, and follows
 * tiebreak serve rules (switch after 1st point, then every 2 points)
 * during tiebreak games.
 */

void tennis_face_setup(uint8_t watch_face_index, void ** context_ptr);
void tennis_face_activate(void *context);
bool tennis_face_loop(movement_event_t event, void *context);
void tennis_face_resign(void *context);

#define tennis_face ((const watch_face_t){ \
    tennis_face_setup, \
    tennis_face_activate, \
    tennis_face_loop, \
    tennis_face_resign, \
    NULL, \
})
