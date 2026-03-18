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

#include <stdlib.h>
#include <string.h>
#include "deadline_face.h"
#include "watch.h"
#include "watch_utility.h"

/* Beep types */
typedef enum {
    BEEP_BUTTON,
    BEEP_ENABLE,
    BEEP_DISABLE
} beep_type_t;

#define SETTINGS_NUM (5)
const char settings_titles[SETTINGS_NUM][6] = { "Year ", "Month", "Day  ", "Hour ", "Minut" };
const char settings_fallback_titles[SETTINGS_NUM][3] = { "YR", "MO", "DA", "HR", "M1" };

const char *running_title = "DUE";
const char *running_fallback_title = "DL";

/* Local functions */
static void _deadline_running_init(deadline_state_t * state);
static bool _deadline_running_loop(movement_event_t event, void *context);
static void _deadline_running_display(movement_event_t event, deadline_state_t * state);
static void _deadline_settings_init(deadline_state_t * state);
static bool _deadline_settings_loop(movement_event_t event, void *context);
static void _deadline_settings_display(movement_event_t event, deadline_state_t * state,
                                       watch_date_time_t date);

/* Check for leap year */
static inline bool _is_leap(int16_t y)
{
    y += 1900;
    return !(y % 4) && ((y % 100) || !(y % 400));
}

/* Modulo function */
static inline unsigned int _mod(int a, int b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}

/* Return days in month */
static inline int _days_in_month(int16_t month, int16_t year)
{
    uint8_t days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    month = _mod(month - 1, 12);

    if (month == 1 && _is_leap(year)) {
        return days[month] + 1;
    } else {
        return days[month];
    }
}

/* Play beep sound based on type */
static inline void _beep(beep_type_t beep_type)
{
    static int8_t beep_sequence[] = {
        0, 4,
        0, 6,
        0, 6,
        0
    };

    if (!movement_button_should_sound())
        return;

    switch (beep_type) {
        case BEEP_BUTTON:
            beep_sequence[0] = BUZZER_NOTE_C7;
            beep_sequence[2] = 0;
            break;

        case BEEP_ENABLE:
            beep_sequence[0] = BUZZER_NOTE_G7;
            beep_sequence[2] = BUZZER_NOTE_REST;
            beep_sequence[4] = BUZZER_NOTE_C8;
            break;

        case BEEP_DISABLE:
            beep_sequence[0] = BUZZER_NOTE_C8;
            beep_sequence[2] = BUZZER_NOTE_REST;
            beep_sequence[4] = BUZZER_NOTE_G7;
            break;
    }

    movement_play_sequence(beep_sequence, 0);
}

/* Change tick frequency */
static inline void _change_tick_freq(uint8_t freq, deadline_state_t *state)
{
    if (state->tick_freq != freq) {
        movement_request_tick_frequency(freq);
        state->tick_freq = freq;
    }
}

/* Determine index of closest deadline */
static uint8_t _closest_deadline(deadline_state_t *state)
{
    watch_date_time_t now = movement_get_local_date_time();
    uint32_t now_ts = watch_utility_date_time_to_unix_time(now, 0);
    uint32_t min_ts = UINT32_MAX;
    uint8_t min_index = 0;

    for (uint8_t i = 0; i < DEADLINE_FACE_DATES; i++) {
        /* Skip expired deadlines and those further in the future than current minimum */
        if (state->deadlines[i] < now_ts || state->deadlines[i] > min_ts) {
            continue;
        }
        min_ts = state->deadlines[i];
        min_index = i;
    }

    return min_index;
}

/* Play background alarm */
static void _background_alarm_play(deadline_state_t *state)
{
    movement_play_alarm();
    movement_move_to_face(state->face_idx);
}

/* Reset deadline to tomorrow */
static inline void _reset_deadline(deadline_state_t *state)
{
    /* Get current time and reset hours/minutes/seconds */
    watch_date_time_t date_time = movement_get_local_date_time();
    date_time.unit.second = 0;
    date_time.unit.minute = 0;
    date_time.unit.hour = 0;

    /* Add 24 hours to obtain first second of tomorrow */
    uint32_t ts = watch_utility_date_time_to_unix_time(date_time, 0);
    ts += 24 * 60 * 60;

    state->deadlines[state->current_index] = ts;
}

