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
#include "watch.h"

#ifdef HAS_ACCELEROMETER

// hacky: we're just tapping into Movement's global state.
// we should make better API for this.
extern uint32_t orientation_changes;
extern uint8_t active_minutes;

static void _activity_logging_face_log_data(activity_logging_state_t *state) {
    watch_date_time_t date_time = movement_get_local_date_time();
    size_t pos = state->data_points % ACTIVITY_LOGGING_NUM_DATA_POINTS;

    state->data[pos].timestamp.reg = date_time.reg;
    state->data[pos].active_minutes = active_minutes;
    state->data[pos].orientation_changes = orientation_changes;
    active_minutes = 0;
    orientation_changes = 0;

    state->data_points++;
}

static void _activity_logging_face_update_display(activity_logging_state_t *state, bool clock_mode_24h) {
    int8_t pos = (state->data_points - 1 - state->display_index) % ACTIVITY_LOGGING_NUM_DATA_POINTS;
    char buf[8];

    watch_clear_indicator(WATCH_INDICATOR_24H);
    watch_clear_indicator(WATCH_INDICATOR_PM);
    watch_clear_colon();

    if (pos < 0) {
        // no data at this index
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "ACT L", "AC");
        watch_display_text(WATCH_POSITION_BOTTOM, "no dat");
        sprintf(buf, "%2d", state->display_index);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    } else if (state->ts_ticks) {
        // we are displaying the timestamp in response to a button press
        watch_date_time_t date_time = state->data[pos].timestamp;
        watch_set_colon();
        if (clock_mode_24h) {
            watch_set_indicator(WATCH_INDICATOR_24H);
        } else {
            if (date_time.unit.hour > 11) watch_set_indicator(WATCH_INDICATOR_PM);
            date_time.unit.hour %= 12;
            if (date_time.unit.hour == 0) date_time.unit.hour = 12;
        }
        watch_display_text(WATCH_POSITION_TOP_LEFT, "AT");
        sprintf(buf, "%2d", date_time.unit.day);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
        sprintf(buf, "%2d%02d%02d", date_time.unit.hour, date_time.unit.minute, date_time.unit.second);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    } else {
        // we are displaying the number of accelerometer wakeups and orientation changes
        watch_display_text(WATCH_POSITION_TOP, "AC");
        sprintf(buf, "%2d", state->display_index);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
        sprintf(buf, "%-3lu/%2d", state->data[pos].orientation_changes > 999 ? 999 : state->data[pos].orientation_changes, state->data[pos].active_minutes);
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
        case EVENT_BACKGROUND_TASK:
            _activity_logging_face_log_data(state);
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

movement_watch_face_advisory_t activity_logging_face_advise(void *context) {
    (void) context;
    movement_watch_face_advisory_t retval = { 0 };

    // this will get called at the top of each minute, so all we check is if we're at the top of the hour as well.
    // if we are, we ask for a background task.
    retval.wants_background_task = movement_get_local_date_time().unit.minute == 0;

    return retval;
}

#endif
