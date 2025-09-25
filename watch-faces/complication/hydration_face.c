/*
 * MIT License
 *
 * Copyright (c) 2025 Konrad Rieck
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
#include "hydration_face.h"
#include "watch.h"
#include "watch_utility.h"

/* Display frequency */
#define DISPLAY_FREQUENCY 1

/* Settings */
#define NUM_SETTINGS 5

/* Default values */
#define DEFAULT_WATER_GLASS_ML 100
#define DEFAULT_WATER_GOAL_ML 2000
#define DEFAULT_WAKE_HOUR 7
#define DEFAULT_SLEEP_HOUR 22
#define DEFAULT_ALERT_INTERVAL 2

static void _display_water_ml(uint16_t water_ml)
{
    char buf[10], *unit = "ml";
    if (watch_get_lcd_type() != WATCH_LCD_TYPE_CUSTOM) {
        unit = "nl";    /* nl looks more like ml on the original LCD */
    }

    snprintf(buf, sizeof(buf), "%4d%s", water_ml, unit);
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_title_display(hydration_state_t *state, char *buf1, char *buf2)
{
    char buf[10];
    watch_display_text_with_fallback(WATCH_POSITION_TOP, buf1, buf2);
    if (watch_get_lcd_type() != WATCH_LCD_TYPE_CUSTOM) {
        snprintf(buf, sizeof(buf), "%2d", state->settings_page + 1);
        watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, buf, buf);
    }
}

static bool _settings_blink(uint8_t subsecond)
{
    if (subsecond % 2 == 0) {
        watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "      ", "      ");
        return true;
    }
    return false;
}

static void _settings_water_glass_display(void *context, uint8_t subsecond)
{
    hydration_state_t *state = (hydration_state_t *) context;

    _settings_title_display(state, "GLASS", "GL");
    if (_settings_blink(subsecond))
        return;

    /* First setting so clear colon and indicators */
    watch_clear_colon();
    watch_clear_indicator(WATCH_INDICATOR_24H);
    watch_clear_indicator(WATCH_INDICATOR_PM);

    _display_water_ml(state->water_glass);
}

static void _settings_water_glass_advance(void *context)
{
    hydration_state_t *state = (hydration_state_t *) context;

    state->water_glass += 100;
    if (state->water_glass > 1000) {
        state->water_glass = 100;
    }
}

static void _settings_water_goal_display(void *context, uint8_t subsecond)
{
    hydration_state_t *state = (hydration_state_t *) context;

    _settings_title_display(state, "GOAL ", "GO");
    if (_settings_blink(subsecond))
        return;

    _display_water_ml(state->water_goal);
}

static void _settings_water_goal_advance(void *context)
{
    hydration_state_t *state = (hydration_state_t *) context;

    state->water_goal += state->water_glass;
    if (state->water_goal > 5000) {
        state->water_goal = 100;
    }
}