/* Calculate the naive difference between deadline and current time */
static void _calculate_time_remaining(watch_date_time_t dl, watch_date_time_t now, int16_t *units)
{
    units[0] = dl.unit.second - now.unit.second;
    units[1] = dl.unit.minute - now.unit.minute;
    units[2] = dl.unit.hour - now.unit.hour;
    units[3] = dl.unit.day - now.unit.day;
    units[4] = dl.unit.month - now.unit.month;
    units[5] = dl.unit.year - now.unit.year;
}

/* Format the remaining time for display */
static void _format_time_remaining(int16_t *units, char *buffer, size_t buffer_size)
{
    const int16_t years = units[5];
    const int16_t months = units[4];
    const int16_t days = units[3];
    const int16_t hours = units[2];
    const int16_t minutes = units[1];
    const int16_t seconds = units[0];

    if (years > 0) {
        /* years:months */
        snprintf(buffer, buffer_size, "%02d%02dYR", years % 100, months % 12);
    } else if (months > 0) {
        /* months:days */
        snprintf(buffer, buffer_size, "%02d%02dMO", (years * 12 + months) % 100, days % 32);
    } else if (days > 0) {
        /* days:hours */
        snprintf(buffer, buffer_size, "%02d%02ddY", days % 32, hours % 24);
    } else {
        /* hours:minutes:seconds */
        snprintf(buffer, buffer_size, "%02d%02d%02d", hours % 24, minutes % 60, seconds % 60);
    }
}

/* Correct the naive time difference calculation */
static void _correct_time_difference(int16_t *units, watch_date_time_t deadline)
{
    const uint8_t range[] = { 60, 60, 24, 30, 12, 0 };

    for (uint8_t i = 0; i < 6; i++) {
        if (units[i] < 0) {
            /* Correct remaining units */
            if (i == 3) {
                units[i] += _days_in_month(deadline.unit.month - 1, deadline.unit.year);
            } else {
                units[i] += range[i];
            }

            /* Carry over change to next unit if non-zero */
            if (i < 5 && units[i + 1] != 0) {
                units[i + 1] -= 1;
            }
        }
    }
}

/* Increment date in settings mode. Function taken from `set_time_face.c` */
static void _increment_date(deadline_state_t *state, watch_date_time_t date_time)
{
    const uint8_t days_in_month[12] = { 31, 28, 31, 30, 31, 30, 30, 31, 30, 31, 30, 31 };

    switch (state->current_page) {
        case 0:
            /* Only 10 years covered. Fix this sometime next decade */
            date_time.unit.year = ((date_time.unit.year % 10) + 1);
            break;
        case 1:
            date_time.unit.month = (date_time.unit.month % 12) + 1;
            break;
        case 2:
            date_time.unit.day = date_time.unit.day + 1;

            /* Check for leap years */
            int8_t days = days_in_month[date_time.unit.month - 1];
            if (date_time.unit.month == 2 && _is_leap(date_time.unit.year))
                days++;

            if (date_time.unit.day > days)
                date_time.unit.day = 1;
            break;
        case 3:
            date_time.unit.hour = (date_time.unit.hour + 1) % 24;
            break;
        case 4:
            date_time.unit.minute = (date_time.unit.minute + 1) % 60;
            break;
    }

    uint32_t ts = watch_utility_date_time_to_unix_time(date_time, 0);
    state->deadlines[state->current_index] = ts;
}

