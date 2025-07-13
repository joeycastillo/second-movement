/*
 * MIT License
 *
 * Copyright (c) 2023-2025 Konrad Rieck
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
#include "world_clock2_face.h"
#include "watch_utility.h"
#include "watch_common_display.h"
#include "watch.h"
#include "zones.h"

static bool refresh_face;
static bool show_offset;

/* Simple macros for navigation */
#define FORWARD             +1
#define BACKWARD            -1

/* Constants */
#define UTC_ZONE_INDEX      15

/* Modulo function */
static inline unsigned int mod(int a, int b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}

/* Convert watch date time to udatetime (Copied from movement.c) */
static udatetime_t _movement_convert_date_time_to_udate(watch_date_time_t dt)
{
    /* *INDENT-OFF* */
    return (udatetime_t) {
        .date.dayofmonth = dt.unit.day,
        .date.dayofweek = dayofweek(UYEAR_FROM_YEAR(dt.unit.year + WATCH_RTC_REFERENCE_YEAR), dt.unit.month, dt.unit.day),
        .date.month = dt.unit.month,.date.year = UYEAR_FROM_YEAR(dt.unit.year + WATCH_RTC_REFERENCE_YEAR),
        .time.hour = dt.unit.hour,
        .time.minute = dt.unit.minute,
        .time.second = dt.unit.second
    };
    /* *INDENT-ON* */
}

/* Find the next selected time zone */
static inline uint8_t _next_selected_zone(world_clock2_state_t *state, int direction)
{
    uint8_t i = state->current_zone;
    while (true) {
        i = mod(i + direction, NUM_ZONE_NAMES);
        /* Return next selected zone */
        if (state->zones[i].selected) {
            return i;
        }
        /* Could not find a selected zone. Return UTC */
        if (i == state->current_zone) {
            return UTC_ZONE_INDEX;
        }
    }
}

/* Play beep sound based on type */
static inline void _beep(beep_type_t beep_type)
{
    if (!movement_button_should_sound())
        return;

    switch (beep_type) {
        case BEEP_BUTTON:
            watch_buzzer_play_note(BUZZER_NOTE_C7, 50);
            break;

        case BEEP_ENABLE:
            watch_buzzer_play_note(BUZZER_NOTE_G7, 50);
            watch_buzzer_play_note(BUZZER_NOTE_REST, 75);
            watch_buzzer_play_note(BUZZER_NOTE_C8, 75);
            break;

        case BEEP_DISABLE:
            watch_buzzer_play_note(BUZZER_NOTE_C8, 50);
            watch_buzzer_play_note(BUZZER_NOTE_REST, 75);
            watch_buzzer_play_note(BUZZER_NOTE_G7, 75);
            break;
    }
}

/* Display zone abbreviation on top */
static void _display_zone_abbr(world_clock2_state_t *state, const char *abbr)
{
    char buf[11];

    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        /* Long abbreviation on custom LCD */
        sprintf(buf, "%-5s", abbr);
        watch_display_text_with_fallback(WATCH_POSITION_TOP, buf, buf);
    } else {
        /* Short abbreviation with zone number on classic LCD */
        sprintf(buf, "%.2s%2d", abbr, state->current_zone);
        watch_display_text(WATCH_POSITION_TOP_LEFT, buf);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf + 2);
    }
}

/* Get abbreviation for current zone */
static void _get_zone_info(world_clock2_state_t *state, char *abbr, uoffset_t *offset)
{
    uzone_t zone_info;
    udatetime_t dt;
    char ds;

    watch_date_time_t utc_time = watch_rtc_get_date_time();
    unpack_zone(&zone_defns[state->current_zone], "", &zone_info);
    dt = _movement_convert_date_time_to_udate(utc_time);
    ds = get_current_offset(&zone_info, &dt, offset);
    sprintf(abbr, zone_info.abrev_formatter, ds);
}

