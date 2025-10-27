/*
 * MIT License
 *
 * Copyright (c) 2025 David Volovskiy
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
 *
 * Sunrise/sunset calculations are public domain code by Paul Schlyter, December 1992
 *
 */

#include <stdlib.h>
#include <string.h>
#include "location_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_common_display.h"
#include "filesystem.h"

#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define FREQ_FAST 8
#define FREQ 2

static uint8_t _ts_ticks = 0;
static const uint8_t _location_count = sizeof(locationLongLatPresets) / sizeof(locationLongLatPresets[0]);

static void persist_location_to_filesystem(movement_location_t new_location) {
    movement_location_t maybe_location = {0};

    filesystem_read_file("location.u32", (char *) &maybe_location.reg, sizeof(movement_location_t));
    if (new_location.reg != maybe_location.reg) {
        filesystem_write_file("location.u32", (char *) &new_location.reg, sizeof(movement_location_t));
    }
}

static movement_location_t load_location_from_filesystem() {
    movement_location_t location = {0};

    filesystem_read_file("location.u32", (char *) &location.reg, sizeof(movement_location_t));

    return location;
}

static uint8_t city_idx_of_curr_location(int16_t latitude, int16_t longitude){
    for (uint8_t i = 0; i < _location_count; i++) {
        if (locationLongLatPresets[i].latitude == latitude && locationLongLatPresets[i].longitude == longitude) {
            return i;
        }
    }
    printf("custmom %d\n", _location_count);
    return _location_count;
}

static void display_city(location_state_t *state) {
    char buf[7];
    printf("display_city %d\n", state->city_idx);
    printf("%s\n", locationLongLatPresets[state->city_idx].name);
    if (state->city_idx >= _location_count) {
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
        watch_display_text(WATCH_POSITION_BOTTOM, "CUSTOM");
    } else {
        sprintf(buf, " %d", locationLongLatPresets[state->city_idx].region);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
        sprintf(buf, " %.5s", locationLongLatPresets[state->city_idx].name);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    }
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "LOC", "L ");
}

static int16_t _location_face_latlon_from_struct(location_lat_lon_settings_t val) {
    int16_t retval = (val.sign ? -1 : 1) *
                        (
                            val.hundreds * 10000 +
                            val.tens * 1000 +
                            val.ones * 100 +
                            val.tenths * 10 +
                            val.hundredths
                        );
    return retval;
}

static location_lat_lon_settings_t _location_face_struct_from_latlon(int16_t val) {
    location_lat_lon_settings_t retval;

    retval.sign = val < 0;
    val = abs(val);
    retval.hundredths = val % 10;
    val /= 10;
    retval.tenths = val % 10;
    val /= 10;
    retval.ones = val % 10;
    val /= 10;
    retval.tens = val % 10;
    val /= 10;
    retval.hundreds = val % 10;

    return retval;
}

static void _location_face_update_location_register(location_state_t *state) {
    movement_location_t movement_location;
    int16_t lat = _location_face_latlon_from_struct(state->working_latitude);
    int16_t lon = _location_face_latlon_from_struct(state->working_longitude);
    state->city_idx = city_idx_of_curr_location(lat, lon);
    movement_location.bit.latitude = lat;
    movement_location.bit.longitude = lon;
    persist_location_to_filesystem(movement_location);
}

