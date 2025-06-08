/*
 * MIT License
 *
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
#include "temperature_display_face.h"
#include "watch.h"

static bool skip = false;

static void _temperature_display_face_update_display(bool in_fahrenheit) {
    float temperature_c = movement_get_temperature();
    if (in_fahrenheit) {
        watch_display_float_with_best_effort(temperature_c * 1.8 + 32.0, "#F");
    } else {
        watch_display_float_with_best_effort(temperature_c, "#C");
    }
}

void temperature_display_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    (void) context_ptr;
    // if temperature is invalid, we don't have a temperature sensor which means we shouldn't be here.
    if (movement_get_temperature() == 0xFFFFFFFF) skip = true;
}

void temperature_display_face_activate(void *context) {
    (void) context;
}

bool temperature_display_face_loop(movement_event_t event, void *context) {
    (void) context;
    watch_date_time_t date_time = watch_rtc_get_date_time();
    switch (event.event_type) {
        case EVENT_ALARM_LONG_PRESS:
            movement_set_use_imperial_units(!movement_use_imperial_units());
            _temperature_display_face_update_display(movement_use_imperial_units());
            break;
        case EVENT_ACTIVATE:
            if (skip) {
                movement_move_to_next_face();
                return false;
            }
            if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "TEMP", "TE");
            // force a measurement to be taken immediately.
            date_time.unit.second = 0;
            // fall through
        case EVENT_TICK:
            if (date_time.unit.second % 5 == 4) {
                // Not 100% on this, but I like the idea of using the signal indicator to indicate that we're sensing data.
                // In this case we turn the indicator on a second before the reading is taken, and clear it when we're done.
                // In reality the measurement takes a fraction of a second, but this is just to show something is happening.
                watch_set_indicator(WATCH_INDICATOR_SIGNAL);
            } else if (date_time.unit.second % 5 == 0) {
                _temperature_display_face_update_display(movement_use_imperial_units());
                watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
            }
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            // clear seconds area and start tick animation if necessary
            if (!watch_sleep_animation_is_running()) {
                watch_start_sleep_animation(1000);
            }
            // update every 5 minutes
            if (date_time.unit.minute % 5 == 0) {
                watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
                _temperature_display_face_update_display(movement_use_imperial_units());
            }
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void temperature_display_face_resign(void *context) {
    (void) context;
}