static void _clock_display(movement_event_t event, world_clock2_state_t *state)
{
    char buf[11], zone_abbr[MAX_ZONE_NAME_LEN + 1];
    watch_date_time_t date_time, utc_time;
    uint32_t previous_date_time;
    int32_t offset;
    uoffset_t zone_offset;

    /* Update indicators and colon on refresh */
    if (refresh_face) {
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
        watch_set_colon();
        if (movement_clock_mode_24h())
            watch_set_indicator(WATCH_INDICATOR_24H);
        state->previous_date_time = 0xFFFFFFFF;
        refresh_face = false;
    }

    /* Determine current time at time zone and store date/time */
    utc_time = watch_rtc_get_date_time();
    offset = movement_get_current_timezone_offset_for_zone(state->current_zone);
    date_time = watch_utility_date_time_convert_zone(utc_time, 0, offset);
    previous_date_time = state->previous_date_time;
    state->previous_date_time = date_time.reg;

    /* Energy-efficient display taken from world_clock_face.c */
    if ((date_time.reg >> 6) == (previous_date_time >> 6) && event.event_type != EVENT_LOW_ENERGY_UPDATE) {
        // everything before seconds is the same, don't waste cycles setting those segments.
        watch_display_character_lp_seconds('0' + date_time.unit.second / 10, 8);
        watch_display_character_lp_seconds('0' + date_time.unit.second % 10, 9);
    } else if ((date_time.reg >> 12) == (previous_date_time >> 12)
               && event.event_type != EVENT_LOW_ENERGY_UPDATE) {
        // everything before minutes is the same.
        sprintf(buf, "%02d%02d", date_time.unit.minute, date_time.unit.second);
        watch_display_text(WATCH_POSITION_MINUTES, buf);
        watch_display_text(WATCH_POSITION_SECONDS, buf + 2);
    } else {
        // other stuff changed; let's do it all.
        if (!movement_clock_mode_24h()) {
            // if we are in 12 hour mode, do some cleanup.
            if (date_time.unit.hour < 12) {
                watch_clear_indicator(WATCH_INDICATOR_PM);
            } else {
                watch_set_indicator(WATCH_INDICATOR_PM);
            }
            date_time.unit.hour %= 12;
            if (date_time.unit.hour == 0)
                date_time.unit.hour = 12;
        }

        /* Display day and time */
        sprintf(buf, "%02d%02d%02d", date_time.unit.hour, date_time.unit.minute, date_time.unit.second);
        watch_display_text(WATCH_POSITION_HOURS, buf + 0);
        watch_display_text(WATCH_POSITION_MINUTES, buf + 2);

        if (event.event_type == EVENT_LOW_ENERGY_UPDATE) {
            if (!watch_sleep_animation_is_running()) {
                watch_display_text(WATCH_POSITION_SECONDS, "  ");
                watch_start_sleep_animation(500);
                watch_start_indicator_blink_if_possible(WATCH_INDICATOR_COLON, 500);
            }
        } else {
            watch_display_text(WATCH_POSITION_SECONDS, buf + 4);
        }

        /* Get and display zone abbreviation */
        _get_zone_info(state, zone_abbr, &zone_offset);
        _display_zone_abbr(state, zone_abbr);
    }
}

static void _settings_display(movement_event_t event, world_clock2_state_t *state)
{
    char buf[11], zone_abbr[MAX_ZONE_NAME_LEN + 1], *zone_name;
    uoffset_t zone_offset;

    /* Update indicator and colon on refresh */
    if (refresh_face) {
        watch_clear_colon();
        watch_clear_indicator(WATCH_INDICATOR_24H);
        watch_clear_indicator(WATCH_INDICATOR_PM);
        refresh_face = false;
    }

    /* Mark selected zone */
    if (state->zones[state->current_zone].selected)
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    else
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);

    /* Display zone abbreviation on top */
    _get_zone_info(state, zone_abbr, &zone_offset);
    /* Blink up abbreviation */
    if (event.subsecond % 2) {
        sprintf(zone_abbr, "%5s", "     ");
    }
    _display_zone_abbr(state, zone_abbr);

    /* Display zone name or offset on bottom */
    if (show_offset) {
        sprintf(buf, " %3d%02d", zone_offset.hours, zone_offset.minutes);
        watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
    } else {
        zone_name = watch_utility_time_zone_name_at_index(state->current_zone);
        watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, zone_name, zone_name);
    }
}

