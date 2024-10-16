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
#include "accel_interrupt_count_face.h"
#include "lis2dw.h"
#include "tc.h"
#include "watch.h"

#ifdef HAS_ACCELEROMETER

static void _accel_interrupt_count_face_update_display(accel_interrupt_count_state_t *state) {
    (void) state;
    char buf[7];

    // "AC"celerometer "IN"terrupts
    watch_display_text(WATCH_POSITION_TOP_LEFT, "AC");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "1N");
    uint16_t count = tc_count16_get_count(2);
    snprintf(buf, 7, "%6d", count);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    printf("%s\n", buf);
}

void accel_interrupt_count_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(accel_interrupt_count_state_t));
        memset(*context_ptr, 0, sizeof(accel_interrupt_count_state_t));
        accel_interrupt_count_state_t *state = (accel_interrupt_count_state_t *)*context_ptr;
        /// TODO: hook up to movement methods for tracking threshold
        state->threshold = 24;
    }
}

void accel_interrupt_count_face_activate(void *context) {
    accel_interrupt_count_state_t *state = (accel_interrupt_count_state_t *)context;

    // never in settings mode at the start
    state->is_setting = false;
}

bool accel_interrupt_count_face_loop(movement_event_t event, void *context) {
    accel_interrupt_count_state_t *state = (accel_interrupt_count_state_t *)context;

    if (state->is_setting) {
        switch (event.event_type) {
            case EVENT_LIGHT_BUTTON_DOWN:
                state->new_threshold = (state->new_threshold + 1) % 64;
                // fall through
            case EVENT_TICK:
                {
                    char buf[11];
                    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "W_THS", "TH");
                    watch_display_float_with_best_effort(state->new_threshold * 0.03125, " G");
                    printf("%s\n", buf);
                }
                break;
            case EVENT_ALARM_BUTTON_UP:
                lis2dw_configure_wakeup_threshold(state->new_threshold);
                state->threshold = state->new_threshold;
                state->is_setting = false;
                break;
            default:
                movement_default_loop_handler(event);
                break;
        }
    } else {
        switch (event.event_type) {
            case EVENT_ALARM_BUTTON_UP:
                // reset the counter
                tc_count16_set_count(2, 0);
                // fall through
            case EVENT_ACTIVATE:
            case EVENT_TICK:
                _accel_interrupt_count_face_update_display(state);
                break;
            case EVENT_ALARM_LONG_PRESS:
                state->new_threshold = state->threshold;
                state->is_setting = true;
                return false;
            default:
                movement_default_loop_handler(event);
                break;
        }
    }

    return true;
}

void accel_interrupt_count_face_resign(void *context) {
    (void) context;
}

movement_watch_face_advisory_t accel_interrupt_count_face_advise(void *context) {
    (void) context;
    movement_watch_face_advisory_t retval = { 0 };

    return retval;
}

#endif // HAS_ACCELEROMETER
