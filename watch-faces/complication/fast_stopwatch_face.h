/*
 * MIT License
 *
 * Copyright (c) 2022 Andreas Nebinger
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

#ifndef FAST_STOPWATCH_FACE_H_
#define FAST_STOPWATCH_FACE_H_

/*
 * STOCK STOPWATCH face
 *
 * The Stock Stopwatch face implements the original F-91W stopwatch
 * functionality, including counting hundredths of seconds and lap timing.
 *
 * Use the ALARM button to start and stop the stopwatch.
 * Press the LIGHT button while the stopwatch is running to view the lap time.
 *  (The stopwatch continues running in the background, indicated by a blinking colon.)
 * Press the LIGHT button again to switch back to the running stopwatch.
 * Press the LIGHT button when the timekeeping is stopped to reset the stopwatch.
 *
 * There are two improvements compared to the original F-91W:
 *  o When the stopwatch reaches 59:59, the counter does not simply jump back
 *    to zero but keeps track of hours in the upper right-hand corner
 *    (up to 24 hours).
 *  o Long-press the light button to toggle the LED behavior.
 *    It either turns on with each button press or remains off.
 *
 * NOTE:
 * This watch face relies heavily on static vars in fast_stopwatch.c.
 * The disadvantage is that you cannot use more than one instance of this
 * watch face on your custom firmware - but then again, who would want that?
 * The advantage is that accessing vars is more direct and faster, and we
 * can save some precious cpu cycles.  :-)
 */

#include "movement.h"

typedef struct {
    rtc_counter_t start_counter; // rtc counter when the stopwatch was started
    rtc_counter_t lap_counter;   // rtc counter when the stopwatch was lapped
    rtc_counter_t stop_counter;  // rtc counter when the stopwatch was stopped
    uint8_t status;              // the status the stopwatch is in (idle, running, stopped)
    bool slow_refresh;           // update the display slowly (same 128Hz timekeeping accuracy)
    struct {
        rtc_counter_t seconds;
        rtc_counter_t minutes;
        rtc_counter_t hours;
    } old_display;               // the digits currently being displayed on screen
} fast_stopwatch_state_t;

void fast_stopwatch_face_setup(uint8_t watch_face_index, void ** context_ptr);
void fast_stopwatch_face_activate(void *context);
bool fast_stopwatch_face_loop(movement_event_t event, void *context);
void fast_stopwatch_face_resign(void *context);

#define fast_stopwatch_face ((const watch_face_t){ \
    fast_stopwatch_face_setup, \
    fast_stopwatch_face_activate, \
    fast_stopwatch_face_loop, \
    fast_stopwatch_face_resign, \
    NULL, \
})

#endif // FAST_STOPWATCH_FACE_H_
