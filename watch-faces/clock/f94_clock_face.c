/*
 * MIT License
 *
 * Copyright © 2026 Konrad Rieck <konrad@mlsec.org>
 * Copyright © 2021-2023 Joey Castillo <joeycastillo@utexas.edu> <jose.castillo@gmail.com>
 * Copyright © 2022 David Keck <davidskeck@users.noreply.github.com>
 * Copyright © 2022 TheOnePerson <a.nebinger@web.de>
 * Copyright © 2023 Jeremy O'Brien <neutral@fastmail.com>
 * Copyright © 2023 Mikhail Svarichevsky <3@14.by>
 * Copyright © 2023 Wesley Aptekar-Cassels <me@wesleyac.com>
 * Copyright © 2024 Matheus Afonso Martins Moreira <matheus.a.m.moreira@gmail.com>
 *
 * The clock display is derived from clock_face.c.
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
#include "f94_clock_face.h"
#include "f94_ring.h"
#include "watch_utility.h"
#include "watch_common_display.h"

/* 2.4 volts seems to offer adequate warning of a low battery condition?
 * refined based on user reports and personal observations; may need further
 * adjustment. */
#ifndef LOW_BATTERY_VOLTAGE
#define LOW_BATTERY_VOLTAGE 2400
#endif

/* Ring segments of the F-94, in the order they light: clockwise from AL. */
static const watch_indicator_t _ring_segments[F94_RING_SEGMENTS] = {
    WATCH_INDICATOR_SIGNAL,     /* AL  */
    WATCH_INDICATOR_PM,         /* PM  */
    WATCH_INDICATOR_24H,        /* 24H */
    WATCH_INDICATOR_LAP,        /* SPL */
    WATCH_INDICATOR_BELL,       /* SIG */
};

/* Segment carrying the low battery warning: LAP on the F-91W, SPL here. */
#define F94_RING_BATTERY WATCH_INDICATOR_LAP

static void _beep(void)
{
    if (!movement_button_should_sound())
        return;
    watch_buzzer_play_note(BUZZER_NOTE_C7, 50);
}

static void _indicate(watch_indicator_t indicator, bool on)
{
    if (on) {
        watch_set_indicator(indicator);
    } else {
        watch_clear_indicator(indicator);
    }
}

/* Needs the hour before it is folded to 12h. */
static void _indicate_pm(watch_date_time_t date_time)
{
    _indicate(WATCH_INDICATOR_PM, date_time.unit.hour >= 12);
}

/* Restores the five segments to their default meaning. PM is only cleared
 * here; the next clock redraw sets it. */
static void _display_default(f94_state_t *state)
{
    _indicate(WATCH_INDICATOR_SIGNAL, movement_alarm_enabled());
    _indicate(WATCH_INDICATOR_BELL, state->time_signal);
    _indicate(WATCH_INDICATOR_24H, movement_clock_mode_24h() != MOVEMENT_CLOCK_MODE_12H);
    _indicate(F94_RING_BATTERY, state->battery_low);
    _indicate(WATCH_INDICATOR_PM, false);
}

/* Fills the ring up to F94_RING_SEGMENTS, then clears it again in the same
 * order. */
static bool _ring_lit(uint8_t level, uint8_t index)
{
    if (level <= F94_RING_SEGMENTS)
        return index < level;

    return index >= level - F94_RING_SEGMENTS;
}

/* Paints the ring only if the level changed. */
static void _display_ring(f94_state_t *state)
{
    if (state->ring_source == F94_RING_OFF)
        return;

    /* The low battery warning preempts the source. */
    uint8_t level = state->battery_low ? F94_LEVEL_BATTERY : f94_ring_get(state->ring_source);

    if (level == state->ring_shown)
        return;

    state->ring_shown = level;

    for (uint8_t i = 0; i < F94_RING_SEGMENTS; i++) {
        bool lit = level == F94_LEVEL_BATTERY ? _ring_segments[i] == F94_RING_BATTERY : _ring_lit(level, i);

        _indicate(_ring_segments[i], lit);
    }
}

/* Source for 60SEC: One segment every ten seconds. (F94_RING_FILL) */
static uint8_t _level_minute(void *context)
{
    (void) context;

    return movement_get_local_date_time().unit.second / 10;
}

/* Source for 5SEC: One segment every second (F94_RING_DOUBLE) */
static uint8_t _level_seconds(void *context)
{
    (void) context;

    return movement_get_local_date_time().unit.second % F94_RING_DOUBLE_MAX;
}

