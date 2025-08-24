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
#include "rtccount_face.h"
#include "watch.h"
#include "sam.h"
#include "watch_utility.h"
#include "watch_common_display.h"
#include "watch_rtc.h"

typedef enum {
    RTCCOUNT_STATUS_COUNTER = 0,
    RTCCOUNT_STATUS_COUNTER_SUB,
    RTCCOUNT_STATUS_MINUTES,
    RTCCOUNT_STATUS_MINUTES_DIFF,
    RTCCOUNT_STATUS_NUMBER
} rtccount_face_status_t;

typedef struct {
    rtccount_face_status_t status;
    uint8_t frequency;
    uint32_t n_top_of_minute;
    uint32_t ref_timestamp;
} rtccount_state_t;

static const uint32_t COUNTER_MASK = (1 << 19) - 1;

static void _rtccount_face_draw(movement_event_t event, rtccount_state_t* state) {
    uint32_t counter = watch_rtc_get_counter();

    char buf[11] = "    000000\0";
    switch (state->status) {
        case RTCCOUNT_STATUS_COUNTER: {
            buf[0] = 'C';
            break;
        }

        case RTCCOUNT_STATUS_COUNTER_SUB: {
            buf[0] = 'S';
            break;
        }

        case RTCCOUNT_STATUS_MINUTES: {
            buf[0] = 'M';
            break;
        }

        case RTCCOUNT_STATUS_MINUTES_DIFF: {
            buf[0] = 'D';
            break;
        }

        default:
            break;
    }

    watch_display_string(buf, 0);

    snprintf(buf, sizeof(buf), "%u", event.subsecond);
    uint32_t len = strlen(buf);
    watch_display_string(buf, 4 - len);

    switch (state->status) {
        case RTCCOUNT_STATUS_COUNTER: {
            snprintf(buf, sizeof(buf), "%lu", counter & COUNTER_MASK);

            size_t len = strlen(buf);

            watch_display_string(buf, 10 - len);
            break;
        }

        case RTCCOUNT_STATUS_COUNTER_SUB: {
            snprintf(buf, sizeof(buf), "%lu", counter & 127);

            size_t len = strlen(buf);

            watch_display_string(buf, 10 - len);
            break;
        }

        case RTCCOUNT_STATUS_MINUTES: {
            snprintf(buf, sizeof(buf), "%lu", state->n_top_of_minute & COUNTER_MASK);

            size_t len = strlen(buf);

            watch_display_string(buf, 10 - len);
            break;
        }

        case RTCCOUNT_STATUS_MINUTES_DIFF: {
            uint32_t elapsed_minutes = (movement_get_utc_timestamp() - state->ref_timestamp) / 60;

            snprintf(buf, sizeof(buf), "%lu", (elapsed_minutes - state->n_top_of_minute) & COUNTER_MASK);

            size_t len = strlen(buf);

            watch_display_string(buf, 10 - len);
            break;
        }

        default:
            break;
    }
}

void rtccount_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(rtccount_state_t));
        memset(*context_ptr, 0, sizeof(rtccount_state_t));
        rtccount_state_t *state = (rtccount_state_t *) *context_ptr;
        state->status = RTCCOUNT_STATUS_COUNTER;
        state->frequency = 1;
        state->n_top_of_minute = 0;
        rtc_date_time_t datetime = movement_get_utc_date_time();
        state->ref_timestamp = movement_get_utc_timestamp() - datetime.unit.second;
    }
}

void rtccount_face_activate(void *context) {
    rtccount_state_t* state = (rtccount_state_t*)context;
    movement_request_tick_frequency(state->frequency);
}

bool rtccount_face_loop(movement_event_t event, void *context) {
    rtccount_state_t* state = (rtccount_state_t*)context;

    switch (event.event_type) {
        case EVENT_BACKGROUND_TASK:
            state->n_top_of_minute += 1;
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->frequency == 128) {
                state->frequency = 1;
            } else {
                state->frequency *= 2;
            }

            movement_request_tick_frequency(state->frequency);
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->n_top_of_minute = 0;
            rtc_date_time_t datetime = movement_get_utc_date_time();
            state->ref_timestamp = movement_get_utc_timestamp() - datetime.unit.second;
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            state->status = (state->status + 1) % RTCCOUNT_STATUS_NUMBER;
            _rtccount_face_draw(event, state);
            break;
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            _rtccount_face_draw(event, state);
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void rtccount_face_resign(void *context) {
    (void) context;
    movement_request_tick_frequency(1);
}

movement_watch_face_advisory_t rtccount_face_advise(void *context) {
    (void) context;
    movement_watch_face_advisory_t retval = { 0 };
    retval.wants_background_task = true;
    return retval;
}
