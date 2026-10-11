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
#include "watch.h"

static void _status_display(void) {
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "ACCEL", "AC");
    watch_set_indicator(WATCH_INDICATOR_SIGNAL);

    // the motion pin reads HIGH when still at rest, LOW when active
    if (HAL_GPIO_A4_read()) watch_display_text(WATCH_POSITION_BOTTOM, "Still ");
    else watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Active", " ACtiv");
}

static bool _settings_blink(uint8_t subsecond) {
    if (subsecond % 2 == 0) {
        watch_display_text(WATCH_POSITION_BOTTOM, "      ");
        watch_clear_decimal_if_available();
        return true;
    }
    return false;
}

static void _settings_display(uint8_t subsecond) {
    watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "WAKE ", "WA");
    if (_settings_blink(subsecond)) return;
    watch_clear_decimal_if_available();
    watch_display_text(WATCH_POSITION_BOTTOM, movement_get_wake_on_motion() ? "  on  " : "  off ");
}

void accelerometer_status_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(accel_interrupt_count_state_t));
        memset(*context_ptr, 0, sizeof(accel_interrupt_count_state_t));

        /* Set up accelerometer and enable background sensing */
        movement_set_accelerometer_background_rate(LIS2DW_DATA_RATE_LOWEST);
    }
}

void accelerometer_status_face_activate(void *context) {
    accel_interrupt_count_state_t *state = (accel_interrupt_count_state_t *)context;

    state->is_setting = false;

    movement_request_tick_frequency(4);
}

bool accelerometer_status_face_loop(movement_event_t event, void *context) {
    accel_interrupt_count_state_t *state = (accel_interrupt_count_state_t *)context;

    if (state->is_setting) {
        switch (event.event_type) {
            case EVENT_ACTIVATE:
            case EVENT_TICK:
                _settings_display(event.subsecond);
                break;
            case EVENT_ALARM_BUTTON_UP:
                movement_set_wake_on_motion(!movement_get_wake_on_motion());
                _settings_display(event.subsecond);
                break;
            case EVENT_MODE_BUTTON_UP:
                state->is_setting = false;
                watch_clear_decimal_if_available();
                _status_display();
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
                _status_display();
                break;
            case EVENT_LOW_ENERGY_UPDATE:
                // start tick animation if necessary
                if (!watch_sleep_animation_is_running()) watch_start_sleep_animation(1000);
                _status_display();
                // on classic LCD, clear seconds since they interfere with the sleep animation.
                if (watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC) watch_display_text(WATCH_POSITION_SECONDS, "  ");
                break;
            case EVENT_LIGHT_LONG_PRESS:
                state->is_setting = true;
                _settings_display(event.subsecond);
                break;
            default:
                movement_default_loop_handler(event);
                break;
        }
    }

    return true;
}

void accelerometer_status_face_resign(void *context) {
    (void)context;
}
