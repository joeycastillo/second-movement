/*
 * MIT License
 *
 * Copyright (c) 2022 Andreas Nebinger, building on Wesley Ellis’ countdown_face.c
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
#include "baby_511_face.h"
#include "watch.h"
#include "watch_utility.h"

static uint8_t _get_minutes(uint32_t utc_secs)
{
    return utc_secs / 60;
}

static uint8_t _get_seconds(uint32_t utc_secs)
{
    return utc_secs % 60;
}

static void add_contraction(baby_state_t * state, uint32_t now) {
    state->con_last = (state->con_last + 1) % MAX_LOGGED_CONTRACTIONS;
    if (state->con_log_full) {
        state->con_first = state->con_last;
    }
    else if (state->con_first == state->con_last)
    {
        state->con_log_full = true;
    }
    state->con_log[state->con_last] = now;
}

static void clear_old_contractions(baby_state_t * state) {
    uint32_t now = watch_utility_date_time_to_unix_time(watch_rtc_get_date_time(), 0);
    uint8_t first = state->con_first;
    uint32_t hour_ago = now - (SECS_PER_MIN * MINS_PER_HOUR);
    uint32_t oldest = state->con_log[first];
    while (oldest < hour_ago && (first != state->con_last))
    {
        first = (first + 1) % MAX_LOGGED_CONTRACTIONS;
    }
    state->con_first = first;
}

static uint8_t get_con_count(baby_state_t * state)
{
    uint8_t count;
    if (state->con_log_full)
    {
        count = MAX_LOGGED_CONTRACTIONS;
    }
    else
    {
        if (state->con_first > state->con_last)
        {
            count = (state->con_last + MAX_LOGGED_CONTRACTIONS) - state->con_first;
        }
        else
        {
            count = (state->con_last - state->con_first);
        }
    }
    return count;
}

static uint32_t get_average_contraction_spacing_sec(baby_state_t * state)
{
    clear_old_contractions(state);
    
    uint32_t seconds = (state->con_log[state->con_last] - state->con_log[state->con_first]);

    uint8_t count = get_con_count(state);
    if (count <= 1) {
        return MINS_PER_HOUR * 60;
    }
    else {
        // dividing by gaps between contractions, so subtract 1.
        return (seconds / (count - 1));
    }
    
}

static void _contract(baby_state_t *state) {
    state->con_state = contracting;
    state->last_con_start = watch_utility_date_time_to_unix_time(watch_rtc_get_date_time(), 0);
}

static void _rest(baby_state_t *state) {
    state->con_state = resting;
    watch_date_time_t now = watch_rtc_get_date_time();
    uint32_t unix_now = watch_utility_date_time_to_unix_time(now, 0);

    if ((unix_now - state->last_con_start) > (MIN_SECS_PER_CONTRACTION)) {
        // Real contraction!
        add_contraction(state, unix_now);
    }
    clear_old_contractions(state);
}

static void _draw(baby_state_t *state) {
    char mins[2];
    char secs[2];
    char cons[2];
    char state_string[3];

    if (state->con_state == resting){
        sprintf(state_string, "RES");
    }
    else {
        sprintf(state_string, "CON");
    }

    sprintf(mins, "%02u", state->now_ts / 60);
    sprintf(secs, "%02u", state->now_ts % 60);
    sprintf(cons, "%02u", get_con_count(state));
    watch_set_colon();


    watch_display_text_with_fallback(WATCH_POSITION_HOURS, mins, mins);
    watch_display_text_with_fallback(WATCH_POSITION_MINUTES, secs, secs);
    watch_display_text_with_fallback(WATCH_POSITION_SECONDS, cons, cons);
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, state_string, state_string);
}

void baby_511_face_setup(uint8_t watch_face_index, void ** context_ptr) {

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(baby_state_t));
        baby_state_t *state = (baby_state_t *)*context_ptr;
        memset(*context_ptr, 0, sizeof(baby_state_t));
        state->watch_face_index = watch_face_index;
    }
}

void baby_511_face_activate(void *context) {
    baby_state_t *state = (baby_state_t *)context;
}

bool baby_511_face_loop(movement_event_t event, void *context) {
    baby_state_t *state = (baby_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _draw(state);
            break;
        case EVENT_TICK:
            state->now_ts++;
            clear_old_contractions(state);
            _draw(state);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            if (state->con_state == resting) {
                _contract(state);
            }
            else {
                _rest(state);
            }
            _draw(state);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            state->con_state = resting;
            state->now_ts = 0;
            state->con_first = 0;
            state->con_last = 0;
            state->con_log_full = false;
            _draw(state);
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}
void baby_511_face_resign(void *context) {
}
