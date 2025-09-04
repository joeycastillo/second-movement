/*
 * MIT License
 *
 * Copyright (c) 2025 Joey Castillo
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
#include "ke_decimal_time_face.h"
#include "watch_utility.h"

static void _display_date(watch_date_time_t date_time) {
    char buf[3];

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, watch_utility_get_long_weekday(date_time), watch_utility_get_weekday(date_time));
    snprintf(buf, sizeof(buf), "%2d", date_time.unit.day);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
}

static void _display_time(ke_decimal_time_state_t *state, watch_date_time_t date_time, bool low_energy) {
    char buf[8];
    uint32_t value = date_time.unit.hour * 3600 + date_time.unit.minute * 60 + date_time.unit.second;

    if (value == state->previous_time) return;

    value = value * 100;
    value = value / 864;
    snprintf(buf, sizeof(buf), "%04ld#o", value);

    // if under 10%, display 0.00 instead of 00.00
    if (value < 1000) buf[0] = ' ';

    // if low energy, truncate at the tens place
    if (low_energy) buf[3] = 0;

    watch_display_text(WATCH_POSITION_BOTTOM, buf);

    state->previous_time = value;
}

void ke_decimal_time_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(ke_decimal_time_state_t));
        memset(*context_ptr, 0, sizeof(ke_decimal_time_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

void ke_decimal_time_face_activate(void *context) {
    ke_decimal_time_state_t *state = (ke_decimal_time_state_t *)context;

    if (watch_sleep_animation_is_running()) {
        watch_stop_sleep_animation();
    }

    // force re-display of date and time in EVENT_ACTIVATE
    state->previous_day = 0xFF;
    state->previous_time = 0xFFFFFFFF;
}

bool ke_decimal_time_face_loop(movement_event_t event, void *context) {
    ke_decimal_time_state_t *state = (ke_decimal_time_state_t *)context;
    watch_date_time_t date_time = movement_get_local_date_time();

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_set_decimal_if_available();
            if (movement_alarm_enabled()) {
                watch_set_indicator(WATCH_INDICATOR_SIGNAL);
            }
            // fall through
        case EVENT_TICK:
            _display_time(state, date_time, false);
            if (state->previous_day != date_time.unit.day) {
                _display_date(date_time);
                state->previous_day = date_time.unit.day;
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // You can use the Light button for your own purposes. Note that by default, Movement will also
            // illuminate the LED in response to EVENT_LIGHT_BUTTON_DOWN; to suppress that behavior, add an
            // empty case for EVENT_LIGHT_BUTTON_DOWN.
            break;
        case EVENT_ALARM_BUTTON_UP:
            // Just in case you have need for another button.
            break;
        case EVENT_TIMEOUT:
            // Your watch face will receive this event after a period of inactivity. If it makes sense to resign,
            // you may uncomment this line to move back to the first watch face in the list:
            // movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            // If you did not resign in EVENT_TIMEOUT, you can use this event to update the display once a minute.
            // Avoid displaying fast-updating values like seconds, since the display won't update again for 60 seconds.
            // You should also consider starting the tick animation, to show the wearer that this is sleep mode:
            if (!watch_sleep_animation_is_running()) {
                watch_start_sleep_animation(500);
                watch_display_text(WATCH_POSITION_SECONDS, "  ");
                watch_display_text(WATCH_POSITION_MINUTES, "  ");
            }
            _display_time(state, date_time, true);
            break;
        default:
            // Movement's default loop handler will step in for any cases you don't handle above:
            // * EVENT_LIGHT_BUTTON_DOWN lights the LED
            // * EVENT_MODE_BUTTON_UP moves to the next watch face in the list
            // * EVENT_MODE_LONG_PRESS returns to the first watch face (or skips to the secondary watch face, if configured)
            // You can override any of these behaviors by adding a case for these events to this switch statement.
            return movement_default_loop_handler(event);
    }

    // return true if the watch can enter standby mode. Generally speaking, you should always return true.
    return true;
}

void ke_decimal_time_face_resign(void *context) {
    (void) context;

    // handle any cleanup before your watch face goes off-screen.
}

