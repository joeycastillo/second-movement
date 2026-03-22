/*
 * MIT License
 *
 * Copyright (c) 2023-2025 Konrad Rieck
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef DEADLINE_FACE_H_
#define DEADLINE_FACE_H_

/*
 * # Deadline Face
 * 
 * This is a watch face for tracking deadlines.  It draws inspiration from
 * other watch faces of the project but focuses on keeping track of
 * deadlines.  You can enter and monitor up to four different deadlines by
 * providing their respective date and time.  The face has two modes:
 * *running mode* and *settings mode*.
 *
 * ## Running Mode
 * 
 * When the watch face is activated, it defaults to running mode.  The top
 * right corner shows the current deadline number, and the main display
 * presents the time left until the deadline.  The format of the display
 * varies depending on the remaining time.
 *
 * - When less than a day is left, the display shows the remaining hours,
 *   minutes, and seconds in the form `HH:MM:SS`.
 * 
 * - When less than a month is left, the display shows the remaining days
 *   and hours in the form `DD:HH` with the unit `dy` for days.
 * 
 * - When less than a year is left, the display shows the remaining months
 *   and days in the form `MM:DD` with the unit `mo` for months.
 * 
 * - When more than a year is left, the years and months are displayed in
 *   the form `YY:MM` with the unit `yr` for years.
 * 
 * - When a deadline has passed in the last 24 hours, the display shows
 *   `over` to indicate that the deadline has just recently been reached.
 * 
 * - When no deadline is set for a particular slot, or if a deadline has
 *   already passed by more than 24 hours, `--:--` is displayed.
 * 
 * The user can navigate in running mode using the following buttons:
 * 
 * - The *alarm button* moves the next deadline.  There are currently four
 *   slots available for deadlines.  When the last slot has been reached,
 *   pressing the button moves to the first slot.
 *
 * - A *long press* on the *alarm button* activates settings mode and
 *   enables configuring the currently selected deadline.
 * 
 * - A *long press* on the *light button* activates a deadline alarm.  The 
 *   bell icon is displayed, and the alarm will ring upon reaching any of
 *   the deadlines set.  It is important to note that the watch will not 
 *   enter low-energy sleep mode while the alarm is enabled.
 *
 *
 * ## Settings Mode
 * 
 * In settings mode, the currently selected slot for a deadline can be
 * configured by providing the date and the time.  Like running mode, the
 * top right corner of the display indicates the current deadline number. 
 * The main display shows the date and, on the next page, the time to be
 * configured.
 *
 * The user can use the following buttons in settings mode.
 *
 * - The *light button* navigates through the different date and time
 *   settings, going from year, month, day, hour, to minute.  The selected
 *   position is blinking.
 *
 * - A *long press* on the light button resets the date and time to the next
 *   day at midnight.  This is the default deadline.
 *
 * - The *alarm button* increments the currently selected position.  A *long
 *   press* on the *alarm button* changes the value faster.
 *
 * - The *mode button* exists setting mode and returns to *running mode*. 
 *   Here the selected deadline slot can be changed.
 *
 */

#include "movement.h"

/* Modes of face */
typedef enum {
    DEADLINE_RUNNING = 0,
    DEADLINE_SETTINGS
} deadline_mode_t;

/* Number of deadline dates */
#define DEADLINE_FACE_DATES (4)

/* Deadline configuration */
typedef struct {
    deadline_mode_t mode:1;
    uint8_t current_page:3;
    uint8_t current_index:2;
    uint8_t alarm_enabled:1;
    uint8_t tick_freq;
    uint8_t face_idx;
    uint32_t deadlines[DEADLINE_FACE_DATES];
} deadline_state_t;

void deadline_face_setup(uint8_t watch_face_index, void **context_ptr);
void deadline_face_activate(void *context);
bool deadline_face_loop(movement_event_t event, void *context);
void deadline_face_resign(void *context);
movement_watch_face_advisory_t deadline_face_advise(void *context);

#define deadline_face ((const watch_face_t){ \
    deadline_face_setup, \
    deadline_face_activate, \
    deadline_face_loop, \
    deadline_face_resign, \
    deadline_face_advise, \
})

#endif                          // DEADLINE_FACE_H_