static void _settings_wake_time_display(void *context, uint8_t subsecond)
{
    char buf[10];
    hydration_state_t *state = (hydration_state_t *) context;

    _settings_title_display(state, "WAKE ", "WK");
    if (_settings_blink(subsecond))
        return;

    /* Start of time settings, set colon and check 24h indicator */
    watch_set_colon();

    uint8_t hour = state->wake_hour;
    if (movement_clock_mode_24h()) {
        watch_set_indicator(WATCH_INDICATOR_24H);
    } else {
        if (state->wake_hour > 12) {
            watch_set_indicator(WATCH_INDICATOR_PM);
            hour = hour % 12;
        } else {
            watch_clear_indicator(WATCH_INDICATOR_PM);
        }
    }

    snprintf(buf, sizeof(buf), "%02d00", hour);
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_wake_time_advance(void *context)
{
    hydration_state_t *state = (hydration_state_t *) context;

    state->wake_hour += 1;
    if (state->wake_hour >= 24) {
        state->wake_hour = 0;
    }
}

static void _settings_sleep_time_display(void *context, uint8_t subsecond)
{
    char buf[10];
    hydration_state_t *state = (hydration_state_t *) context;

    _settings_title_display(state, "SLEEP", "SL");
    if (_settings_blink(subsecond))
        return;

    uint8_t hour = state->sleep_hour;
    if (movement_clock_mode_24h()) {
        watch_set_indicator(WATCH_INDICATOR_24H);
    } else {
        if (state->sleep_hour >= 12) {
            watch_set_indicator(WATCH_INDICATOR_PM);
            hour = hour % 12;
        } else {
            watch_clear_indicator(WATCH_INDICATOR_PM);
        }
    }

    snprintf(buf, sizeof(buf), "%02d00", hour);
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_sleep_time_advance(void *context)
{
    hydration_state_t *state = (hydration_state_t *) context;

    state->sleep_hour += 1;
    if (state->sleep_hour >= 24) {
        state->sleep_hour = 0;
    }
}

static void _settings_alert_interval_display(void *context, uint8_t subsecond)
{
    char buf[10];
    hydration_state_t *state = (hydration_state_t *) context;

    _settings_title_display(state, "INTV", "IN");
    if (_settings_blink(subsecond))
        return;

    /* No time setting so clear colon and indicators */
    watch_clear_colon();
    watch_clear_indicator(WATCH_INDICATOR_24H);
    watch_clear_indicator(WATCH_INDICATOR_PM);

    snprintf(buf, sizeof(buf), "  %2dh ", state->alert_interval);
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_alert_interval_advance(void *context)
{
    hydration_state_t *state = (hydration_state_t *) context;

    state->alert_interval += 1;
    if (state->alert_interval > 8) {
        state->alert_interval = 1;
    }
}

/* Play beep sound */
static inline void _beep()
{
    if (!movement_button_should_sound())
        return;
    watch_buzzer_play_note(BUZZER_NOTE_C7, 50);
}

/* Calculate expected intake */
static uint16_t _get_expected_intake(hydration_state_t *state, uint8_t hours_since_wake)
{
    uint8_t day_hours = (state->sleep_hour + 24 - state->wake_hour) % 24;
    uint16_t expected_intake = (state->water_goal * hours_since_wake) / day_hours;
    return expected_intake;
}


static void _tracking_display(hydration_state_t *state)
{
    char buf[4];

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "HYDRA", "Hy");

    if (!state->display_deviation) {
        /* Display current intake in bottom */
        _display_water_ml(state->water_intake);

        /* Display percentage in top right */
        uint8_t percent = (state->water_intake * 10) / state->water_goal;
        snprintf(buf, sizeof(buf), "%2d", percent);
        watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, buf, buf);
    } else {
        watch_date_time_t now = movement_get_local_date_time();
        uint16_t hours_since_wake = (now.unit.hour + 24 - state->wake_hour) % 24;
        uint16_t expected_intake = _get_expected_intake(state, hours_since_wake);
        int16_t deviation = state->water_intake - expected_intake;

        /* Display absolute deviation from expected intake */
        _display_water_ml(abs(deviation));

        /* Display sign in top right */
        snprintf(buf, sizeof(buf), "%s", deviation >= 0 ? " +" : " -");
        watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, buf, buf);
    }
}

static void _switch_to_tracking(hydration_state_t *state)
{
    movement_request_tick_frequency(DISPLAY_FREQUENCY);
    state->page = PAGE_HYDRATION_TRACKING;

    /* No colon or indicators in tracking mode */
    watch_clear_colon();
    watch_clear_indicator(WATCH_INDICATOR_24H);
    watch_clear_indicator(WATCH_INDICATOR_PM);

    _tracking_display(state);
}

static void _switch_to_settings(hydration_state_t *state)
{
    movement_request_tick_frequency(4);
    state->page = PAGE_HYDRATION_SETTINGS;
    state->settings_page = HYDRATION_SETTING_WATER_GLASS;
    state->settings[state->settings_page].display(state, 0);
}

static void _check_hydration_alert(hydration_state_t *state)
{
    watch_date_time_t now = movement_get_local_date_time();

    /* Check for alert at sleep time */
    if (now.unit.hour == state->sleep_hour) {
        if (state->water_intake < state->water_goal) {
            movement_play_alarm();
            movement_move_to_face(state->face_index);
            return;
        }
    }

    /* Check at interval */
    uint8_t hours_since_wake = (now.unit.hour + 24 - state->wake_hour) % 24;
    if (hours_since_wake > 0 && hours_since_wake % state->alert_interval == 0) {
        uint16_t expected_intake = _get_expected_intake(state, hours_since_wake);
        if (state->water_intake < expected_intake) {
            movement_play_alarm();
            movement_move_to_face(state->face_index);
            return;
        }
    }
}

