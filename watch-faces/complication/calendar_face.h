/*
 * MIT License
 *
 * Copyright (c) 2025 Moritz Glöckl
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
 * European Calendar Watch Face
 *
 * This watch face displays the current date, weekday, and highlights public holidays for the selected country.
 *
 * Setup:
 *   - Holidays are configured in `calendar_face.c` via the `holidays[]` array.
 *     To change the country or add/remove holidays, edit the array and use macros from `calendar_face_holidays.h`.
 *     Both fixed-date and dynamic (Easter-based) holidays are supported.
 *
 * Usage:
 *   - By default, the watch face shows today's date and highlights if it is a holiday.
 *   - Hold ALARM to enter scrolling mode (shows today as starting point).
 *   - Press ALARM to scroll forward (day/month/year depending on mode).
 *   - Press LIGHT to scroll backward.
 *   - Hold ALARM or LIGHT for fast scrolling (7 days or 3 months/years).
 *   - Press MODE to switch between day, month, and year scrolling modes.
 *   - Hold MODE to exit scrolling mode and return to the default view.
 *   - Timeout or long MODE press returns to the first watch face.
 *   - The bell indicator shows when the selected date is a public holiday.
 *
 * To customize holidays, use the macros in `calendar_face_holidays.h` and update the `holidays[]` array in `calendar_face.c`.
 */

typedef enum {
    CAL_SCROLL_DAY,
    CAL_SCROLL_MONTH,
    CAL_SCROLL_YEAR
} cal_scroll_mode_t;

typedef struct {
    bool scrolling_mode;
    bool blink_state;
    uint8_t scroll_day;
    uint8_t scroll_month;
    uint8_t scroll_year;
    uint16_t scroll_tick_count;
    cal_scroll_mode_t scroll_mode;
} calendar_state_t;


void calendar_face_setup(uint8_t watch_face_index, void ** context_ptr);
void calendar_face_activate(void *context);
bool calendar_face_loop(movement_event_t event, void *context);
void calendar_face_resign(void *context);


#define calendar_face ((const watch_face_t){ \
    calendar_face_setup, \
    calendar_face_activate, \
    calendar_face_loop, \
    calendar_face_resign, \
    NULL, \
})