static void _location_face_update_settings_display(movement_event_t event, location_state_t *state) {
    char buf[12];
    printf("%d %d\n", state->page, event.subsecond);

    watch_clear_display();

    switch (state->page) {
        case LOCATION_FACE_CITIES:
        case LOCATION_FACE_PAGES_COUNT:
            return;
        case LOCATION_FACE_SETTING_LAT:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "LAT", "LA");
            if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
                watch_set_decimal_if_available();
                watch_display_character('0' + state->working_latitude.tens, 4);
                watch_display_character('0' + state->working_latitude.ones, 5);
                watch_display_character('0' + state->working_latitude.tenths, 6);
                watch_display_character('0' + state->working_latitude.hundredths, 7);
                watch_display_character('#', 8);
                if (state->working_latitude.sign) watch_display_character('S', 9);
                else watch_display_character('N', 9);

                if (event.subsecond % 2) {
                    printf("hi %d\r\n", event.subsecond);
                    watch_display_character(' ', 4 + state->active_digit);
                    // for degrees N or S, also flash the last character
                    if (state->active_digit == 4) watch_display_character(' ', 9);
                }
            } else {
                sprintf(buf, "%c %04d", state->working_latitude.sign ? '-' : '+', abs(_location_face_latlon_from_struct(state->working_latitude)));
                if (event.subsecond % 2) buf[state->active_digit] = ' ';
                watch_display_text(WATCH_POSITION_BOTTOM, buf);
            }
            break;
        case LOCATION_FACE_SETTING_LONG:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "LON", "LO");
            if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
                watch_set_decimal_if_available();
                // Handle leading 1 for longitudes >99
                if (state->working_longitude.hundreds == 1) watch_set_pixel(0, 22);
                watch_display_character('0' + state->working_longitude.tens, 4);
                watch_display_character('0' + state->working_longitude.ones, 5);
                watch_display_character('0' + state->working_longitude.tenths, 6);
                watch_display_character('0' + state->working_longitude.hundredths, 7);
                watch_display_character('#', 8);
                if (state->working_longitude.sign) watch_display_character('W', 9);
                else watch_display_character('E', 9);
                if (event.subsecond % 2) {
                    watch_display_character(' ', 4 + state->active_digit);
                    // for tens place, also flash leading 1 if present
                    if (state->active_digit == 0) watch_clear_pixel(0, 22);
                    // for degrees E or W, also flash the last character
                    if (state->active_digit == 4) watch_display_character(' ', 9);
                }
            } else {
                sprintf(buf, "%c%05d", state->working_longitude.sign ? '-' : '+', abs(_location_face_latlon_from_struct(state->working_longitude)));
                if (event.subsecond % 2) buf[state->active_digit] = ' ';
                watch_display_text(WATCH_POSITION_BOTTOM, buf);
            }
            break;
    }
}

