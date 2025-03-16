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

#include <stdlib.h>
#include <string.h>
#include "activity_logging_face.h"
#include "filesystem.h"
#include "watch.h"
#include "movement_activity.h"
#include "watch_utility.h"

#ifdef HAS_ACCELEROMETER

static void _activity_logging_face_update_display(activity_logging_state_t *state, bool clock_mode_24h) {
    char buf[8];
    uint32_t count = 0;
    movement_activity_data_point *data_points = movement_get_data_log(&count);
    int32_t pos = ((int32_t)count - 1 - (int32_t)state->display_index) % ACTIVITY_LOGGING_NUM_DATA_POINTS;
    watch_date_time_t timestamp = movement_get_local_date_time();

    // round to previous 5 minute increment
    timestamp.unit.minute = timestamp.unit.minute - (timestamp.unit.minute % 5);
    // advance backward by 5 minutes for each increment of state->display_index
    uint32_t unix_timestamp = watch_utility_date_time_to_unix_time(timestamp, movement_get_current_timezone_offset());
    unix_timestamp -= 300 * state->display_index;
    timestamp = watch_utility_date_time_from_unix_time(unix_timestamp, movement_get_current_timezone_offset());

    watch_clear_indicator(WATCH_INDICATOR_24H);
    watch_clear_indicator(WATCH_INDICATOR_PM);
    watch_clear_colon();

    if (pos < 0) {
        // no data at this index
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "LOG", "AC");
        watch_display_text(WATCH_POSITION_BOTTOM, "no dat");
        sprintf(buf, "%2d", state->display_index);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    } else if (state->ts_ticks) {
        // we are displaying the timestamp in response to a button press
        watch_set_colon();
        if (clock_mode_24h) {
            watch_set_indicator(WATCH_INDICATOR_24H);
        } else {
            if (timestamp.unit.hour > 11) watch_set_indicator(WATCH_INDICATOR_PM);
            timestamp.unit.hour %= 12;
            if (timestamp.unit.hour == 0) timestamp.unit.hour = 12;
        }
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "T*D", "AT");
        sprintf(buf, "%2d", timestamp.unit.day);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
        sprintf(buf, "%2d%02d%02d", timestamp.unit.hour, timestamp.unit.minute, 0);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    } else {
        // we are displaying the number of accelerometer wakeups and orientation changes
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "LOG", "AC");
        sprintf(buf, "%2d", state->display_index);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
        sprintf(buf, "%-3u/%2d", data_points[pos].bit.orientation_changes > 999 ? 999 : data_points[pos].bit.orientation_changes, data_points[pos].bit.stationary_minutes);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    }
}

void activity_logging_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(activity_logging_state_t));
        memset(*context_ptr, 0, sizeof(activity_logging_state_t));
    }
}

void activity_logging_face_activate(void *context) {
    activity_logging_state_t *state = (activity_logging_state_t *)context;
    state->display_index = 0;
    state->ts_ticks = 0;
}

bool activity_logging_face_loop(movement_event_t event, void *context) {
    activity_logging_state_t *state = (activity_logging_state_t *)context;
    switch (event.event_type) {
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            // light button shows the timestamp, but if you need the light, long press it.
            movement_illuminate_led();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            state->ts_ticks = 2;
            _activity_logging_face_update_display(state, movement_clock_mode_24h());
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            state->display_index = (state->display_index + 1) % ACTIVITY_LOGGING_NUM_DATA_POINTS;
            state->ts_ticks = 0;
            // fall through
        case EVENT_ACTIVATE:
            _activity_logging_face_update_display(state, movement_clock_mode_24h());
            break;
        case EVENT_TICK:
            if (state->ts_ticks && --state->ts_ticks == 0) {
                _activity_logging_face_update_display(state, movement_clock_mode_24h());
            }
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void activity_logging_face_resign(void *context) {
    (void) context;
}

#endif
