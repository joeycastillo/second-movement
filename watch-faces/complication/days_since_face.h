/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
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
 * DAYS SINCE face (previously Day One)
 *
 * This watch face displays the number of days since or until a given date.
 * It was originally designed to display the number of days you’ve been alive,
 * but technically it can count up from any date in the 20th century or the
 * 21st century, so far.
 *
 * Long press on the Alarm button to enter customization mode. The text “YR”
 * will appear, and will allow you to set the year starting from 1959. Press
 * Alarm repeatedly to advance the year. If your birthday is before 1959,
 * advance beyond the current year and it will wrap around to 1900.
 *
 * Once you have set the year, press Light to set the month (“MO”) and
 * day (“DA”), advancing the value by pressing Alarm repeatedly.
 *
 * Note that at this time, the Day One face does not display the sleep
 * indicator in sleep mode, which may make the watch appear to be
 * unresponsive in sleep mode. You can still press the Alarm button to
 * wake the watch. This UI quirk will be addressed in a future update.
 */

#include "movement.h"

typedef enum {
    PAGE_DISPLAY,
    PAGE_YEAR,
    PAGE_MONTH,
    PAGE_DAY,
    PAGE_DATE
} days_since_page_t;

typedef union {
    struct {
        uint16_t year : 12;  // good through the year 4095
        uint8_t month : 4;
        uint8_t day : 5;
        uint8_t hour : 5;
        uint8_t minute : 6;
    } bit;
    uint32_t reg;
} days_since_date_t;

typedef struct {
    days_since_page_t current_page;
    uint8_t face_index;
    uint16_t working_year;
    uint8_t working_month;
    uint8_t working_day;
    bool birthday_changed;
    bool quick_cycle;
    uint8_t ticks;
} days_since_state_t;

void days_since_face_setup(uint8_t watch_face_index, void ** context_ptr);
void days_since_face_activate(void *context);
bool days_since_face_loop(movement_event_t event, void *context);
void days_since_face_resign(void *context);

#define days_since_face ((const watch_face_t){ \
    days_since_face_setup, \
    days_since_face_activate, \
    days_since_face_loop, \
    days_since_face_resign, \
    NULL, \
})