/* Full repaint of all five segments, source or default. */
static void _display_indicators(f94_state_t *state)
{
    /* Whatever is on screen no longer matches ring_shown. */
    state->ring_shown = F94_LEVEL_UNKNOWN;

    if (state->ring_source == F94_RING_OFF)
        _display_default(state);
    else
        _display_ring(state);
}

/* Clear all five segments */
static void _clear_indicators(f94_state_t *state)
{
    for (uint8_t i = 0; i < F94_RING_SEGMENTS; i++)
        _indicate(_ring_segments[i], false);

    state->ring_shown = F94_LEVEL_UNKNOWN;
}

static watch_date_time_t _to_12h(watch_date_time_t date_time)
{
    date_time.unit.hour %= 12;

    if (date_time.unit.hour == 0) {
        date_time.unit.hour = 12;
    }

    return date_time;
}

static void _check_battery(f94_state_t *state, watch_date_time_t date_time)
{
    /* check the battery voltage once a day */
    if (date_time.unit.day == state->last_battery_check) {
        return;
    }

    state->last_battery_check = date_time.unit.day;

    uint16_t voltage = watch_get_vcc_voltage();

    state->battery_low = voltage < LOW_BATTERY_VOLTAGE;

    /* Set the low battery indicator if battery power is low. With a source
     * selected, the next _display_ring picks this up instead. */
    if (state->ring_source == F94_RING_OFF)
        _indicate(F94_RING_BATTERY, state->battery_low);
}

static void _toggle_time_signal(f94_state_t *state)
{
    state->time_signal = !state->time_signal;

    /* No BELL indicator with a source selected; the beep is the feedback. */
    if (state->ring_source == F94_RING_OFF)
        _indicate(WATCH_INDICATOR_BELL, state->time_signal);
}

static void _display_all(watch_date_time_t date_time)
{
    char buf[8 + 1];

    snprintf(buf,
             sizeof(buf),
             movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_024H ? "%02d%02d%02d%02d" : "%2d%2d%02d%02d",
             date_time.unit.day, date_time.unit.hour, date_time.unit.minute, date_time.unit.second);

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, watch_utility_get_long_weekday(date_time),
                                     watch_utility_get_weekday(date_time));
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    watch_display_text(WATCH_POSITION_BOTTOM, buf + 2);
}

static bool _display_some(watch_date_time_t current, watch_date_time_t previous)
{
    if ((current.reg >> 6) == (previous.reg >> 6)) {
        /* everything before seconds is the same, don't waste cycles setting
         * those segments. */

        watch_display_character_lp_seconds('0' + current.unit.second / 10, 8);
        watch_display_character_lp_seconds('0' + current.unit.second % 10, 9);

        return true;

    } else if ((current.reg >> 12) == (previous.reg >> 12)) {
        /* everything before minutes is the same. */

        char buf[4 + 1];

        snprintf(buf, sizeof(buf), "%02d%02d", current.unit.minute, current.unit.second);

        watch_display_text(WATCH_POSITION_MINUTES, buf);
        watch_display_text(WATCH_POSITION_SECONDS, buf + 2);

        return true;

    } else {
        /* other stuff changed; let's do it all. */
        return false;
    }
}

static void _display_clock(f94_state_t *state, watch_date_time_t current)
{
    if (!_display_some(current, state->previous)) {
        if (movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_12H) {
            /* PM belongs to the ring with a source selected, so 12h mode
             * can't show which half of the day it is. */
            if (state->ring_source == F94_RING_OFF)
                _indicate_pm(current);
            current = _to_12h(current);
        }
        _display_all(current);
    }
}

static void _display_low_energy(f94_state_t *state, watch_date_time_t date_time)
{
    if (movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_12H) {
        if (state->ring_source == F94_RING_OFF)
            _indicate_pm(date_time);
        date_time = _to_12h(date_time);
    }
    char buf[8 + 1];

    snprintf(buf,
             sizeof(buf),
             movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_024H ? "%02d%02d%02d  " : "%2d%2d%02d  ",
             date_time.unit.day, date_time.unit.hour, date_time.unit.minute);

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, watch_utility_get_long_weekday(date_time),
                                     watch_utility_get_weekday(date_time));
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    watch_display_text(WATCH_POSITION_BOTTOM, buf + 2);
}

static void _start_sleep_animation(void)
{
    if (!watch_sleep_animation_is_running()) {
        watch_start_sleep_animation(500);
        watch_start_indicator_blink_if_possible(WATCH_INDICATOR_COLON, 500);
    }
}

static void _stop_sleep_animation(void)
{
    if (watch_sleep_animation_is_running()) {
        watch_stop_sleep_animation();
        watch_stop_blink();
    }
}

