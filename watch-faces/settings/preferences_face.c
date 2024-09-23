/*
 * MIT License
 *
 * Copyright (c) 2022-2024 Joey Castillo
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
#include "preferences_face.h"
#include "watch.h"

const char preferences_face_titles[PREFERENCES_PAGE_NUM_PREFERENCES][11] = {
    "CL        ",   // Clock: 12 or 24 hour
    "BT   beep ",   // Mode button: how loud should it beep?
    "TO        ",   // Timeout: how long before we snap back to the clock face?
    "LE        ",   // Low Energy mode: how long before it engages?
    "LT        ",   // Light: duration
    "LT   red  ",   // Light: red component
    "LT   green",   // Light: green component
    "LT   blue ",   // Light: blue component (for watches with blue LED)
};

void preferences_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(preferences_state_t));
        preferences_state_t *state = (preferences_state_t *)*context_ptr;
        for (int i = 0; i < PREFERENCES_PAGE_NUM_PREFERENCES; i++) {
            state->setting_enabled[i] = true;
        }
#ifdef CLOCK_FACE_24H_ONLY
        state->setting_enabled[PREFERENCES_PAGE_CLOCK_MODE] = false;
#endif
#ifndef WATCH_RED_TCC_CHANNEL
        state->setting_enabled[PREFERENCES_PAGE_LED_RED] = false;
#endif
#ifndef WATCH_GREEN_TCC_CHANNEL
        state->setting_enabled[PREFERENCES_PAGE_LED_GREEN] = false;
#endif
#ifndef WATCH_BLUE_TCC_CHANNEL
        state->setting_enabled[PREFERENCES_PAGE_LED_BLUE] = false;
#endif

    }
}

void preferences_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    preferences_state_t *state = (preferences_state_t *)context;
    state->current_page = 0;
    movement_request_tick_frequency(4); // we need to manually blink some pixels
}

bool preferences_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    preferences_state_t *state = (preferences_state_t *)context;

    switch (event.event_type) {
        case EVENT_TICK:
        case EVENT_ACTIVATE:
            watch_display_text(WATCH_POSITION_FULL, (char *)preferences_face_titles[state->current_page]);
            watch_clear_all_indicators();
            watch_clear_colon();

            // blink active setting on even-numbered quarter-seconds
            if (event.subsecond % 2) {
                char buf[8];
                switch (state->current_page) {
                    case PREFERENCES_PAGE_CLOCK_MODE:
                        if (settings->bit.clock_mode_24h) watch_display_text(WATCH_POSITION_BOTTOM, "24h");
                        else watch_display_text(WATCH_POSITION_BOTTOM, "12h");
                        break;
                    case PREFERENCES_PAGE_BUTTON_SOUND:
                        if (settings->bit.button_should_sound) {
                            watch_display_text(WATCH_POSITION_TOP_RIGHT, " Y");
                        } else {
                            watch_display_text(WATCH_POSITION_TOP_RIGHT, " N");
                        }
                        break;
                    case PREFERENCES_PAGE_TIMEOUT:
                        switch (settings->bit.to_interval) {
                            case 0:
                                watch_display_text(WATCH_POSITION_BOTTOM, "60 SeC");
                                break;
                            case 1:
                                watch_display_text(WATCH_POSITION_BOTTOM, "2 n&in");
                                break;
                            case 2:
                                watch_display_text(WATCH_POSITION_BOTTOM, "5 n&in");
                                break;
                            case 3:
                                watch_display_text(WATCH_POSITION_BOTTOM, "30n&in");
                                break;
                        }
                        break;
                    case PREFERENCES_PAGE_LOW_ENERGY:
                        switch (settings->bit.le_interval) {
                            case 0:
                                watch_display_text(WATCH_POSITION_BOTTOM, " Never");
                                break;
                            case 1:
                                watch_display_text(WATCH_POSITION_BOTTOM, "10n&in");
                                break;
                            case 2:
                                watch_display_text(WATCH_POSITION_BOTTOM, "1 hour");
                                break;
                            case 3:
                                watch_display_text(WATCH_POSITION_BOTTOM, "2 hour");
                                break;
                            case 4:
                                watch_display_text(WATCH_POSITION_BOTTOM, "6 hour");
                                break;
                            case 5:
                                watch_display_text(WATCH_POSITION_BOTTOM, "12 hr");
                                break;
                            case 6:
                                watch_display_text(WATCH_POSITION_BOTTOM, " 1 day");
                                break;
                            case 7:
                                watch_display_text(WATCH_POSITION_BOTTOM, " 7 day");
                                break;
                        }
                        break;
                    case PREFERENCES_PAGE_LED_DURATION:
                        if (settings->bit.led_duration == 0) {
                            watch_display_text(WATCH_POSITION_BOTTOM, "instnt");
                        } else if (settings->bit.led_duration == 0b111) {
                            watch_display_text(WATCH_POSITION_BOTTOM, "no LEd");
                        } else {
                            sprintf(buf, " %1d SeC", settings->bit.led_duration * 2 - 1);
                            watch_display_text(WATCH_POSITION_BOTTOM, buf);
                        }
                        break;
                    case PREFERENCES_PAGE_LED_RED:
                        sprintf(buf, "%2d", settings->bit.led_red_color);
                        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
                        break;
                    case PREFERENCES_PAGE_LED_GREEN:
                        sprintf(buf, "%2d", settings->bit.led_green_color);
                        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
                        break;
                    case PREFERENCES_PAGE_LED_BLUE:
                        sprintf(buf, "%2d", settings->bit.led_blue_color);
                        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
                        break;
                    case PREFERENCES_PAGE_NUM_PREFERENCES:
                        // nothing to do here, just silencing the warning
                        break;
                }
            }
            break;
        case EVENT_MODE_BUTTON_UP:
            movement_force_led_off();
            movement_move_to_next_face();
            return false;
        case EVENT_LIGHT_BUTTON_DOWN:
            state->current_page = (state->current_page + 1) % PREFERENCES_PAGE_NUM_PREFERENCES;
            while (!state->setting_enabled[state->current_page]) {
                state->current_page = (state->current_page + 1) % PREFERENCES_PAGE_NUM_PREFERENCES;
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            switch (state->current_page) {
                case PREFERENCES_PAGE_CLOCK_MODE:
                    settings->bit.clock_mode_24h = !settings->bit.clock_mode_24h;
                    break;
                case PREFERENCES_PAGE_BUTTON_SOUND:
                    settings->bit.button_should_sound = !settings->bit.button_should_sound;
                    break;
                case PREFERENCES_PAGE_TIMEOUT:
                    settings->bit.to_interval = settings->bit.to_interval + 1;
                    break;
                case PREFERENCES_PAGE_LOW_ENERGY:
                    settings->bit.le_interval = settings->bit.le_interval + 1;
                    break;
                case PREFERENCES_PAGE_LED_DURATION:
                    settings->bit.led_duration = settings->bit.led_duration + 1;
                    if (settings->bit.led_duration > 3) {
                        // set all bits to disable the LED
                        settings->bit.led_duration = 0b111;
                    }
                    break;
                case PREFERENCES_PAGE_LED_RED:
                    settings->bit.led_red_color = settings->bit.led_red_color + 1;
                    break;
                case PREFERENCES_PAGE_LED_GREEN:
                    settings->bit.led_green_color = settings->bit.led_green_color + 1;
                    break;
                case PREFERENCES_PAGE_LED_BLUE:
                    settings->bit.led_blue_color = settings->bit.led_blue_color + 1;
                    break;
                case PREFERENCES_PAGE_NUM_PREFERENCES:
                    // nothing to do here, just silencing the warning
                    break;
            }
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event, settings);
    }

    if (state->current_page == PREFERENCES_PAGE_LED_RED ||
        state->current_page == PREFERENCES_PAGE_LED_GREEN ||
        state->current_page == PREFERENCES_PAGE_LED_BLUE) {
        movement_force_led_on(settings->bit.led_red_color | settings->bit.led_red_color << 4,
                              settings->bit.led_green_color | settings->bit.led_green_color << 4,
                              settings->bit.led_blue_color | settings->bit.led_blue_color << 4);
        return false;
    } else {
        movement_force_led_off();
        return true;
    }
}

void preferences_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    movement_force_led_off();
    watch_store_backup_data(settings->reg, 0);
}
