/*
 * MIT License
 *
 * Copyright (c) 2025 Alessandro Genova
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

/*
 * PIN face
 *
 * Adding the PIN face to a build will automatically enable and lock the PIN service.
 * In the PIN face UI the user will be able to:
 * - Lock/Unlock the PIN service by entering the current PIN
 * - Change the current PIN by entering the OLD PIN, the NEW PIN, and confirming the NEW PIN again.
 * - Configure when the PIN service will be automatically locked after successfully unlocking (5min default).
 * 
 * A PIN is a 6 digit sequence. When entering a PIN through this face, each button press will correspond to a new digit.
 * The encoding from button press to numerical value is the following:
 * MODE_DOWN   ->  0
 * MODE_LONG   ->  1
 * LIGHT_DOWN  ->  2
 * LIGHT_LONG  ->  3
 * ALARM_DOWN  ->  4
 * ALARM_LONG  ->  5
 * 
 * Navigation:
 * - In the menus use the LIGHT button to advance and the ALARM button to select the option.
 * - After a failed PIN attempt press the ALARM button to try again, and the LIGHT button to go back to main menu.
 */

#include "movement.h"

void pin_face_setup(uint8_t watch_face_index, void ** context_ptr);
void pin_face_activate(void *context);
bool pin_face_loop(movement_event_t event, void *context);
void pin_face_resign(void *context);
movement_watch_face_advisory_t pin_face_advise(void *context);

#define pin_face ((const watch_face_t){ \
    pin_face_setup, \
    pin_face_activate, \
    pin_face_loop, \
    pin_face_resign, \
    pin_face_advise, \
})
