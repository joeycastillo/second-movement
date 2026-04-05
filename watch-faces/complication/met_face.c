/*
 * MIT License
 *
 * Copyright (c) 2024 Agustin
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
#include "met_face.h"
#include "watch.h"
#include "watch_utility.h"

// January 1, 2083 — used to keep background task alive without ever firing
static const watch_date_time_t _met_distant_future = {
    .unit = {0, 0, 0, 1, 1, 63}
};

static bool _quick_ticks_running;

// ---------------------------------------------------------------------------
// Display helpers
// ---------------------------------------------------------------------------

static void _draw_running(met_clock_state_t *state) {
    uint32_t now_ts   = movement_get_utc_timestamp();
    uint32_t start_ts = watch_utility_date_time_to_unix_time(state->start_time, 0);
    bool countdown    = now_ts < start_ts;
    uint32_t elapsed  = countdown ? start_ts - now_ts : now_ts - start_ts;

    watch_duration_t d = watch_utility_seconds_to_duration(elapsed);
    char buf[8];

    if (state->was_countdown && !countdown) movement_play_signal();
    state->was_countdown = countdown;

    if (state->page == 0) {
        watch_set_colon();
        if (countdown) {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, " -");
        } else if (d.days < 31) {
            sprintf(buf, "%2u", (unsigned)d.days);
            watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
        } else {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
        }
        sprintf(buf, "%02d%02d  ", d.hours, d.minutes);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
        sprintf(buf, "%02d", d.seconds);
        watch_display_text(WATCH_POSITION_SECONDS, buf);
    } else {
        watch_clear_colon();
        watch_display_text(WATCH_POSITION_TOP_RIGHT, countdown ? " -" : "  ");
        if (d.days > 9999) d.days = 9999;
        sprintf(buf, "%4u  ", (unsigned)d.days);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
        watch_display_text(WATCH_POSITION_SECONDS, "dY");
    }
}

static void _draw_setting(met_clock_state_t *state, uint8_t subsecond) {
    static const char * const labels[] = {"Year ", "Month", "Day  ", "Hour ", "Minut"};
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, labels[state->selection], labels[state->selection]);

    char buf[8];
    if (state->selection <= 2) {
        sprintf(buf, "%02d%02d%02d",
            (uint8_t)((state->edit_time.unit.year + 20) % 100),
            state->edit_time.unit.month,
            state->edit_time.unit.day);
    } else {
        sprintf(buf, "%02d%02d  ",
            state->edit_time.unit.hour,
            state->edit_time.unit.minute);
    }
    watch_display_text(WATCH_POSITION_BOTTOM, buf);

    // Blink active field on odd subseconds
    if (!_quick_ticks_running && (subsecond % 2)) {
        static const watch_position_t blink_pos[] = {
            WATCH_POSITION_HOURS,   // year
            WATCH_POSITION_MINUTES, // month
            WATCH_POSITION_SECONDS, // day
            WATCH_POSITION_HOURS,   // hour
            WATCH_POSITION_MINUTES, // minute
        };
        watch_display_text(blink_pos[state->selection], "  ");
    }
}

static void _draw_confirm(void) {
    watch_display_text(WATCH_POSITION_TOP_LEFT, "    ");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    watch_display_text(WATCH_POSITION_BOTTOM, "   rSt");
    watch_clear_colon();
}

static void _enter_running(met_clock_state_t *state) {
    watch_set_colon();
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "MET", "ME");
    _draw_running(state);
}

static void _enter_unset(void) {
    watch_set_colon();
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "MET", "ME");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    watch_display_text(WATCH_POSITION_BOTTOM, "000000");
}

// ---------------------------------------------------------------------------
// Setting field helpers
// ---------------------------------------------------------------------------

static void _clamp_day(met_clock_state_t *state) {
    uint8_t dim = watch_utility_days_in_month(
        state->edit_time.unit.month, state->edit_time.unit.year + 2020);
    if (state->edit_time.unit.day > dim) state->edit_time.unit.day = dim;
}

static void _increment_field(met_clock_state_t *state) {
    switch (state->selection) {
        case 0:
            state->edit_time.unit.year = (state->edit_time.unit.year + 1) % 64;
            _clamp_day(state);
            break;
        case 1:
            state->edit_time.unit.month = (state->edit_time.unit.month % 12) + 1;
            _clamp_day(state);
            break;
        case 2: {
            uint8_t dim = watch_utility_days_in_month(
                state->edit_time.unit.month, state->edit_time.unit.year + 2020);
            state->edit_time.unit.day = (state->edit_time.unit.day % dim) + 1;
            break;
        }
        case 3:
            state->edit_time.unit.hour = (state->edit_time.unit.hour + 1) % 24;
            break;
        case 4:
            state->edit_time.unit.minute = (state->edit_time.unit.minute + 1) % 60;
            break;
        default:
            break;
    }
}

static void _abort_quick_ticks(void) {
    if (_quick_ticks_running) {
        _quick_ticks_running = false;
        movement_request_tick_frequency(4);
    }
}

// ---------------------------------------------------------------------------
// Mode loop handlers
// ---------------------------------------------------------------------------

static bool _loop_setting(met_clock_state_t *state, movement_event_t event) {
    switch (event.event_type) {
        case EVENT_TICK:
            if (_quick_ticks_running) {
                if (HAL_GPIO_BTN_ALARM_read()) _increment_field(state);
                else _abort_quick_ticks();
            }
            _draw_setting(state, event.subsecond);
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            state->selection++;
            if (state->selection > 4) {
                // Commit and transition to running
                state->edit_time.unit.second = 0;
                state->start_time = state->edit_time;
                state->setting = false;
                state->selection = 0;
                state->page = 0;
                _abort_quick_ticks();
                movement_request_tick_frequency(1);
                movement_schedule_background_task(_met_distant_future);
                _enter_running(state);
            } else {
                _draw_setting(state, event.subsecond);
            }
            break;

        case EVENT_ALARM_BUTTON_UP:
            _abort_quick_ticks();
            _increment_field(state);
            _draw_setting(state, event.subsecond);
            break;

        case EVENT_ALARM_LONG_PRESS:
            _quick_ticks_running = true;
            movement_request_tick_frequency(8);
            break;

        case EVENT_ALARM_LONG_UP:
            _abort_quick_ticks();
            break;

        case EVENT_TIMEOUT:
            break;

        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

static bool _loop_confirming(met_clock_state_t *state, movement_event_t event) {
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            _draw_confirm();
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            state->confirming = false;
            _enter_running(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            state->confirming = false;
            state->start_time.reg = 0;
            state->page = 0;
            movement_cancel_background_task();
            _enter_unset();
            break;

        case EVENT_TIMEOUT:
            break;

        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

static bool _loop_normal(met_clock_state_t *state, movement_event_t event) {
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            if (state->start_time.reg == 0) _enter_unset();
            else _enter_running(state);
            break;

        case EVENT_TICK:
            if (state->start_time.reg != 0) _draw_running(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            if (state->start_time.reg != 0) {
                state->page = !state->page;
                _draw_running(state);
            }
            break;

        case EVENT_ALARM_LONG_PRESS:
            if (state->start_time.reg == 0) {
                state->selection = 0;
                state->edit_time = movement_get_utc_date_time();
                state->edit_time.unit.second = 0;
                _quick_ticks_running = true;
                movement_request_tick_frequency(8);
                watch_clear_colon();
                _draw_setting(state, 0);
            }
            break;

        case EVENT_ALARM_LONG_UP:
            if (state->start_time.reg == 0) {
                state->setting = true;
            }
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            movement_illuminate_led();
            break;

        case EVENT_LIGHT_LONG_PRESS:
            if (state->start_time.reg != 0) {
                state->confirming = true;
                _draw_confirm();
            }
            break;

        case EVENT_TIMEOUT:
            break;

        case EVENT_LOW_ENERGY_UPDATE:
            if (!watch_sleep_animation_is_running()) watch_start_sleep_animation(1000);
            if (state->start_time.reg != 0) _draw_running(state);
            break;

        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Face lifecycle
// ---------------------------------------------------------------------------

void met_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(met_clock_state_t));
        memset(*context_ptr, 0, sizeof(met_clock_state_t));
    }
}

void met_face_activate(void *context) {
    if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();

    met_clock_state_t *state = (met_clock_state_t *)context;
    _quick_ticks_running = false;

    if (state->start_time.reg != 0) {
        movement_schedule_background_task(_met_distant_future);
    }
}

bool met_face_loop(movement_event_t event, void *context) {
    met_clock_state_t *state = (met_clock_state_t *)context;

    if (state->setting)    return _loop_setting(state, event);
    if (state->confirming) return _loop_confirming(state, event);
    return _loop_normal(state, event);
}

void met_face_resign(void *context) {
    met_clock_state_t *state = (met_clock_state_t *)context;

    if (state->setting) {
        state->setting = false;
        state->selection = 0;
        _quick_ticks_running = false;
    }
    state->confirming = false;

    movement_request_tick_frequency(1);
    movement_cancel_background_task();
}
