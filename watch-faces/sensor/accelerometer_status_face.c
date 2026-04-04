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
#include "accelerometer_status_face.h"
#include "lis2dw.h"
#include "tc.h"
#include "watch.h"

static void _accelerometer_status_face_update_display(accel_interrupt_count_state_t *state) {
    (void) state;

    // Accelerometer title
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "ACCEL", "AC");

    // sensing is live!
    watch_set_indicator(WATCH_INDICATOR_SIGNAL);

    // Sleep/active state
    if (HAL_GPIO_A4_read()) watch_display_text(WATCH_POSITION_BOTTOM, "Still ");
    else watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Active", " ACtiv");
}

void accelerometer_status_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(accel_interrupt_count_state_t));
        memset(*context_ptr, 0, sizeof(accel_interrupt_count_state_t));
    }
}

void accelerometer_status_face_activate(void *context) {
    accel_interrupt_count_state_t *state = (accel_interrupt_count_state_t *)context;

    // Force the Step Counter to be turned off 
    // immedietly in case it's on so this face can use the LIS2DW
    if (movement_has_lis2dw() && movement_step_count_is_enabled()) {
        movement_set_step_count_keep_off(true);
        movement_disable_step_count(true);
    }

    // never in settings mode at the start
    state->is_setting = false;

    // update more quickly to catch changes, also to blink setting
    movement_request_tick_frequency(4);

    // fetch current threshold from accelerometer
    state->threshold = movement_get_accelerometer_motion_threshold();
}

bool accelerometer_status_face_loop(movement_event_t event, void *context) {
    accel_interrupt_count_state_t *state = (accel_interrupt_count_state_t *)context;

    if (state->is_setting) {
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
        switch (event.event_type) {
            case EVENT_ALARM_BUTTON_DOWN:
                state->new_threshold = (state->new_threshold + 1) % 64;
                // fall through
            case EVENT_TICK:
                {
                    char buf[11];
                    if (event.subsecond % 2) {
                        watch_display_text(WATCH_POSITION_BOTTOM, "      ");
                        watch_clear_decimal_if_available();
                    } else {
                        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
                        watch_display_text_with_fallback(WATCH_POSITION_TOP, "WAKth", "TH");
                        watch_display_float_with_best_effort(state->new_threshold * 0.03125, " G");
                        printf("%s\n", buf);
                    }
                }
                break;
            case EVENT_LIGHT_BUTTON_DOWN:
                movement_set_accelerometer_motion_threshold(state->new_threshold);
                state->threshold = state->new_threshold;
                watch_clear_decimal_if_available();
                state->is_setting = false;
                break;
            case EVENT_TIMEOUT:
                movement_move_to_face(0);
                break;
            default:
                movement_default_loop_handler(event);
                break;
        }
    } else {
        switch (event.event_type) {
            case EVENT_ACTIVATE:
            case EVENT_TICK:
                _accelerometer_status_face_update_display(state);
                break;
            case EVENT_LOW_ENERGY_UPDATE:
                // start tick animation if necessary
                if (!watch_sleep_animation_is_running()) watch_start_sleep_animation(1000);
                // update the display
                _accelerometer_status_face_update_display(state);
                // on classic LCD, clear seconds since they interfere with the sleep animation.
                if (watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC) watch_display_text(WATCH_POSITION_SECONDS, "  ");
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

void accelerometer_status_face_resign(void *context) {
    (void)context;
    movement_set_step_count_keep_off(false);
}
