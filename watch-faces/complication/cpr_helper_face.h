/*
 * MIT License

 * Copyright (c) 2025 <https://github.com/metrast/>
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

#ifndef cpr_helper_FACE_H_
#define cpr_helper_FACE_H_

/*
 * This watchface is intended for professional healthcare providers!
 *
 * This watchface is designed to be a tracker for an emergency situation when performing cardiopulmonary resuscitation (CPR) and performing Advanced Life Support (ALS).
 * It tracks the time and duration of the CPR, number of defibrillations and administration of adrenaline/epinephrine.
 * It also sounds an alarm every 4 minutes as a reminder for adrenaline administration in accordance with the 2021 European Resuscitation Guidelines to give adrenaline every 3-5 minutes.
 * 
 * At the moment it's working best with the custom LCD.
 * The classic LCD has a few limitations for displaying numbers in the top row. So the display for shock count and adrenaline count is different and timestamps are displayed without seconds. Differences to the custom LCD are written in [***].
 *
 * USAGE:
 * - The first screen encountered when switching to the watchface is the timer screen. Here you can find the timer (bottom row), the counter for defibrillations ("shock-counter" -> upper left/[upper right]) and the counter for administered adrenaline ("adrenaline-counter" -> upper right/[bottom right]).
 * - The timer is started by long-pressing the ALARM button. The timer is configured to show minutes on the left and seconds on the right. It doesn't show hours. The minutes go up to 199 and the seconds to 59.
 * - When the timer is running the shock-counter or adrenaline-counter are increased by short pressing the LIGHT button or ALARM button respectively.
 * - When increasing the each counter the LED is flashed as a visualisation. The colors can be changed in the settings menu. The colors depend on the version of SensorWatch. Green and red boards have red and green LEDs. Blue boards have red and blue LEds. Pro boards have red, green and blue LEDs.
 * - The default colors for the pro and blue board are: shock = blue (4); adrenaline = red (4). For green and red board: shock = red (2), green (2); adrenaline = red (4).
 * - The timer is stopped by long–pressing the ALARM button.
 * -
 * - When the timer is stopped it automatically switches to "review-mode" and displays the elapsed time. The timer can't be started if it was stopped. It has to be reset first.
 * - In review mode one can see the timestamps of the session. The first and last being when the timer was started and stopped. And inbetween are timestamps whenever an action was done (shock/adrenaline).
 * - The first timestamp (timer started) is marked by the LAP-Indicator. The last timestamp (timer stopped) is marked by the SLEEP indicator/[BELL-Indicator].
 * - Pressing the ALARM button advances through the timestamps. Pressing the LIGHT button switches between showing elapsed time (relative) with a maximum of 199:59 (MM:SS) and timestamps (absolute) with seconds (HH:MM:SS)/[no seconds] (HH:MM).
 * - Long-pressing the LIGHT button resets the timer, deletes all the timestamps and the LED is flashed green (4). Now the timer is ready again.
 * -
 * - When the timer is not running and there are no timestamps/is reset long-pressing the MODE button activates the settings mode to set the color of the "shock-LED" and "adrenaline-LED".
 * - There are 6 pages for the pro boards. 3 LED-settings (red, green, blue) for shock count illumination (Sh) and 3 LED-settings (red, green, blue) for adrenaline count illumination (Ad). Depending on the board only the relevant LEDs can be set.
 * - Pressing the LIGHT button switches between the pages. Pressing the ALARM button increases the intensity of the chosen LED (0-15). When settings the intensity the LEDs are activated while in the settings to see the resulting color.
 * - Long-pressing the MODE button activates the home screen (timer).
 * -
 * - Pressing the MODE button anywhere in this watchface switches to the next one.
 * - 
 * THANKS:
 * - To Wesley Ellis and Joey Castillo. I incorporated some of the code of the stopwatch_face for the timer.
 */

#include "movement.h"
#define CPR_MAX_TIMESTAMPS 64

typedef struct {
    bool running : 1;
    bool in_timestamp_view : 1;
    bool show_elapsed_time_in_review : 1;
    bool in_settings_mode : 1;

    uint8_t adrenaline_count:7;
    uint8_t shock_count:7;
    
    uint8_t last_displayed_adrenaline_count : 7;
    uint8_t last_displayed_shock_count : 7;

    uint8_t timestamp_count : 6;
    uint8_t timestamp_index : 6;
    uint8_t settings_index : 4;

    uint8_t led_shock_red : 4;
    uint8_t led_shock_green : 4;
    uint8_t led_shock_blue : 4;
    uint8_t led_adrenaline_red : 4;
    uint8_t led_adrenaline_green : 4;
    uint8_t led_adrenaline_blue : 4;

    watch_date_time_t start_time;

    uint32_t seconds_counted;

    watch_date_time_t timestamps[CPR_MAX_TIMESTAMPS];
    uint32_t timestamp_elapsed[CPR_MAX_TIMESTAMPS];

    uint8_t adrenaline_counts[CPR_MAX_TIMESTAMPS];
    uint8_t shock_counts[CPR_MAX_TIMESTAMPS];
} cpr_helper_state_t;

void cpr_helper_face_setup(uint8_t watch_face_index, void ** context_ptr);
void cpr_helper_face_activate(void *context);
bool cpr_helper_face_loop(movement_event_t event, void *context);
void cpr_helper_face_resign(void *context);

#define cpr_helper_face ((const watch_face_t){ \
    cpr_helper_face_setup, \
    cpr_helper_face_activate, \
    cpr_helper_face_loop, \
    cpr_helper_face_resign, \
    NULL, \
})

#endif // cpr_helper_FACE_H_
