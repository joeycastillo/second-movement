/*
 * MIT License
 *
 * Copyright (c) 2025 Eirik S. Morland
 * Copyright (c) 2024 Joseph Bryant
 * Copyright (c) 2023 Konrad Rieck
 * Copyright (c) 2022 Wesley Ellis
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
#include <time.h>
#include "atb_countdown_face.h"
#include "atb.h"
#include "watch.h"
#include "watch_utility.h"
#include "zones.h"

static inline void store_countdown(atb_countdown_state_t *state) {
    /* Store set countdown time */
    state->set_hours = state->hours;
    state->set_minutes = state->minutes;
    state->set_seconds = state->seconds;
}

static inline void load_countdown(atb_countdown_state_t *state) {
    /* Load set countdown time */
    state->hours = state->set_hours;
    state->minutes = state->set_minutes;
    state->seconds = state->set_seconds;
}

static void draw(atb_countdown_state_t *state, uint8_t subsecond) {
    char buf[16];

    uint32_t delta;
    div_t result;
    // The timestamp we are passing in should be relative to the current timezone.
    watch_date_time_t now = movement_get_local_date_time();
    state->now_ts = watch_utility_date_time_to_unix_time(now, movement_get_current_timezone_offset());


    ResultSet result_set;
    result_set = atb_get_next_departures(state->now_ts, "09_2", "71779");
    state->target_ts = result_set.resultSet[state->offset];

    // Check which stop we are using.
    switch (state->stopOffset) {
        case 0:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "UGL", "UA");
            watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, " 9", " 9");
            break;

        case 1:
            result_set = atb_get_next_departures(state->now_ts, "09_1", "74061");
            state->target_ts = result_set.resultSet[state->offset];
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "STO", "ST");
            watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, " 9", " 9");
            break;

        case 2:
            result_set = atb_get_next_departures(state->now_ts, "11_2", "71773");
            state->target_ts = result_set.resultSet[state->offset];
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "UGL", "UA");
            watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, "11", "11");
            break;

        case 3:
            result_set = atb_get_next_departures(state->now_ts, "11_1", "74265");
            state->target_ts = result_set.resultSet[state->offset];
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "KON", "KG");
            watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, "11", "11");
            break;
    }
    if (state->target_ts <= state->now_ts) {
        delta = 0;
    }
    else {
        delta = state->target_ts - state->now_ts;
    }
    result = div(delta, 60);
    state->seconds = result.rem;
    result = div(result.quot, 60);
    state->hours = result.quot;
    state->minutes = result.rem;
    // Blink to an alternative every now and then.
    if (state->now_ts % 4 != 0) {
        sprintf(buf, "%2d%02d%02d", state->hours, state->minutes, state->seconds);
    }
    else {
        // Convert timestamp to a H:mm situation. We want to display the time of the
        // next departure in the local time, regardless of the timezone. This is
        // mostly so its easier to validate when testing, since these should be the same.
        watch_date_time_t time_to_use = watch_utility_date_time_from_unix_time(state->target_ts, movement_get_current_timezone_offset_for_zone(UTZ_BERLIN));
        sprintf(buf, "%2d%02d%02d", time_to_use.unit.hour, time_to_use.unit.minute, 0);
    }

    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

void atb_countdown_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(atb_countdown_state_t));
        atb_countdown_state_t *state = (atb_countdown_state_t *)*context_ptr;
        memset(*context_ptr, 0, sizeof(atb_countdown_state_t));
        state->watch_face_index = watch_face_index;
        state->offset = 0;
        store_countdown(state);
    }
}

void atb_countdown_face_activate(void *context) {
    atb_countdown_state_t *state = (atb_countdown_state_t *)context;
    watch_set_colon();

    movement_request_tick_frequency(1);
}

bool atb_countdown_face_loop(movement_event_t event, void *context) {
    atb_countdown_state_t *state = (atb_countdown_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            draw(state, event.subsecond);
            break;
        case EVENT_TICK:
            draw(state, event.subsecond);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            // Let's not trigger light.
            break;
        case EVENT_LIGHT_BUTTON_UP:
            state->offset = 0;
            if (state->stopOffset < 3) {
                state->stopOffset++;
            }
            else {
                state->stopOffset = 0;
            }
            draw(state, event.subsecond);
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->offset < 4) {
                state->offset++;
            }
            else {
                state->offset = 0;
            }
            draw(state, event.subsecond);
            break;

        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;

        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void atb_countdown_face_resign(void *context) {
    atb_countdown_state_t *state = (atb_countdown_state_t *)context;
}
