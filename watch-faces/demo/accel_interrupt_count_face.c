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
#include "watch.h"

// hacky hacky!
uint32_t *ptr_to_count = 0;

void accel_interrupt_handler(void);
void accel_interrupt_handler(void) {
    (*ptr_to_count)++;
}

void sleep_interrupt_handler(void);
void sleep_interrupt_handler(void) {
    if (HAL_GPIO_A3_read()) {
        movement_force_led_on(255, 0, 0);
    } else {
        movement_force_led_on(0, 255, 0);
    }
}

static void _accel_interrupt_count_face_update_display(accel_interrupt_count_state_t *state) {
    char buf[7];

    if (state->running) {
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    }

    // "AC"celerometer "IN"terrupts
    watch_display_text(WATCH_POSITION_TOP_LEFT, "AC");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "1N");
    snprintf(buf, 7, "%6ld", state->count);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    printf("%s\n", buf);
}

static void _accel_interrupt_count_face_configure_threshold(uint8_t threshold) {
    lis2dw_enable_sleep();
    lis2dw_configure_wakeup_threshold(threshold);
    // lis2dw_configure_int1(LIS2DW_CTRL4_INT1_WU);
    lis2dw_configure_int2(LIS2DW_CTRL5_INT2_SLEEP_CHG);
    lis2dw_enable_interrupts();
}

void accel_interrupt_count_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(accel_interrupt_count_state_t));
        memset(*context_ptr, 0, sizeof(accel_interrupt_count_state_t));
        ptr_to_count = &((accel_interrupt_count_state_t *)*context_ptr)->count;
        watch_enable_i2c();
        lis2dw_begin();
        lis2dw_set_mode(LIS2DW_MODE_LOW_POWER);
        lis2dw_set_low_power_mode(LIS2DW_LP_MODE_1);    // lowest power mode
        lis2dw_set_low_noise_mode(true);                // only marginally raises power consumption
        lis2dw_enable_sleep();                          // sleep at 1.6Hz, wake to 12.5Hz?
        lis2dw_set_range(LIS2DW_CTRL6_VAL_RANGE_2G);    // data sheet recommends 2G range
        lis2dw_set_data_rate(LIS2DW_DATA_RATE_LOWEST);  // 1.6Hz in low power mode

        // threshold is 1/64th of full scale, so for a FS of ±2G this is 1.5G
        ((accel_interrupt_count_state_t *)*context_ptr)->threshold = 24;
        _accel_interrupt_count_face_configure_threshold(((accel_interrupt_count_state_t *)*context_ptr)->threshold);
    }
}

void accel_interrupt_count_face_activate(void *context) {
    accel_interrupt_count_state_t *state = (accel_interrupt_count_state_t *)context;

    // never in settings mode at the start
    state->is_setting = false;

    // force LE interval to never sleep
    movement_set_low_energy_timeout(0);

    state->running = true;
    watch_register_interrupt_callback(HAL_GPIO_A4_pin(), accel_interrupt_handler, INTERRUPT_TRIGGER_RISING);
    watch_register_interrupt_callback(HAL_GPIO_A3_pin(), sleep_interrupt_handler, INTERRUPT_TRIGGER_BOTH);
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
                    watch_display_float_with_best_effort(state->new_threshold * 0.0625, " G");
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
            case EVENT_LIGHT_BUTTON_DOWN:
                movement_illuminate_led();

                // if stopped, reset the count
                if (!state->running) {
                    state->count = 0;
                }
                _accel_interrupt_count_face_update_display(state);
                break;
            case EVENT_ALARM_BUTTON_UP:
                if (state->running) {
                    state->running = false;
                    watch_register_interrupt_callback(HAL_GPIO_A4_pin(), NULL, INTERRUPT_TRIGGER_RISING);
                } else {
                    state->running = true;
                    watch_register_interrupt_callback(HAL_GPIO_A4_pin(), accel_interrupt_handler, INTERRUPT_TRIGGER_RISING);
                }
                _accel_interrupt_count_face_update_display(state);
                break;
            case EVENT_ACTIVATE:
            case EVENT_TICK:
                _accel_interrupt_count_face_update_display(state);
                break;
            case EVENT_ALARM_LONG_PRESS:
                if (!state->running) {
                    state->new_threshold = state->threshold;
                    state->is_setting = true;
                }
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
