/*
 * MIT License
 *
 * Copyright (c) 2024 Agustin
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

#ifndef MET_FACE_H_
#define MET_FACE_H_

/*
 * MET FACE
 *
 * Mission Elapsed Time (MET) clock. Counts elapsed time from a configured
 * start date/time (e.g. a launch time).
 *
 * Mission Elapsed Time clock that counts up from a configured UTC launch time.
 *
 * PAGES (ALARM toggles):
 *   Page 0: HH:MM:SS — days 0-30 shown in top right, hidden after day 31.
 *   Page 1: days elapsed (bottom line), with "dY" label in seconds position.
 *   A "-" in the top right indicates T- countdown (start time in the future).
 *   The hourly signal plays when T-0 is crossed.
 *
 * SETTING MODE (long-press ALARM when unset):
 *   Fields: Year → Month → Day → Hour → Minute (all UTC).
 *   LIGHT advances to the next field; after Minute the clock starts running.
 *   ALARM increments the current field; hold for quick increment.
 *   Defaults to current UTC time.
 *
 * RESET (long-press LIGHT when running):
 *   Shows confirmation screen ("rSt"). ALARM confirms, LIGHT cancels.
 */

#include "movement.h"

typedef struct {
    uint8_t page;               // 0 = HH:MM:SS, 1 = days view
    bool setting;               // true when in time-setting mode
    bool confirming;            // true when showing reset confirmation
    bool was_countdown;         // tracks previous tick's countdown state to detect T-0 crossing
    uint8_t selection;          // active field: 0=year,1=month,2=day,3=hour,4=minute
    watch_date_time_t start_time;  // configured launch time (.reg == 0 means unset)
    watch_date_time_t edit_time;   // working copy during setting mode
} met_clock_state_t;

void met_face_setup(uint8_t watch_face_index, void ** context_ptr);
void met_face_activate(void *context);
bool met_face_loop(movement_event_t event, void *context);
void met_face_resign(void *context);

#define met_face ((const watch_face_t){ \
    met_face_setup, \
    met_face_activate, \
    met_face_loop, \
    met_face_resign, \
    NULL, \
})

#endif // MET_FACE_H_
