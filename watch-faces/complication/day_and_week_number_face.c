/* SPDX-License-Identifier: MIT */

/*
 * MIT License
 *
 * Copyright © 2021-2023 Joey Castillo <joeycastillo@utexas.edu> <jose.castillo@gmail.com>
 * Copyright © 2022 David Keck <davidskeck@users.noreply.github.com>
 * Copyright © 2022 TheOnePerson <a.nebinger@web.de>
 * Copyright © 2023 Jeremy O'Brien <neutral@fastmail.com>
 * Copyright © 2023 Mikhail Svarichevsky <3@14.by>
 * Copyright © 2023 Wesley Aptekar-Cassels <me@wesleyac.com>
 * Copyright © 2024 Matheus Afonso Martins Moreira <matheus.a.m.moreira@gmail.com>
 * Copyright © 2026 Chris Gillings <git@ned-ludd.com>
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

#include <stdlib.h>
#include "day_and_week_number_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_common_display.h"

#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

static void day_and_week_number_display_all(watch_date_time_t date_time) {
    char daybuf[2 + 1];
    char dayweekbuf[6 + 1];
    uint8_t weeknumber = watch_utility_get_weeknumber(date_time.unit.year+WATCH_RTC_REFERENCE_YEAR, date_time.unit.month, date_time.unit.day);
    uint16_t daynumber = watch_utility_days_since_new_year(date_time.unit.year+WATCH_RTC_REFERENCE_YEAR, date_time.unit.month, date_time.unit.day);

#if __EMSCRIPTEN__
    char logbuf[60];
    sprintf(logbuf, "Year = %d, Month = %d, Day = %d",
         date_time.unit.year+WATCH_RTC_REFERENCE_YEAR, date_time.unit.month, date_time.unit.day);
    emscripten_log(EM_LOG_CONSOLE, logbuf);

    sprintf(logbuf, "Week = %d, Day = %d", weeknumber, daynumber);
    emscripten_log(EM_LOG_CONSOLE, logbuf);
#endif

    snprintf( daybuf, sizeof(daybuf), "%2d", date_time.unit.day);
    snprintf( dayweekbuf, sizeof(dayweekbuf), "n%3d%2d", daynumber, weeknumber);

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT,
        watch_utility_get_long_weekday(date_time), watch_utility_get_weekday(date_time));
    watch_display_text(WATCH_POSITION_TOP_RIGHT, daybuf);
    watch_display_text(WATCH_POSITION_BOTTOM, dayweekbuf);
}

static void day_and_week_number_display_day_and_week_number(watch_date_time_t current) {
    day_and_week_number_display_all(current);
}

void day_and_week_number_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(day_and_week_number_face_state_t));
        day_and_week_number_face_state_t *state = (day_and_week_number_face_state_t *) *context_ptr;
        state->watch_face_index = watch_face_index;
    }
}

void day_and_week_number_face_activate(void *context) {
    day_and_week_number_face_state_t *state = (day_and_week_number_face_state_t *) context;

    // this ensures that none of the timestamp fields will match, so we can re-render them all.
    state->date_time.previous.reg = 0xFFFFFFFF;
}

bool day_and_week_number_face_loop(movement_event_t event, void *context) {
    day_and_week_number_face_state_t *state = (day_and_week_number_face_state_t *) context;
    watch_date_time_t current;
#if __EMSCRIPTEN__
    char logbuf[60];
#endif

    switch (event.event_type) {
        case EVENT_LOW_ENERGY_UPDATE:
            break;
        case EVENT_TICK:
        case EVENT_ACTIVATE:
            current = movement_get_local_date_time();

            if (current.unit.day != state->date_time.previous.unit.day) {
                // only need to update on activation or if the current day ticks over
                day_and_week_number_display_day_and_week_number(current);
            }
            state->date_time.previous = current;

            break;
        case EVENT_ALARM_BUTTON_UP:
            break;
        case EVENT_ALARM_LONG_PRESS:
            break;
        case EVENT_BACKGROUND_TASK:
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void day_and_week_number_face_resign(void *context) {
    (void) context;
}

movement_watch_face_advisory_t day_and_week_number_face_advise(void *context) {
    movement_watch_face_advisory_t retval = { 0 };
    day_and_week_number_face_state_t *state = (day_and_week_number_face_state_t *) context;

    return retval;
}