static void _settings_display(f94_state_t *state, uint8_t subsecond)
{
    const char *value = f94_ring_name(state->ring_source);

    /* The page label goes in the top left; settings mode shows no colon. */
    watch_clear_colon();
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "SO", "SO");

    /* Blink the value being edited. */
    if (subsecond % 2 == 0)
        value = "      ";
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, value, value);
}

/* Back to the clock, restoring what settings mode took over. */
static void _leave_settings(f94_state_t *state)
{
    state->page = PAGE_F94_CLOCK;
    /* this ensures that none of the timestamp fields will match, so we can
     * re-render them all. */
    state->previous.reg = 0xFFFFFFFF;
    watch_set_colon();          /* restore the colon hidden by settings */
    _display_indicators(state);
    movement_request_tick_frequency(1);
}

static bool _settings_loop(movement_event_t event, f94_state_t *state)
{
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            _settings_display(state, event.subsecond);
            break;
        case EVENT_ALARM_BUTTON_UP:
            /* The segments stay dark until we leave; the name is the feedback. */
            state->ring_source = (state->ring_source + 1) % f94_ring_count();
            _settings_display(state, event.subsecond);
            break;
        case EVENT_MODE_BUTTON_UP:
            _leave_settings(state);
            _beep();
            break;
        case EVENT_TIMEOUT:
            _leave_settings(state);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void f94_clock_face_setup(uint8_t watch_face_index, void **context_ptr)
{
    (void) watch_face_index;

    if (*context_ptr != NULL)
        return;

    f94_state_t *state = malloc(sizeof(f94_state_t));

    memset(state, 0, sizeof(*state));
    state->ring_source = F94_RING_OFF;
    state->ring_shown = F94_LEVEL_UNKNOWN;

    f94_ring_register("60SEC ", _level_minute, NULL, F94_RING_FILL);
    f94_ring_register(" 5SEC ", _level_seconds, NULL, F94_RING_DOUBLE);

    *context_ptr = state;
}

void f94_clock_face_activate(void *context)
{
    f94_state_t *state = (f94_state_t *) context;

    _stop_sleep_animation();

    watch_set_colon();

    /* Settings runs at 4 Hz to blink the value and owns the segments; the
     * previous face may have left them in any state. */
    switch (state->page) {
        case PAGE_F94_SETTINGS:
            movement_request_tick_frequency(4);
            _clear_indicators(state);
            break;
        default:
        case PAGE_F94_CLOCK:
            movement_request_tick_frequency(1);
            _display_indicators(state);
            break;
    }

    /* this ensures that none of the timestamp fields will match, so we can
     * re-render them all. */
    state->previous.reg = 0xFFFFFFFF;
}

static bool _clock_loop(movement_event_t event, f94_state_t *state)
{
    watch_date_time_t current;

    switch (event.event_type) {
        case EVENT_LOW_ENERGY_UPDATE:
            _start_sleep_animation();
            _display_low_energy(state, movement_get_local_date_time());
            /* The ring freezes here; repainting would cost power for a
             * reading that is stale until the next tick. */
            break;
        case EVENT_TICK:
        case EVENT_ACTIVATE:
            current = movement_get_local_date_time();

            _display_clock(state, current);

            _check_battery(state, current);
            _display_ring(state);

            state->previous = current;

            break;
        case EVENT_ALARM_LONG_PRESS:
            _toggle_time_signal(state);
            _beep();
            break;
        case EVENT_LIGHT_LONG_PRESS:
            state->page = PAGE_F94_SETTINGS;
            movement_request_tick_frequency(4);
            _clear_indicators(state);
            _settings_display(state, event.subsecond);
            _beep();
            break;
        case EVENT_BACKGROUND_TASK:
            if (state->time_signal && movement_get_local_date_time().unit.minute == 0)
                movement_play_signal();
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

bool f94_clock_face_loop(movement_event_t event, void *context)
{
    f94_state_t *state = (f94_state_t *) context;

    switch (state->page) {
        case PAGE_F94_SETTINGS:
            return _settings_loop(event, state);
        default:
        case PAGE_F94_CLOCK:
            return _clock_loop(event, state);
    }
}

void f94_clock_face_resign(void *context)
{
    (void) context;
}

movement_watch_face_advisory_t f94_clock_face_advise(void *context)
{
    movement_watch_face_advisory_t retval = { 0 };
    f94_state_t *state = (f94_state_t *) context;

    /* Only the chime needs a background task; skip the clock read when it is off. */
    if (state->time_signal)
        retval.wants_background_task = movement_get_local_date_time().unit.minute == 0;

    return retval;
}