static void _location_face_advance_digit(location_state_t *state) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        switch (state->page) {
            case LOCATION_FACE_CITIES:
            case LOCATION_FACE_PAGES_COUNT:
                return;
            case LOCATION_FACE_SETTING_LAT:
                switch (state->active_digit) {
                    case 0: // tens
                        state->working_latitude.tens = (state->working_latitude.tens + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_latitude)) > 9000) {
                            // prevent latitude from going over ±90.
                            // TODO: perform these checks when advancing the digit?
                            state->working_latitude.ones = 0;
                            state->working_latitude.tenths = 0;
                            state->working_latitude.hundredths = 0;
                        }
                        break;
                    case 1:
                        state->working_latitude.ones = (state->working_latitude.ones + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_latitude)) > 9000) state->working_latitude.ones = 0;
                        break;
                    case 2:
                        state->working_latitude.tenths = (state->working_latitude.tenths + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_latitude)) > 9000) state->working_latitude.tenths = 0;
                        break;
                    case 3:
                        state->working_latitude.hundredths = (state->working_latitude.hundredths + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_latitude)) > 9000) state->working_latitude.hundredths = 0;
                        break;
                    case 4:
                        state->working_latitude.sign++;
                        break;
                }
                break;
            case LOCATION_FACE_SETTING_LONG:
                switch (state->active_digit) {
                    case 0:
                        // Increase tens and handle carry-over to hundreds
                        state->working_longitude.tens++;
                        if (state->working_longitude.tens >= 10) {
                            state->working_longitude.tens = 0;
                            state->working_longitude.hundreds++;
                        }

                        // Reset if we've gone over ±180
                        if (abs(_location_face_latlon_from_struct(state->working_longitude)) > 18000) {
                            state->working_longitude.hundreds = 0;
                            state->working_longitude.tens = 0;
                            state->working_longitude.ones = 0;
                            state->working_longitude.tenths = 0;
                            state->working_longitude.hundredths = 0;
                        }
                        break;
                    case 1:
                        state->working_longitude.ones = (state->working_longitude.ones + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_longitude)) > 18000) state->working_longitude.ones = 0;
                        break;
                    case 2:
                        state->working_longitude.tenths = (state->working_longitude.tenths + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_longitude)) > 18000) state->working_longitude.tenths = 0;
                        break;
                    case 3:
                        state->working_longitude.hundredths = (state->working_longitude.hundredths + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_longitude)) > 18000) state->working_longitude.hundredths = 0;
                        break;
                    case 4:
                        state->working_longitude.sign++;
                        break;
                }
                break;
        }
    } else {
        switch (state->page) {
            case LOCATION_FACE_CITIES:
            case LOCATION_FACE_PAGES_COUNT:
                return;
            case LOCATION_FACE_SETTING_LAT:
                switch (state->active_digit) {
                    case 0:
                        state->working_latitude.sign++;
                        break;
                    case 1:
                        // we skip this digit
                        break;
                    case 2:
                        state->working_latitude.tens = (state->working_latitude.tens + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_latitude)) > 9000) {
                            // prevent latitude from going over ±90.
                            // TODO: perform these checks when advancing the digit?
                            state->working_latitude.ones = 0;
                            state->working_latitude.tenths = 0;
                            state->working_latitude.hundredths = 0;
                        }
                        break;
                    case 3:
                        state->working_latitude.ones = (state->working_latitude.ones + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_latitude)) > 9000) state->working_latitude.ones = 0;
                        break;
                    case 4:
                        state->working_latitude.tenths = (state->working_latitude.tenths + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_latitude)) > 9000) state->working_latitude.tenths = 0;
                        break;
                    case 5:
                        state->working_latitude.hundredths = (state->working_latitude.hundredths + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_latitude)) > 9000) state->working_latitude.hundredths = 0;
                        break;
                }
                break;
            case LOCATION_FACE_SETTING_LONG:
                switch (state->active_digit) {
                    case 0:
                        state->working_longitude.sign++;
                        break;
                    case 1:
                        state->working_longitude.hundreds = (state->working_longitude.hundreds + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_longitude)) > 18000) {
                            // prevent longitude from going over ±180
                            state->working_longitude.tens = 8;
                            state->working_longitude.ones = 0;
                            state->working_longitude.tenths = 0;
                            state->working_longitude.hundredths = 0;
                        }
                        break;
                    case 2:
                        state->working_longitude.tens = (state->working_longitude.tens + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_longitude)) > 18000) state->working_longitude.tens = 0;
                        break;
                    case 3:
                        state->working_longitude.ones = (state->working_longitude.ones + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_longitude)) > 18000) state->working_longitude.ones = 0;
                        break;
                    case 4:
                        state->working_longitude.tenths = (state->working_longitude.tenths + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_longitude)) > 18000) state->working_longitude.tenths = 0;
                        break;
                    case 5:
                        state->working_longitude.hundredths = (state->working_longitude.hundredths + 1) % 10;
                        if (abs(_location_face_latlon_from_struct(state->working_longitude)) > 18000) state->working_longitude.hundredths = 0;
                        break;
                }
                break;
        }
    }
}

static void _location_face_move_forward(location_state_t *state) {
    state->city_idx = (state->city_idx + 1) % (_location_count + 1);
    display_city(state);
}

static void _location_face_move_backwards(location_state_t *state) {
    state->city_idx = (_location_count + state->city_idx) % (_location_count + 1);
    printf("_location_face_move_backwards %d\n", state->city_idx);
    display_city(state);
}

static void _location_face_start_quick_cyc(location_state_t *state) {
    state->quick_ticks_running = true;
    movement_request_tick_frequency(FREQ_FAST);
}

static void _location_face_stop_quick_cyc(location_state_t *state) {
    state->quick_ticks_running = false;
    movement_request_tick_frequency(FREQ);
}