static bool _clock_loop(movement_event_t event, world_clock2_state_t *state)
{
    char *zone_name;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
            _clock_display(event, state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            refresh_face = true;
            state->current_zone = _next_selected_zone(state, FORWARD);
            _clock_display(event, state);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            /* Do nothing. */
            break;
        case EVENT_LIGHT_BUTTON_UP:
            refresh_face = true;
            state->current_zone = _next_selected_zone(state, BACKWARD);
            _clock_display(event, state);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            movement_illuminate_led();
            break;
        case EVENT_ALARM_LONG_PRESS:
            /* Switch to settings mode */
            state->current_mode = WORLD_CLOCK2_MODE_SETTINGS;
            refresh_face = true;
            movement_request_tick_frequency(4);
            _settings_display(event, state);
            _beep(BEEP_BUTTON);
            break;
        case EVENT_MODE_BUTTON_UP:
            /* Reset frequency and move to next face */
            movement_request_tick_frequency(1);
            movement_move_to_next_face();
            break;
        case EVENT_SINGLE_TAP:
            /* Experimental: Display zone name on tap for a bit */
            zone_name = watch_utility_time_zone_name_at_index(state->current_zone);
            watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, zone_name, zone_name);
            refresh_face = true;
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

static bool _settings_loop(movement_event_t event, world_clock2_state_t *state)
{
    uint8_t zone;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
            _settings_display(event, state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            state->current_zone = mod(state->current_zone + FORWARD, NUM_ZONE_NAMES);
            _settings_display(event, state);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            state->current_zone = mod(state->current_zone + BACKWARD, NUM_ZONE_NAMES);
            _settings_display(event, state);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            /* Do nothing */
            break;
        case EVENT_ALARM_LONG_PRESS:
            /* Toggle selection of current zone */
            zone = state->current_zone;
            state->zones[zone].selected = !state->zones[zone].selected;
            _settings_display(event, state);
            if (state->zones[zone].selected) {
                _beep(BEEP_ENABLE);
            } else {
                _beep(BEEP_DISABLE);
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            show_offset = !show_offset;
            _settings_display(event, state);
            break;
        case EVENT_MODE_BUTTON_UP:
            /* Find next selected zone */
            if (!state->zones[state->current_zone].selected)
                state->current_zone = _next_selected_zone(state, FORWARD);

            /* Switch to display mode */
            state->current_mode = WORLD_CLOCK2_MODE_CLOCK;
            refresh_face = true;
            movement_request_tick_frequency(1);
            _clock_display(event, state);
            _beep(BEEP_BUTTON);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void world_clock2_face_setup(uint8_t watch_face_index, void **context_ptr)
{
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(world_clock2_state_t));
        memset(*context_ptr, 0, sizeof(world_clock2_state_t));

        /* Start in settings mode */
        world_clock2_state_t *state = (world_clock2_state_t *) * context_ptr;
        state->current_mode = WORLD_CLOCK2_MODE_SETTINGS;
        state->current_zone = UTC_ZONE_INDEX;
    }
}

void world_clock2_face_activate(void *context)
{
    world_clock2_state_t *state = (world_clock2_state_t *) context;

    switch (state->current_mode) {
        case WORLD_CLOCK2_MODE_CLOCK:
            /* Normal tick frequency */
            movement_request_tick_frequency(1);
            break;
        case WORLD_CLOCK2_MODE_SETTINGS:
            /* Faster frequency for blinking effect */
            movement_request_tick_frequency(4);
            break;
    }

    /* Set initial state */
    refresh_face = true;
    show_offset = false;
}

bool world_clock2_face_loop(movement_event_t event, void *context)
{
    world_clock2_state_t *state = (world_clock2_state_t *) context;
    switch (state->current_mode) {
        case WORLD_CLOCK2_MODE_CLOCK:
            return _clock_loop(event, state);
        case WORLD_CLOCK2_MODE_SETTINGS:
            return _settings_loop(event, state);
    }
    return false;
}

void world_clock2_face_resign(void *context)
{
    (void) context;
}
