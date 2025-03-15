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
    "CL        K",   // Clock: 12 or 24 hour
    "BT   beep N",   // Mode button: how loud should it beep?
    "TO        U",   // Timeout: how long before we snap back to the clock face?
    "LE        M",   // Low Energy mode: how long before it engages?
    "LT        D",   // Light: duration
    "LT   red  C",   // Light: red component
    "LT   greenC",   // Light: green component
    "LT   blue C",   // Light: blue component (for watches with blue LED)
};

void preferences_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(preferences_state_t));
        preferences_state_t *state = (preferences_state_t *)*context_ptr;
        for (int i = 0; i < PREFERENCES_PAGE_NUM_PREFERENCES; i++) {
            state->setting_enabled[i] = true;
        }
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

void preferences_face_activate(void *context) {
    preferences_state_t *state = (preferences_state_t *)context;
    state->current_page = 0;
    movement_request_tick_frequency(4); // we need to manually blink some pixels
}

bool preferences_face_loop(movement_event_t event, void *context) {
    preferences_state_t *state = (preferences_state_t *)context;
    movement_color_t color; // to use in the switch if we need it

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
                        if (movement_clock_mode_24h()) watch_display_text(WATCH_POSITION_BOTTOM, "24h");
                        else watch_display_text(WATCH_POSITION_BOTTOM, "12h");
                        break;
                    case PREFERENCES_PAGE_BUTTON_SOUND:
                        if (movement_button_should_sound()) {
                            watch_display_text(WATCH_POSITION_TOP_RIGHT, " Y");
                        } else {
                            watch_display_text(WATCH_POSITION_TOP_RIGHT, " N");
                        }
                        break;
                    case PREFERENCES_PAGE_TIMEOUT:
                        switch (movement_get_fast_tick_timeout()) {
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
                        switch (movement_get_low_energy_timeout()) {
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
                        if (movement_get_backlight_dwell() == 0) {
                            watch_display_text(WATCH_POSITION_BOTTOM, "instnt");
                        } else if (movement_get_backlight_dwell() == 0b111) {
                            watch_display_text(WATCH_POSITION_BOTTOM, "no LEd");
                        } else {
                            sprintf(buf, " %1d SeC", (movement_get_backlight_dwell() * 2 - 1) % 10);
                            watch_display_text(WATCH_POSITION_BOTTOM, buf);
                        }
                        break;
                    case PREFERENCES_PAGE_LED_RED:
                        color = movement_backlight_color();
                        sprintf(buf, "%2d", color.red);
                        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
                        break;
                    case PREFERENCES_PAGE_LED_GREEN:
                        color = movement_backlight_color();
                        sprintf(buf, "%2d", color.green);
                        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
                        break;
                    case PREFERENCES_PAGE_LED_BLUE:
                        color = movement_backlight_color();
                        sprintf(buf, "%2d", color.blue);
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
                    movement_set_clock_mode_24h(((movement_clock_mode_24h() + 1) % MOVEMENT_NUM_CLOCK_MODES));
                    break;
                case PREFERENCES_PAGE_BUTTON_SOUND:
                    movement_set_button_should_sound(!movement_button_should_sound());
                    break;
                case PREFERENCES_PAGE_TIMEOUT:
                    movement_set_fast_tick_timeout((movement_get_fast_tick_timeout() + 1));
                    break;
                case PREFERENCES_PAGE_LOW_ENERGY:
                    movement_set_low_energy_timeout((movement_get_low_energy_timeout() + 1));
                    break;
                case PREFERENCES_PAGE_LED_DURATION:
                    movement_set_backlight_dwell(movement_get_backlight_dwell() + 1);
                    if (movement_get_backlight_dwell() > 3) {
                        // set all bits to disable the LED
                        movement_set_backlight_dwell(0b111);
                    }
                    break;
                case PREFERENCES_PAGE_LED_RED:
                    color = movement_backlight_color();
                    color.red++;
                    movement_set_backlight_color(color);
                    break;
                case PREFERENCES_PAGE_LED_GREEN:
                    color = movement_backlight_color();
                    color.green++;
                    movement_set_backlight_color(color);
                    break;
                case PREFERENCES_PAGE_LED_BLUE:
                    color = movement_backlight_color();
                    color.blue++;
                    movement_set_backlight_color(color);
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
            return movement_default_loop_handler(event);
    }

    if (state->current_page == PREFERENCES_PAGE_LED_RED ||
        state->current_page == PREFERENCES_PAGE_LED_GREEN ||
        state->current_page == PREFERENCES_PAGE_LED_BLUE) {
        movement_color_t color = movement_backlight_color();
        // this bitwise math turns #000 into #000000, #111 into #111111, etc.
        movement_force_led_on(color.red | color.red << 4,
                              color.green | color.green << 4,
                              color.blue | color.blue << 4);
        return false;
    } else {
        movement_force_led_off();
        return true;
    }
}

void preferences_face_resign(void *context) {
    (void) context;
    movement_force_led_off();
    movement_store_settings();
}