/* Update display in running mode */
static void _deadline_running_display(movement_event_t event, deadline_state_t *state)
{
    (void) event;

    /* Seconds, minutes, hours, days, months, years */
    int16_t units[] = { 0, 0, 0, 0, 0, 0 };
    char buf[16];

    /* Top row with face name and deadline index */
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, running_title, running_fallback_title);
    sprintf(buf, "%2d", state->current_index + 1);
    watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, buf, buf);

    /* Display indicators */
    if (state->alarm_enabled)
        watch_set_indicator(WATCH_INDICATOR_BELL);
    else
        watch_clear_indicator(WATCH_INDICATOR_BELL);

    watch_date_time_t now = movement_get_local_date_time();
    uint32_t now_ts = watch_utility_date_time_to_unix_time(now, 0);

    /* Deadline expired */
    if (state->deadlines[state->current_index] < now_ts) {
        if (state->deadlines[state->current_index] + 24 * 60 * 60 > now_ts)
            sprintf(buf, "OVER  ");
        else
            sprintf(buf, "----  ");

        watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
        return;
    }

    /* Get date time structs */
    uint32_t dl_ts = state->deadlines[state->current_index];
    watch_date_time_t deadline = watch_utility_date_time_from_unix_time(dl_ts, 0);

    /* Calculate and format time remaining */
    _calculate_time_remaining(deadline, now, units);
    _correct_time_difference(units, deadline);
    _format_time_remaining(units, buf, sizeof(buf));
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

/* Init running mode */
static void _deadline_running_init(deadline_state_t *state)
{
    (void) state;

    watch_clear_indicator(WATCH_INDICATOR_24H);
    watch_clear_indicator(WATCH_INDICATOR_PM);
    watch_set_colon();

    /* Ensure 1Hz updates only */
    _change_tick_freq(1, state);
}