static bool _tracking_loop(movement_event_t event, void *context)
{
    hydration_state_t *state = (hydration_state_t *) context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_clear_colon();
            _tracking_display(state);
            break;
        case EVENT_TICK:
            if (state->display_deviation > 0) {
                state->display_deviation--;
            }
            _tracking_display(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            state->water_intake += state->water_glass;
            _tracking_display(state);
            _beep();
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if (state->water_intake >= state->water_glass) {
                state->water_intake -= state->water_glass;
            } else {
                state->water_intake = 0;
            }
            _tracking_display(state);
            _beep();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            /* Do nothing. No light */
            break;
        case EVENT_LIGHT_LONG_PRESS:
            _switch_to_settings(state);
            _beep();
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->display_deviation = 2;       /* Display deviation for 1-2 seconds */
            _tracking_display(state);
            _beep();
            break;
        case EVENT_BACKGROUND_TASK:
            _check_hydration_alert(state);
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

static bool _settings_loop(movement_event_t event, void *context)
{
    hydration_state_t *state = (hydration_state_t *) context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            state->settings[state->settings_page].display(context, event.subsecond);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            state->settings_page = (state->settings_page + 1) % NUM_SETTINGS;
            state->settings[state->settings_page].display(context, event.subsecond);
            break;
        case EVENT_MODE_BUTTON_UP:
            _switch_to_tracking(state);
            _beep();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            /* Do nothing. No light */
            break;
        case EVENT_ALARM_BUTTON_UP:
            /* Advance current settings */
            state->settings[state->settings_page].advance(context);
            state->settings[state->settings_page].display(context, event.subsecond);
            break;
        case EVENT_ALARM_LONG_PRESS:
            /* Reset to default value */
            switch (state->settings_page) {
                case HYDRATION_SETTING_WATER_GLASS:
                    state->water_glass = DEFAULT_WATER_GLASS_ML;
                    break;
                case HYDRATION_SETTING_WATER_GOAL:
                    state->water_goal = DEFAULT_WATER_GOAL_ML;
                    break;
                case HYDRATION_SETTING_WAKE_TIME:
                    state->wake_hour = DEFAULT_WAKE_HOUR;
                    break;
                case HYDRATION_SETTING_SLEEP_TIME:
                    state->sleep_hour = DEFAULT_SLEEP_HOUR;
                    break;
                case HYDRATION_SETTING_ALERT_INTERVAL:
                    state->alert_interval = DEFAULT_ALERT_INTERVAL;
                    break;
            }
            break;
        case EVENT_BACKGROUND_TASK:
            _check_hydration_alert(state);
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

void hydration_face_setup(uint8_t watch_face_index, void **context_ptr)
{
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(hydration_state_t));
        memset(*context_ptr, 0, sizeof(hydration_state_t));
    }
    hydration_state_t *state = (hydration_state_t *) * context_ptr;

    /* Default setup */
    state->water_glass = DEFAULT_WATER_GLASS_ML;
    state->water_goal = DEFAULT_WATER_GOAL_ML;
    state->wake_hour = DEFAULT_WAKE_HOUR;
    state->sleep_hour = DEFAULT_SLEEP_HOUR;
    state->alert_interval = DEFAULT_ALERT_INTERVAL;

    /* Initialize settings */
    uint8_t settings_page = 0;
    state->settings = malloc(NUM_SETTINGS * sizeof(hydration_settings_t));
    state->settings[settings_page].display = _settings_water_glass_display;
    state->settings[settings_page].advance = _settings_water_glass_advance;
    settings_page++;
    state->settings[settings_page].display = _settings_water_goal_display;
    state->settings[settings_page].advance = _settings_water_goal_advance;
    settings_page++;
    state->settings[settings_page].display = _settings_wake_time_display;
    state->settings[settings_page].advance = _settings_wake_time_advance;
    settings_page++;
    state->settings[settings_page].display = _settings_sleep_time_display;
    state->settings[settings_page].advance = _settings_sleep_time_advance;
    settings_page++;
    state->settings[settings_page].display = _settings_alert_interval_display;
    state->settings[settings_page].advance = _settings_alert_interval_advance;
    settings_page++;

    /* Store face index for background tasks */
    state->face_index = watch_face_index;
}

void hydration_face_activate(void *context)
{
    hydration_state_t *state = (hydration_state_t *) context;

    /* Switch to tracking page. */
    _switch_to_tracking(state);
}

bool hydration_face_loop(movement_event_t event, void *context)
{
    hydration_state_t *state = (hydration_state_t *) context;

    switch (state->page) {
        default:
        case PAGE_HYDRATION_TRACKING:
            return _tracking_loop(event, context);
        case PAGE_HYDRATION_SETTINGS:
            return _settings_loop(event, context);
    }
}

void hydration_face_resign(void *context)
{
    (void) context;
}

movement_watch_face_advisory_t hydration_face_advise(void *context)
{
    hydration_state_t *state = (hydration_state_t *) context;
    movement_watch_face_advisory_t retval = { 0 };
    watch_date_time_t now = movement_get_local_date_time();

    /* Check for daily reset at wake time */
    if (now.unit.hour == state->wake_hour && now.unit.minute == 0) {
        state->water_intake = 0;
    }

    /* Check for alert at every hour */
    if (now.unit.minute == 0) {
        retval.wants_background_task = true;
    }

    return retval;
}
