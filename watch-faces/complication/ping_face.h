/*
 * MIT License
 *
 * Copyright (c) 2025 <David Volovskiy>
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

#ifndef PING_FACE_H_
#define PING_FACE_H_

#include "movement.h"

/*
    PING face
    I saw the face made on the Ollee watch and thought it'd be fun to have on my Sensorwatch.
    https://www.instagram.com/reel/DNlTb-ERE1F/
    On the title screen, you can select a difficulty by long-pressing LIGHT or toggle sound by long-pressing ALARM.
    ALARM are used to paddle. Holding the ALARM button longer makes the paddle travel further.
    If the accelerometer is installed, you can tap the screen to move the paddle. Paddle will travel its full distance when tapping is used.
    High-score is displayed on the top-right on the title screen. During a game, the current score is displayed.

    Difficulties:
        Baby: 2 FPS
        Easy: 4 FPS
        Normal: 8 FPS
        Hard: 8 FPS and the ball travels half the half the board.

*/

typedef struct {
    uint16_t hi_score : 10;
    uint8_t difficulty : 3;
    uint8_t month_last_hi_score : 4;
    uint8_t year_last_hi_score : 6;
    uint8_t soundOn : 1;
    uint8_t tap_control_on : 1;
    uint8_t unused : 7;
} ping_state_t;

void ping_face_setup(uint8_t watch_face_index, void ** context_ptr);
void ping_face_activate(void *context);
bool ping_face_loop(movement_event_t event, void *context);
void ping_face_resign(void *context);

#define ping_face ((const watch_face_t){ \
    ping_face_setup, \
    ping_face_activate, \
    ping_face_loop, \
    ping_face_resign, \
    NULL, \
})

#endif // ping_FACE_H_

