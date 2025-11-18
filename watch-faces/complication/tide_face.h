/*
 * MIT License
 *
 * Copyright (c) 2025 Mathias Kende
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
 * TIDE COMPUTATION face
 *
 * Computes the time of the next high and low tides in your areas as well as
 * their magnitudes and gives an approximation of the current tide.
 * 
 * For now, this face only handle a theoretical perfect semi-diurnal tide,
 * similarly to what some Casio watches are doing. In the future, it may be
 * possible to handle more precise computation by sending the harmonics
 * coeficients of the tides over IR.
 * 
 * To configure the face, long press the Alarm button to enter the setting mode
 * then set the time of the high tide in your area. You can move the hour using
 * the Alarm and Mode button to go up and down, then press the Light button to
 * set the minutes in the same way. Notice that if you overflow the minutes then
 * the hour will change. Similarly, when the hours overflow, the day for the
 * tide (shown in the upper right corner) is changed too (however there is no
 * direct way to set the day). You must be sure to select the right day for the
 * tide that you are currently entering.
 * Note that, if you set the high tide value for a day with a full or new moon,
 * the computation will be slightly more precise.
 * Because the Mode button is used, you must first press the Alarm button once
 * or twice, to exit the settings, before pressing Mode to exit the watch face.
 * 
 * Once configured, the face start by showing the state of the current tide
 * (low, flowing, high, or ebbing) at the top of the screen. If the tide is
 * flowing or ebbing, the bottom left part of the screen also shows the current
 * hight of the tide as a percentage of the total tide. The bottom right portion
 * of the screen shows a representation of whether the current tide is a neap
 * or spring tide, or an intermediate one.
 * 
 * You can then repeatedly press the Alarm buttom to see the time of the future
 * high and low tides. The bottow right corner has the same representation of
 * the amplitude of the tide and the top right corner has the day of the month
 * for which the tide is shown.
 * 
 * You can long press the Light button to come back to the state of the current
 * tide. Exiting and re-entering the watch face will have the same effect.
 */

void tide_face_setup(uint8_t watch_face_index, void ** context_ptr);
void tide_face_activate(void *context);
bool tide_face_loop(movement_event_t event, void *context);
void tide_face_resign(void *context);

#define tide_face ((const watch_face_t){ \
    tide_face_setup, \
    tide_face_activate, \
    tide_face_loop, \
    tide_face_resign, \
    NULL, \
})
