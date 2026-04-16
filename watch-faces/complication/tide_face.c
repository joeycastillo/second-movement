/*
 * MIT License
 *
 * Copyright (c) 2025 Mathias Kende
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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "tide_face.h"
#include "watch.h"
#include "watch_common_display.h"
#include "watch_utility.h"

// Parameters taken from the moon_phase_face.c file.
#define LUNAR_DAYS 29.53058770576
#define FIRST_MOON 947182440 // Saturday, 6 January 2000 18:14:00 in unix epoch time
#define SEMI_DIURNAL_TIDAL_PERIOD (LUNAR_DAYS / (LUNAR_DAYS - 1) * 12 * 3600) // 12h25m in seconds

#ifndef M_PI
#define M_PI 3.14159265358979
#endif

typedef enum {
    TIDE_SPRING,  // Less than 1.8 days away from a full or new moon.
    TIDE_NEAP,    // Less than 1.8 days away from a first or third quarter moon.
    TIDE_MEDIUM,  // The rest
} tide_amplitude_t;

static tide_amplitude_t _get_tide_amplitude(uint32_t time) {
    // Moon age in days, looped over beetween new and full moon (so age is 14.7 days at most).
    double moon_age = fmod(((double)(time - FIRST_MOON)) / 86400, LUNAR_DAYS / 2);

    if (moon_age <= LUNAR_DAYS / 16 || moon_age >= LUNAR_DAYS * 7 / 16) {
        return TIDE_SPRING;
    } else if (moon_age > LUNAR_DAYS * 3 / 16 && moon_age < LUNAR_DAYS * 5 / 16) {
        return TIDE_NEAP;
    } else {
        return TIDE_MEDIUM;
    }
}

typedef enum {
    TIDE_SCREEN_EMPTY,        // No tide data set.
    TIDE_SCREEN_CURRENT,      // Default screen, showing the current tide.
    TIDE_SCREEN_FUTURE,       // Screen showing the time of future high and low tides.
    TIDE_SCREEN_SETTING_HOUR, // Setting screen, setting the hour of the next high tide.
    TIDE_SCREEN_SETTING_MIN,  // Setting screen, setting the minute of the next high tide.
} tide_mode_t;

typedef enum {
    TIDE_HIGH,
    TIDE_LOW,
} tide_type_t;

typedef struct {
    tide_mode_t mode;
    bool start_setting;  // we entered the setting mode but did not yet changed any value.
    uint32_t next_high_tide;
    uint32_t last_current_update_time;
    uint32_t future_tide_time;
    tide_type_t future_tide_type;
} tide_state_t;

void tide_face_setup(uint8_t watch_face_index, void** state_ptr) {
    (void) watch_face_index;
    if (*state_ptr == NULL) {
        // Boot time initialization.
        *state_ptr = malloc(sizeof(tide_state_t));
        tide_state_t* state = (tide_state_t*)*state_ptr;
        state->mode = TIDE_SCREEN_EMPTY;
    }
}

static uint32_t _get_current_unix_time() {
    return watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), 0);
}

static void _move_next_high_tide(tide_state_t* state, uint32_t now) {
    while (state->next_high_tide > now + SEMI_DIURNAL_TIDAL_PERIOD) {
        state->next_high_tide -= SEMI_DIURNAL_TIDAL_PERIOD;
    }
    while (state->next_high_tide < now) {
        state->next_high_tide += SEMI_DIURNAL_TIDAL_PERIOD;
    }
}

void tide_face_activate(void* context) {
    tide_state_t* state = (tide_state_t*)context;
    if (state->mode != TIDE_SCREEN_EMPTY) {
      state->mode = TIDE_SCREEN_CURRENT;
    }
    // int64 so that the substraction below works (we need a signed number and int32 will overflow soon).
    // using int64 everywhere for the unix time would probably be better.
    int64_t now = _get_current_unix_time();
    if (llabs(now - state->next_high_tide) > 60 * 86400) {
        // We revert to the empty mode if the next high tide is more than 2
        // months from now, to avoid accumulating too much errors.
        state->mode = TIDE_SCREEN_EMPTY;
        return;
    }
    _move_next_high_tide(state, now);
}

static void _set_pixel(segment_mapping_t mapping) {
    watch_set_pixel(mapping.address.com, mapping.address.seg);
}

static void _draw_tide_amplitude(uint32_t time) {
    const digit_mapping_t* digit_mapping =
        watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC ? Classic_LCD_Display_Mapping : Custom_LCD_Display_Mapping;
    switch (_get_tide_amplitude(time)) {
        case TIDE_SPRING:
            _set_pixel(digit_mapping[9].segment[0]);  // top horizontal bar on the bottom-right character.
            // fall-through
        case TIDE_MEDIUM:
            _set_pixel(digit_mapping[9].segment[6]);  // mid horizontal bar on the bottom-right character.
            // fall-through
        case TIDE_NEAP:
            _set_pixel(digit_mapping[9].segment[3]);  // bottom horizontal bar on the bottom-right character.
            break;
    }
}

static void _draw_day_and_time(uint32_t time, bool show_day, bool show_hour, bool show_minute) {
    watch_date_time_t date_time = 
        watch_utility_date_time_from_unix_time(time, movement_get_current_timezone_offset());
    bool pm = false;
    if (movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_12H) {
        pm = watch_utility_convert_to_12_hour(&date_time);
    } else {
        watch_set_indicator(WATCH_INDICATOR_24H);
    }
    if (pm) {
        watch_set_indicator(WATCH_INDICATOR_PM);
    }

    if (show_hour) {
        char tide_hour[3];
        sprintf(tide_hour, "%2u", date_time.unit.hour);
        watch_display_text(WATCH_POSITION_HOURS, tide_hour);
    }
    if (show_minute) {
        char tide_minute[3];
        sprintf(tide_minute, "%02u", date_time.unit.minute);
        watch_display_text(WATCH_POSITION_MINUTES, tide_minute);
    }
    if (show_day) {
        char tide_day[3];
        sprintf(tide_day, "%2u", date_time.unit.day);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, tide_day);
    }

    watch_set_colon();
}

static void _draw(tide_state_t *state, uint32_t now, uint8_t subsecond) {
    watch_clear_display();
    switch (state->mode) {
        case TIDE_SCREEN_EMPTY:
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "TIDE", "TI");
            watch_display_text(WATCH_POSITION_BOTTOM, "----");
            break;
        case TIDE_SCREEN_CURRENT: {
            double tide_age = state->next_high_tide - now;
            _draw_tide_amplitude(now);
            double tide_percent = (cos(tide_age / SEMI_DIURNAL_TIDAL_PERIOD * M_PI * 2) + 1) * 50;
            if (tide_percent < 5) {
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "LOW", "LO");
            } else if (tide_percent > 95) {
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "HIGH", "HI");
            } else {
                if (state->next_high_tide - now < SEMI_DIURNAL_TIDAL_PERIOD / 2) {
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "FLOOd", "FL");
                } else {
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "EBB", "EB");
                }
                if (watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC) {
                    uint8_t tide_upercent = tide_percent;
                    char hour[2];
                    char minute[2];
                    hour[0] = minute[1] = ' ';
                    hour[1] = '0' + tide_upercent / 10;
                    minute[0] = '0' + tide_upercent % 10;
                    // We use the second hour digit for our first digit, as it’s
                    // more capable than the first hour or minute digits.
                    watch_display_text(WATCH_POSITION_HOURS, hour);
                    watch_display_text(WATCH_POSITION_MINUTES, minute);
                } else {
                    char tide_text[7];
                    uint8_t tide_upercent = tide_percent;
                    sprintf(tide_text, "%2hhu", tide_upercent);
                    watch_display_text(WATCH_POSITION_HOURS, tide_text);
                    watch_display_text(WATCH_POSITION_MINUTES, "o#");  // # is rendered as °, o° looks like a percent sign, maybe...
                }
            }
            break;
        }
        case TIDE_SCREEN_FUTURE:
            if (state->future_tide_type == TIDE_LOW) {
                watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "LOW", "LO");
            } else {
                watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "HIG", "HI");
            }
            _draw_day_and_time(state->future_tide_time, true, true, true);
            _draw_tide_amplitude(state->future_tide_time);
            break;
        case TIDE_SCREEN_SETTING_HOUR:
        case TIDE_SCREEN_SETTING_MIN:
            if (state->start_setting) {
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "HIGH", "HI");
            } else {
                watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "HIG", "HI");
            }
            _draw_day_and_time(state->next_high_tide, !state->start_setting,
                               (state->mode != TIDE_SCREEN_SETTING_HOUR || subsecond % 2), (state->mode != TIDE_SCREEN_SETTING_MIN || subsecond % 2));
            break;
    }
}

static void _offset_next_high_tide(tide_state_t* state, int16_t offset) {
    state->next_high_tide += offset;
    if (state->next_high_tide % 60) {
        state->next_high_tide -= state->next_high_tide % 60;
    }
    state->start_setting = false;
}

bool tide_face_loop(movement_event_t event, void* context) {
    tide_state_t* state = (tide_state_t*)context;
    uint32_t now = _get_current_unix_time();

    // TODO: handle long press in setting mode.
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _draw(state, now, event.subsecond);
            if (state->mode == TIDE_SCREEN_CURRENT) {
                state->last_current_update_time = now;
            }
            break;
        case EVENT_TICK:
            switch (state->mode) {
                case TIDE_SCREEN_CURRENT:
                    if (now - state->last_current_update_time >= 60) {
                        _move_next_high_tide(state, now);    
                        _draw(state, now, event.subsecond);
                        state->last_current_update_time = now;
                    }
                    break;
                case TIDE_SCREEN_SETTING_HOUR:
                case TIDE_SCREEN_SETTING_MIN:
                    _draw(state, now, event.subsecond);
                    break;
                default:
                    break;
            }
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            _draw(state, now, event.subsecond);
            if (watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC) {
                watch_start_sleep_animation(500);
            } else {
                watch_set_indicator(WATCH_INDICATOR_SLEEP);
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            switch (state->mode) {
                case TIDE_SCREEN_SETTING_HOUR:
                    state->mode = TIDE_SCREEN_SETTING_MIN;
                    _draw(state, now, event.subsecond);
                    break;
                case TIDE_SCREEN_SETTING_MIN:
                    state->mode = TIDE_SCREEN_CURRENT;
                    _move_next_high_tide(state, _get_current_unix_time());
                    movement_request_tick_frequency(1);
                    _draw(state, now, event.subsecond);
                    break;
                default:
                    movement_illuminate_led();
                    break;
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (state->mode == TIDE_SCREEN_FUTURE) {
                state->mode = TIDE_SCREEN_CURRENT;
                _draw(state, now, event.subsecond);
                state->last_current_update_time = now;
            }
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            switch(state->mode) {
                case TIDE_SCREEN_SETTING_HOUR:
                    _offset_next_high_tide(state, 3600);
                    break;
                case TIDE_SCREEN_SETTING_MIN:
                    _offset_next_high_tide(state, 60);
                    break;
                default:
                    break;
            }
            _draw(state, now, event.subsecond);
            break;
        case EVENT_ALARM_BUTTON_UP:
            // We react to UP event only so that we don’t switch to a TIDE_SCREEN_FUTURE day at the beginning of a long press.
            switch(state->mode) {
                case TIDE_SCREEN_CURRENT:
                    if (state->next_high_tide - now > SEMI_DIURNAL_TIDAL_PERIOD / 2) {
                        state->future_tide_time = state->next_high_tide - SEMI_DIURNAL_TIDAL_PERIOD / 2;
                        state->future_tide_type = TIDE_LOW;
                    } else {
                        state->future_tide_time = state->next_high_tide;
                        state->future_tide_type = TIDE_HIGH;
                    }
                    state->mode = TIDE_SCREEN_FUTURE;
                    break;
                case TIDE_SCREEN_FUTURE:
                    state->future_tide_time += SEMI_DIURNAL_TIDAL_PERIOD / 2;
                    state->future_tide_type = state->future_tide_type == TIDE_LOW ? TIDE_HIGH : TIDE_LOW;
                    break;
                default:
                    break;
            }
            _draw(state, now, event.subsecond);
            break;
        case EVENT_ALARM_LONG_PRESS:
            switch(state->mode) {
                case TIDE_SCREEN_EMPTY:
                  state->next_high_tide = _get_current_unix_time();
                  // fall-through
                case TIDE_SCREEN_CURRENT:
                case TIDE_SCREEN_FUTURE:
                    state->mode = TIDE_SCREEN_SETTING_HOUR;
                    state->start_setting = true;
                    movement_request_tick_frequency(4);
                    break;
                case TIDE_SCREEN_SETTING_HOUR:
                case TIDE_SCREEN_SETTING_MIN:
                    break;
            }
            _draw(state, now, event.subsecond);
            break;
        case EVENT_MODE_BUTTON_DOWN:
            switch(state->mode) {
                case TIDE_SCREEN_SETTING_HOUR:
                    _offset_next_high_tide(state, -3600);
                    break;
                case TIDE_SCREEN_SETTING_MIN:
                    _offset_next_high_tide(state, -60);
                    break;
                default:
                    return movement_default_loop_handler(event);
            }
            _draw(state, now, event.subsecond);
            break;
        case EVENT_MODE_BUTTON_UP:
        case EVENT_MODE_LONG_PRESS:
            switch(state->mode) {
                case TIDE_SCREEN_SETTING_HOUR:
                case TIDE_SCREEN_SETTING_MIN:
                    break;
                default:
                    return movement_default_loop_handler(event);
            }
            break;
        case EVENT_TIMEOUT:
            if (state->mode == TIDE_SCREEN_SETTING_MIN || state->mode == TIDE_SCREEN_SETTING_HOUR) {
                state->mode = TIDE_SCREEN_CURRENT;
                _draw(state, now, event.subsecond);
            }
            // fall-through
            // Delegate the resign behavior to the default loop handler.
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void tide_face_resign(void* context) {
    // Any cleanup needed before the watch face goes off-screen.
    tide_state_t* state = (tide_state_t*)context;
    if (state->mode == TIDE_SCREEN_SETTING_HOUR || state->mode == TIDE_SCREEN_SETTING_MIN) {
        // Not strictly needed because it will be done upon re-entering the
        // watch face. But let’s leave a clean state.
        _move_next_high_tide(state, _get_current_unix_time());
    }
}
