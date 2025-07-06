/*
 * MIT License
 *
 * Copyright (c) 2025 Ruben Nic
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
#include <math.h>
#include "close_enough_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_common_display.h"

// 2.4 volts seems to offer adequate warning of a low battery condition?
// refined based on user reports and personal observations; may need further adjustment.
#ifndef CLOCK_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD
#define CLOCK_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD 2400
#endif

static const char *words[12] = {
    "  ",
    " 5",
    "10",
    "15",
    "20",
    "25",
    "30",
    "35",
    "40",
    "45",
    "50",
    "55",
};

static const char *past_word = " P";
static const char *to_word = " 2";
static const char *oclock_word = "OC";

// sets when in the five minute period we switch
// from "X past HH" to  "X to HH+1"
static const int hour_switch_index = 8;

static void clock_stop_tick_tock_animation(void) {
    if (watch_sleep_animation_is_running()) {
        watch_stop_sleep_animation();
        watch_stop_blink();
    }
}

static void clock_indicate(watch_indicator_t indicator, bool on) {
    if (on) {
        watch_set_indicator(indicator);
    } else {
        watch_clear_indicator(indicator);
    }
}

void close_enough_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(close_enough_state_t));
        memset(*context_ptr, 0, sizeof(close_enough_state_t));
    }
}

void close_enough_face_activate(void *context) {
    close_enough_state_t *state = (close_enough_state_t *)context;

    clock_stop_tick_tock_animation();

    clock_indicate(WATCH_INDICATOR_BELL, movement_alarm_enabled());
    clock_indicate(WATCH_INDICATOR_24H, !!movement_clock_mode_24h());

    // this ensures that none of the five_minute_periods will match, so we always rerender when the face activates
    state->prev_five_minute_period = -1;
    state->prev_min_checked = -1;
}

static void clock_check_battery_periodically(close_enough_state_t *state) {
    // If the battery is  low, skip the check. We have already indicated it.
    if (state->battery_low) {
        return;
    }

    watch_date_time_t date_time = movement_get_local_date_time();

    if (date_time.unit.day == state->last_battery_check) { return; }

    state->last_battery_check = date_time.unit.day;

    uint16_t voltage = watch_get_vcc_voltage();

    state->battery_low = voltage < CLOCK_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD;

    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        // interlocking arrows imply "exchange" the battery.
        clock_indicate(WATCH_INDICATOR_ARROWS, state->battery_low);
    } else {
        // LAP indicator on classic LCD is an adequate fallback.
        clock_indicate(WATCH_INDICATOR_LAP, state->battery_low);
    }
}

bool close_enough_face_loop(movement_event_t event, void *context) {
    close_enough_state_t *state = (close_enough_state_t *)context;
    watch_date_time_t date_time;
    bool show_next_hour = false;
    int prev_five_minute_period;
    int prev_min_checked;
    int close_enough_hour;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
            date_time = movement_get_local_date_time();
            prev_five_minute_period = state->prev_five_minute_period;
            prev_min_checked = state->prev_min_checked;

            // check the battery voltage once a day...
            clock_check_battery_periodically(state);

            // same minute, skip update
            if (date_time.unit.minute == prev_min_checked) {
                break;
            } else {
                state->prev_min_checked = date_time.unit.minute;
            }

            int five_minute_period = (date_time.unit.minute / 5) % 12;

            // Move to next five minute period if we are above 50% through the current five minute period (we are only checking the remainder)
            if (fmodf(date_time.unit.minute / 5.0f, 1.0f) > 0.5f) {
                // If we are on the last 5 interval and moving to the next period we need to display the next hour
                if (five_minute_period == 11) {
                    show_next_hour = true;
                }

                five_minute_period = (five_minute_period + 1) % 12;
            }

            // same five_minute_period, skip update
            if (five_minute_period == prev_five_minute_period) {
                break;
            }

            close_enough_hour = date_time.unit.hour;

            // move from "MM P HH" to "MM 2 HH+1"
            if (five_minute_period >= hour_switch_index || show_next_hour) {
                close_enough_hour = (close_enough_hour + 1) % 24;
                show_next_hour = true;
            }

            if (movement_clock_mode_24h() != MOVEMENT_CLOCK_MODE_24H) {
                // if we are at "MM 2 12", don't show the PM indicator
                if (close_enough_hour < 12 || show_next_hour) {
                    watch_clear_indicator(WATCH_INDICATOR_PM);
                } else {
                    watch_set_indicator(WATCH_INDICATOR_PM);
                }

                close_enough_hour %= 12;
                if (close_enough_hour == 0) {
                    close_enough_hour = 12;
                }
            }

            char first_word[3];
            char second_word[3];
            char third_word[3];
            if (five_minute_period == 0) { // "  HH OC",
                sprintf(first_word, "  ");
                sprintf(second_word, "%2d", close_enough_hour);
                strncpy(third_word, oclock_word, 3);
            } else { // "MM P HH" or "MM 2 HH+1"
                int words_length = sizeof(words) / sizeof(words[0]);

                strncpy(
                    first_word,
                    show_next_hour ?
                        words[words_length - five_minute_period] :
                        words[five_minute_period],
                    3
                );
                strncpy(
                    second_word,
                    show_next_hour ? to_word : past_word,
                    3
                );
                sprintf(third_word, "%2d", close_enough_hour);
            }

            watch_display_text_with_fallback(
                WATCH_POSITION_TOP_LEFT,
                watch_utility_get_long_weekday(date_time), watch_utility_get_weekday(date_time)
            );

            char day_buf[2 + 1];
            sprintf(day_buf, "%2d", date_time.unit.day);
            watch_display_text(
                WATCH_POSITION_TOP_RIGHT,
                day_buf
            );

            char words_buf[6 + 1];
            sprintf(words_buf,
                "%s%s%s",
                first_word,
                second_word,
                third_word
            );
            watch_display_text(
                WATCH_POSITION_BOTTOM,
                words_buf
            );

            state->prev_five_minute_period = five_minute_period;
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void close_enough_face_resign(void *context) {
    (void) context;
}

