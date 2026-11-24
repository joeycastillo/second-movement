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
#include "contraction_face.h"
#include "watch.h"
#include "watch_utility.h"

static const int8_t _con_beep[] = {BUZZER_NOTE_C8, 2, 0};
static const int8_t _rest_beep[] = {BUZZER_NOTE_C8, 2, BUZZER_NOTE_REST, 2, BUZZER_NOTE_C8, 2, 0};

static const int8_t _birthday_beep[] = {
    BUZZER_NOTE_G5, 8,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G5, 8,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_A5, 16,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G5, 16,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_C6, 16,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_B5, 32,
0};

static void _add_contraction(contraction_state_t *state, uint32_t now) {
    if ((state->con_newest == state->con_oldest) && !(state->con_log_empty))
    {
        //ran out of room!
        //need to overwrite the oldest value
        state->con_oldest = (state->con_oldest + 1) % MAX_LOGGED_CONTRACTIONS;
    }
    state->con_log_empty = false;
    state->con_log[state->con_newest] = now;
    state->con_newest = (state->con_newest + 1) % MAX_LOGGED_CONTRACTIONS;
}

static void _clear_old_contractions(contraction_state_t *state) {
    if (state->con_log_empty) {
        return;
    }
    uint32_t now = watch_utility_date_time_to_unix_time(watch_rtc_get_date_time(), 0);
    uint32_t hour_ago = now - SECS_TO_TRACK;
    uint32_t oldest = state->con_log[state->con_oldest];
    while (oldest < hour_ago && !(state->con_log_empty))
    {
        state->con_oldest = (state->con_oldest + 1) % MAX_LOGGED_CONTRACTIONS;

        if (state->con_oldest == state->con_newest) {
            state->con_log_empty = true;
        }

        oldest = state->con_log[state->con_oldest];
    }
}

static uint8_t _get_con_count(contraction_state_t *state)
{
    uint8_t count;
    if (state->con_log_empty == true)
    {
        count = 0;
    }
    else if (state->con_newest == state->con_oldest) {
        return MAX_LOGGED_CONTRACTIONS;
    }
    else
    {
        if (state->con_oldest >= state->con_newest)
        {
            count = (state->con_newest + MAX_LOGGED_CONTRACTIONS) - state->con_oldest;
        }
        else
        {
            count = (state->con_newest - state->con_oldest);
        }
    }
    return count;
}

static uint32_t get_average_contraction_spacing_sec(contraction_state_t * state) {
    uint32_t newindex = state->con_newest;
    if (newindex == 0) {
        newindex = MAX_LOGGED_CONTRACTIONS - 1;
    }
    else {
        newindex--;
    }
    // If there's one or fewer timestamps, there's no time span.
    if ((state->con_log_empty) || (newindex == state->con_oldest)) {
        return 0;
    }
    else {
        uint32_t seconds = (state->con_log[newindex] - state->con_log[state->con_oldest]);
        uint8_t count = _get_con_count(state);

        // We're averaging gaps between contractions,
        // so subtract 1 from total contractions
        return (seconds) / (count - 1);
    }
    
}

static void _contract(contraction_state_t *state) {
    watch_buzzer_play_sequence((int8_t *)_con_beep, NULL);
    state->con_state = contracting;
    state->now_ts = 0;
    state->last_con_start = watch_utility_date_time_to_unix_time(watch_rtc_get_date_time(), 0);
}

static void _rest(contraction_state_t *state) {
    state->con_state = resting;
    watch_date_time_t now = watch_rtc_get_date_time();
    uint32_t unix_now = watch_utility_date_time_to_unix_time(now, 0);

    if ((unix_now - state->last_con_start) > (MIN_SECS_PER_VALID_CONTRACTION)) {
        // Real contraction!
        _add_contraction(state, unix_now);
        state->last_con_start = unix_now;
    }
    uint32_t spacing = get_average_contraction_spacing_sec(state);
    if ( spacing < CONTRACTION_GAP_THRESHOLD_SECS && spacing != 0 && !state->chime_played)
    {
        watch_buzzer_play_sequence((int8_t *)_birthday_beep, NULL);
        state->chime_played = true;
    }
    else{
        watch_buzzer_play_sequence((int8_t *)_rest_beep, NULL);
    }
}

static void _draw(contraction_state_t *state) {
    char mins[3];
    char secs[3];
    char cons[3];
    char state_string[4];
    uint32_t time_to_display = 0;

    if (state->con_state == resting){
        sprintf(state_string, "RES");
        time_to_display = get_average_contraction_spacing_sec(state);
    }
    else {
        sprintf(state_string, "CON");
        time_to_display = state->now_ts;
    }

    if (time_to_display >=  MINS_PER_HOUR * SECS_PER_MIN) time_to_display = (MINS_PER_HOUR * SECS_PER_MIN) - 1;
    snprintf(mins, 3, "%02u", (uint8_t) (time_to_display / 60));
    snprintf(secs, 3, "%02u", (uint8_t) (time_to_display % 60));

    // Modulo to suppress compiler warnings
    snprintf(cons, 3, "%02u", _get_con_count(state) % MAX_LOGGED_CONTRACTIONS);
    watch_set_colon();

    watch_display_text_with_fallback(WATCH_POSITION_HOURS, mins, mins);
    watch_display_text_with_fallback(WATCH_POSITION_MINUTES, secs, secs);
    watch_display_text_with_fallback(WATCH_POSITION_SECONDS, cons, cons);
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, state_string, state_string);
}

void contraction_face_setup(uint8_t watch_face_index, void ** context_ptr) {

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(contraction_state_t));
        contraction_state_t *state = (contraction_state_t *)*context_ptr;
        memset(*context_ptr, 0, sizeof(contraction_state_t));
        state->watch_face_index = watch_face_index;
        state->con_oldest = 0;
        state->con_newest = 0;
        state->con_state = resting;
        state->con_log_empty = true;
        state->chime_played = false;
    }
}

void contraction_face_activate(void *context) {
    (void) context;
}

bool contraction_face_loop(movement_event_t event, void *context) {
    contraction_state_t *state = (contraction_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _draw(state);
            break;
        case EVENT_TICK:
            if (state->con_state == contracting && state->now_ts < (MINS_PER_HOUR * SECS_PER_MIN)-1){
                state->now_ts++;
            }
            _clear_old_contractions(state);
            _draw(state);
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            if (state->con_state == resting) {
                _contract(state);
            }
            else {
                _rest(state);
            }
            _draw(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->con_state = resting;
            state->now_ts = 0;
            state->con_oldest = 0;
            state->con_newest = 0;
            state->chime_played = false;
            state->con_log_empty = true;
            _draw(state);
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}
void contraction_face_resign(void *context) {
    (void) context;
}
