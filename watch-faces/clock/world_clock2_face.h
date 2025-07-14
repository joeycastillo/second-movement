/*
 * MIT License
 *
 * Copyright (c) 2023 Konrad Rieck
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

#ifndef WORLD_CLOCK2_FACE_H_
#define WORLD_CLOCK2_FACE_H_

/*
 * WORLD CLOCK 2
 *
 * This is an alternative world clock face that allows the user to cycle
 * through a list of selected time zones. It extends the original
 * implementation by Joey Castillo. The face has two modes: clock mode
 * and settings mode.
 *
 * Settings mode
 *
 * When the clock face is activated for the first time, it enters settings
 * mode. Here, the user can select the time zones they want to display. The
 * face shows a summary of the current time zone:
 *
 *  - The top of the face displays the first letters of the time zone
 *    abbreviation, such as "PS" for Pacific Standard Time on the classic
 *    display and "PST" on the custom display. The letters blink.
 * 
 *  - On the classic display, the upper-right corner additionally shows the 
 *    index number of the time zone. This helps avoid confusion when multiple 
 *    time zones have the same two-letter abbreviation.
 * 
 *  - The bottom display shows either the name of the time zone or its 
 *    offset from UTC. For example, it either shows "Tokyo" or "9:00" 
 *    for Japanese Standard Time.
 *
 * The user can navigate through the time zones and select them using the
 * following buttons:
 *
 *  - The ALARM button moves forward to the next time zone, while the LIGHT
 *    button moves backward to the previous zone. 
 * 
 *  - A long press on the ALARM button (de)selects the current time zone, and
 *    the signal indicator (dis)appears at the top left. 
 * 
 *  - A long press on the LIGHT button toggles the display of the time zone
 *    name or offset in the bottom display.
 * 
 *  - A press on the MODE button exits settings mode and returns to the
 *    clock mode.
 *
 * Clock mode
 *
 * In clock mode, the face shows the time of the currently selected time 
 * zone. The face includes the following components:
 * 
 *  - The top of the face displays the first letters of the time zone 
 *    abbreviation, such as "PS" for Pacific Standard Time on the classic 
 *    display and "PST" on the custom display.
 * 
 *  - On the classic display, the upper-right corner additionally shows the 
 *    index number of the time zone. This helps avoid confusion when multiple 
 *    time zones have the same two-letter abbreviation.
 *   
 *  - The main display shows the time in the selected time zone in either
 *    12-hour or 24-hour form. There is no timeout, allowing users to keep
 *    the chosen time zone displayed for as long as they wish.
 *
 * The user can navigate through the selected time zones using the following
 * buttons:
 * 
 *  - The ALARM button moves to the next selected time zone, while the LIGHT
 *    button moves to the previous zone. If no time zone is selected, the
 *    face simply shows UTC.
 * 
 *  - A long press on the ALARM button enters settings mode and enables the
 *    user to re-configure the selected time zones.
 * 
 *  - A long press on the LIGHT button activates the LED illumination of the
 *    watch.
 * 
 *  - Experimental: A single tap on the face displays the name of the selected
 *    time zone for a short moment.
 */

#include "movement.h"
#include "zones.h"

typedef enum {
    WORLD_CLOCK2_MODE_CLOCK,
    WORLD_CLOCK2_MODE_SETTINGS
} world_clock2_mode_t;

typedef struct {
    bool selected;
} world_clock2_zone_t;

typedef struct {
    world_clock2_zone_t zones[NUM_ZONE_NAMES];
    uint8_t current_zone;
    world_clock2_mode_t current_mode;
    uint32_t previous_date_time;
} world_clock2_state_t;

void world_clock2_face_setup(uint8_t watch_face_index, void **context_ptr);
void world_clock2_face_activate(void *context);
bool world_clock2_face_loop(movement_event_t event, void *context);
void world_clock2_face_resign(void *context);

#define world_clock2_face ((const watch_face_t){ \
    world_clock2_face_setup, \
    world_clock2_face_activate, \
    world_clock2_face_loop, \
    world_clock2_face_resign, \
    NULL, \
})

#endif                          /* WORLD_CLOCK2_FACE_H_ */
