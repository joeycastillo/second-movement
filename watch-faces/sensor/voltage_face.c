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
#include "voltage_face.h"
#include "watch.h"

static void _voltage_face_update_display(void) {
    float voltage = (float)watch_get_vcc_voltage() / 1000.0;

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "BAT", "BA");
    watch_display_float_with_best_effort(voltage, " V");
}

void voltage_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    (void) context_ptr;
}

void voltage_face_activate(void *context) {
    (void) context;
}

bool voltage_face_loop(movement_event_t event, void *context) {
    (void) context;
    watch_date_time_t date_time;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();
            _voltage_face_update_display();
            break;
        case EVENT_TICK:
            date_time = movement_get_local_date_time();
            if (date_time.unit.second % 5 == 4) {
                watch_set_indicator(WATCH_INDICATOR_SIGNAL);
            } else if (date_time.unit.second % 5 == 0) {
                _voltage_face_update_display();
                watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
            }
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            // clear seconds area (on classic LCD) and start tick animation if necessary
            if (!watch_sleep_animation_is_running()) {
                watch_display_text_with_fallback(WATCH_POSITION_SECONDS, " V", "  ");
                watch_start_sleep_animation(1000);
            }
            // update once an hour
            if (date_time.unit.minute == 0) {
                watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
                _voltage_face_update_display();
                watch_display_text_with_fallback(WATCH_POSITION_SECONDS, " V", "  ");
            }
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void voltage_face_resign(void *context) {
    (void) context;
}