static bool _location_face_update_long_lat_display(movement_event_t event, location_state_t *state) {
    switch (event.event_type) {
        case EVENT_LOW_ENERGY_UPDATE:
        case EVENT_TICK:
            printf("_location_face_update_long_lat_display\n");
            _location_face_update_settings_display(event, state);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            state->active_digit++;
            if (state->active_digit == 1) state->active_digit++; // max latitude is +- 90, no hundreds place
            if (state->active_digit > 5) {
                state->active_digit = 0;
                state->page = (state->page + 1) % LOCATION_FACE_PAGES_COUNT;
                _location_face_update_location_register(state);
                if (state->page == LOCATION_FACE_CITIES) {
                    display_city(state);
                }
                break;
            }
            _location_face_update_settings_display(event, state);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_ALARM_BUTTON_UP:
            _location_face_advance_digit(state);
            _location_face_update_settings_display(event, state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->active_digit = 0;
            state->page = LOCATION_FACE_CITIES;
            _location_face_update_location_register(state);
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

static bool _location_face_update_choose_city(movement_event_t event, location_state_t *state) {
    switch (event.event_type) {
        case EVENT_LOW_ENERGY_UPDATE:
        case EVENT_LIGHT_BUTTON_DOWN:
        case EVENT_ALARM_BUTTON_DOWN:
            break;
        case EVENT_TICK:
            if (state->quick_ticks_running) {
                if (HAL_GPIO_BTN_ALARM_read()) {
                    _location_face_move_forward(state);
                } else {
                    _location_face_stop_quick_cyc(state);
                }
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            _location_face_move_backwards(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            _location_face_move_forward(state);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            state->active_digit = 0;
            if (state->city_idx < _location_count) {
                    state->working_latitude = _location_face_struct_from_latlon(locationLongLatPresets[state->city_idx].latitude);
                    state->working_longitude = _location_face_struct_from_latlon(locationLongLatPresets[state->city_idx].longitude);
            }
            state->page = LOCATION_FACE_SETTING_LAT;
            break;
        case EVENT_ALARM_LONG_PRESS:
            _location_face_start_quick_cyc(state);
            _location_face_move_forward(state);
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void location_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(location_state_t));
        memset(*context_ptr, 0, sizeof(location_state_t));
    }
}

void location_face_activate(void *context) {
    if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();

#if __EMSCRIPTEN__
    int16_t browser_lat = EM_ASM_INT({
        return lat;
    });
    int16_t browser_lon = EM_ASM_INT({
        return lon;
    });
    if ((watch_get_backup_data(1) == 0) && (browser_lat || browser_lon)) {
        movement_location_t browser_loc;
        browser_loc.bit.latitude = browser_lat;
        browser_loc.bit.longitude = browser_lon;
        watch_store_backup_data(browser_loc.reg, 1);
    }
#endif

    location_state_t *state = (location_state_t *)context;
    movement_location_t movement_location = load_location_from_filesystem();
    state->working_latitude = _location_face_struct_from_latlon(movement_location.bit.latitude);
    state->working_longitude = _location_face_struct_from_latlon(movement_location.bit.longitude);
    state->city_idx = city_idx_of_curr_location(movement_location.bit.latitude, movement_location.bit.longitude);
    state->quick_ticks_running = false;
    display_city(state);
    movement_request_tick_frequency(FREQ);
}

bool location_face_loop(movement_event_t event, void *context) {
    location_state_t *state = (location_state_t *)context;
    bool retval;
    switch (state->page)
    {
        case LOCATION_FACE_SETTING_LAT:
        case LOCATION_FACE_SETTING_LONG:
            retval = _location_face_update_long_lat_display(event, state);
            break;
        case LOCATION_FACE_CITIES:
        default:
            retval = _location_face_update_choose_city(event, state);
            break;
    }

    return retval;
}

void location_face_resign(void *context) {
    location_state_t *state = (location_state_t *)context;
    state->page = 0;
    state->active_digit = 0;
}