/* Loop of running mode */
static bool _deadline_running_loop(movement_event_t event, void *context)
{
    deadline_state_t *state = (deadline_state_t *) context;

    if (event.event_type != EVENT_BACKGROUND_TASK)
        _deadline_running_display(event, state);

    switch (event.event_type) {
        case EVENT_ALARM_BUTTON_UP:
            _beep(BEEP_BUTTON);
            state->current_index = (state->current_index + 1) % DEADLINE_FACE_DATES;
            _deadline_running_display(event, state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            _beep(BEEP_ENABLE);
            _deadline_settings_init(state);
            state->mode = DEADLINE_SETTINGS;
            break;
        case EVENT_MODE_BUTTON_UP:
            movement_move_to_next_face();
            return false;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_LONG_PRESS:
            _beep(BEEP_BUTTON);
            state->alarm_enabled = !state->alarm_enabled;
            _deadline_running_display(event, state);
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        case EVENT_BACKGROUND_TASK:
            _background_alarm_play(state);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

/* Update display in settings mode */
static void _deadline_settings_display(movement_event_t event,
                                       deadline_state_t *state, watch_date_time_t date_time)
{
    char buf[7];

    watch_display_text_with_fallback(WATCH_POSITION_TOP, settings_titles[state->current_page],
                                     settings_fallback_titles[state->current_page]);

    if (state->current_page > 2) {
        /* Time settings */
        watch_set_colon();
        if (movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_24H) {
            /* 24h format */
            watch_set_indicator(WATCH_INDICATOR_24H);
            sprintf(buf, "%2d%02d  ", date_time.unit.hour, date_time.unit.minute);
        } else {
            /* 12h format */
            if (date_time.unit.hour < 12)
                watch_clear_indicator(WATCH_INDICATOR_PM);
            else
                watch_set_indicator(WATCH_INDICATOR_PM);
            uint8_t hour = date_time.unit.hour % 12;
            sprintf(buf, "%2d%02d  ", hour ? hour : 12, date_time.unit.minute);
        }
    } else {
        /* Date settings */
        watch_clear_colon();
        watch_clear_indicator(WATCH_INDICATOR_24H);
        watch_clear_indicator(WATCH_INDICATOR_PM);
        sprintf(buf, "%2d%02d%02d",
                date_time.unit.year + 20, date_time.unit.month, date_time.unit.day);
    }

    /* Blink up the parameter we are setting */
    if (event.subsecond % 2) {
        switch (state->current_page) {
            case 0:
            case 3:
                buf[0] = buf[1] = ' ';
                break;
            case 1:
            case 4:
                buf[2] = buf[3] = ' ';
                break;
            case 2:
                buf[4] = buf[5] = ' ';
                break;
        }
    }

    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

/* Init setting mode */
static void _deadline_settings_init(deadline_state_t *state)
{
    state->current_page = 0;

    /* Init fresh deadline to next day */
    if (state->deadlines[state->current_index] == 0) {
        _reset_deadline(state);
    }

    /* Ensure 1Hz updates only */
    _change_tick_freq(1, state);
}

/* Loop of setting mode */
static bool _deadline_settings_loop(movement_event_t event, void *context)
{
    deadline_state_t *state = (deadline_state_t *) context;
    watch_date_time_t date_time;
    date_time = watch_utility_date_time_from_unix_time(state->deadlines[state->current_index], 0);

    if (event.event_type != EVENT_BACKGROUND_TASK)
        _deadline_settings_display(event, state, date_time);

    switch (event.event_type) {
        case EVENT_TICK:
            if (state->tick_freq == 8) {
                if (HAL_GPIO_BTN_ALARM_read()) {
                    _increment_date(state, date_time);
                    _deadline_settings_display(event, state, date_time);
                } else {
                    _change_tick_freq(4, state);
                }
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            _change_tick_freq(8, state);
            break;
        case EVENT_ALARM_LONG_UP:
            _change_tick_freq(4, state);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            _beep(BEEP_BUTTON);
            _reset_deadline(state);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_BUTTON_UP:
            state->current_page = (state->current_page + 1) % SETTINGS_NUM;
            _deadline_settings_display(event, state, date_time);
            break;
        case EVENT_ALARM_BUTTON_UP:
            _change_tick_freq(4, state);
            _increment_date(state, date_time);
            _deadline_settings_display(event, state, date_time);
            break;
        case EVENT_TIMEOUT:
            _beep(BEEP_BUTTON);
            _change_tick_freq(1, state);
            state->mode = DEADLINE_RUNNING;
            movement_move_to_face(0);
            break;
        case EVENT_MODE_BUTTON_UP:
            _beep(BEEP_DISABLE);
            _deadline_running_init(state);
            _deadline_running_display(event, state);
            state->mode = DEADLINE_RUNNING;
            break;
        case EVENT_BACKGROUND_TASK:
            _background_alarm_play(state);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

/* Setup face */
void deadline_face_setup(uint8_t watch_face_index, void **context_ptr)
{
    (void) watch_face_index;
    if (*context_ptr != NULL)
        return; /* Skip setup if context available */

    /* Allocate state */
    *context_ptr = malloc(sizeof(deadline_state_t));
    memset(*context_ptr, 0, sizeof(deadline_state_t));

    /* Store face index for background tasks */
    deadline_state_t *state = (deadline_state_t *) * context_ptr;
    state->face_idx = watch_face_index;
}

/* Activate face */
void deadline_face_activate(void *context)
{
    deadline_state_t *state = (deadline_state_t *) context;

    /* Set display options */
    _deadline_running_init(state);
    state->mode = DEADLINE_RUNNING;
    state->current_index = _closest_deadline(state);
}

/* Loop face */
bool deadline_face_loop(movement_event_t event, void *context)
{
    deadline_state_t *state = (deadline_state_t *) context;
    switch (state->mode) {
        case DEADLINE_SETTINGS:
            _deadline_settings_loop(event, context);
            break;
        default:
        case DEADLINE_RUNNING:
            _deadline_running_loop(event, context);
            break;
    }

    return true;
}

/* Resign face */
void deadline_face_resign(void *context)
{
    (void) context;
}


/* Background task */
movement_watch_face_advisory_t deadline_face_advise(void *context)
{
    deadline_state_t *state = (deadline_state_t *) context;
    movement_watch_face_advisory_t retval = { 0 };

    if (!state->alarm_enabled)
        return retval;

    /* Determine closest deadline */
    watch_date_time_t now = movement_get_local_date_time();
    uint32_t now_ts = watch_utility_date_time_to_unix_time(now, 0);
    uint32_t next_ts = state->deadlines[_closest_deadline(state)];

    /* No active deadline */
    if (next_ts < now_ts)
        return retval;

    /* No deadline within next 60 seconds */
    if (next_ts >= now_ts + 60)
        return retval;

    /* Deadline within next minute. Let's set up an alarm */
    retval.wants_background_task = true;
    return retval;
}
