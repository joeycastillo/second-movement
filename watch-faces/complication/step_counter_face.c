/*
 * MIT License
 *
 * Copyright (c) 2025 David Volovskiy
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
#include "step_counter_face.h"

#define STEP_COUNTER_DISPLAY_NO_STEP_DAYS false
#define STEP_COUNTER_MINUTES_NO_ACTIVITY_RESIGN 5
#define STEP_COUNTER_MINUTES_SEC_BEFORE_START 2
#define STEP_COUNTER_MAX_STEPS_DISPLAY 999999

// distant future for background task: January 1, 2083
static const watch_date_time_t distant_future = {
    .unit = {0, 0, 0, 1, 1, 63}
};

static const uint16_t sec_inactivity_allow_sleep = STEP_COUNTER_MINUTES_NO_ACTIVITY_RESIGN * 60;

static uint32_t get_step_count(void) {
    uint32_t step_count = movement_get_step_count();
    if (step_count > STEP_COUNTER_MAX_STEPS_DISPLAY) return STEP_COUNTER_MAX_STEPS_DISPLAY;
    return step_count;
}

static uint16_t display_step_count_now(bool sensor_seen) {
    char buf[10];
    uint32_t step_count = get_step_count();
    if (!sensor_seen) {
        watch_display_text(WATCH_POSITION_BOTTOM, "NO SNS");
    } else {
        sprintf(buf, "%6lu", step_count);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    }
    return step_count;
}

static void _step_counter_face_log_data(step_counter_state_t *logger_state, uint32_t step_count) {
    watch_date_time_t date_time = movement_get_local_date_time();
    size_t pos = logger_state->data_points % STEP_COUNTER_NUM_DATA_POINTS;

    logger_state->data[pos].day = date_time.unit.day;
    logger_state->data[pos].step_count = step_count;
    logger_state->data_points++;
}

static void _step_counter_face_logging_update_display(step_counter_state_t *logger_state) {
    if (logger_state->display_index == logger_state->data_points) {
        logger_state->step_count_prev = display_step_count_now(logger_state->sensor_seen);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  "); // To clear the date on the classic display
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "STEP ", "SC");
        return;
    }
    char buf[9];
    int8_t pos = logger_state->data_points - logger_state->display_index - 1;
    watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    // we are displaying the step_counter
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "STP", "SC");
    sprintf(buf, "%2d", logger_state->data[pos].day);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    sprintf(buf, "%6lu", logger_state->data[pos].step_count);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

// The idea is that we can sleep only if we're not looking at the current step count or see enough time of inactivity
static void allow_sleeping(bool sleeping_is_wanted, step_counter_state_t *logger_state) {
    if (sleeping_is_wanted == logger_state->can_sleep) return;
    if (sleeping_is_wanted) {
        logger_state->can_sleep = true;
        movement_cancel_background_task();
    } else {
        logger_state->can_sleep = false;
        movement_schedule_background_task(distant_future);
    }
}

static void enable_sensor(step_counter_state_t *logger_state) {
    logger_state->sensor_seen = movement_enable_step_count_multiple_attempts(3, false);
    if (logger_state->sensor_seen) {
        movement_set_step_count_keep_off(false);
        movement_set_step_count_keep_on(true);
        logger_state->can_sleep = false;
        movement_schedule_background_task(distant_future);
    }
    _step_counter_face_logging_update_display(logger_state);
}

void step_counter_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(step_counter_state_t));
        memset(*context_ptr, 0, sizeof(step_counter_state_t));
        step_counter_state_t *logger_state = (step_counter_state_t *)*context_ptr;
        logger_state->sensor_seen = true;
    }
}

void step_counter_face_activate(void *context) {
    (void) context;
}

bool step_counter_face_loop(movement_event_t event, void *context) {
    step_counter_state_t *logger_state = (step_counter_state_t *)context;
    bool displaying_curr_step_count = logger_state->display_index == logger_state->data_points;
    uint32_t step_count;
    switch (event.event_type) {
        case EVENT_LIGHT_LONG_PRESS:
            // light button shows the timestamp, but if you need the light, long press it.
            movement_illuminate_led();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_BUTTON_UP:
            logger_state->display_index = (logger_state->display_index + logger_state->data_points) % (logger_state->data_points + 1);
            _step_counter_face_logging_update_display(logger_state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            logger_state->display_index = (logger_state->display_index + 1) % (logger_state->data_points + 1);
            _step_counter_face_logging_update_display(logger_state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (displaying_curr_step_count) {
                movement_reset_step_count();
                logger_state->step_count_prev = display_step_count_now(logger_state->sensor_seen);
            } else {
                logger_state->display_index = logger_state->data_points;
                _step_counter_face_logging_update_display(logger_state);
            }
            break;
        case EVENT_ACTIVATE:
            if (!movement_has_lis2dw()) {  // Skip this face if no accelerometer was seen on start-up
                movement_move_to_next_face();
                return false;
            }
            logger_state->display_index = logger_state->data_points;
            logger_state->sec_inactivity = 1;
            if (!movement_step_count_is_enabled()) {
                // There can be a delay in showing the screen when turning on the step counter,
                // So if it's off, display stale step counts that'll then get updated
                _step_counter_face_logging_update_display(logger_state);
                logger_state->sec_before_starting = STEP_COUNTER_MINUTES_SEC_BEFORE_START;
            }
            else {
                enable_sensor(logger_state);
                logger_state->sec_before_starting = 0;
            }
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            if(displaying_curr_step_count) {
                watch_display_text(WATCH_POSITION_BOTTOM, "SLEEP ");
            }
            break;
        case EVENT_TICK:
            // This makes it so if the we're just scrolling through faces, we don't immedietly turn
            // on the step counter only to turn it back off.
            if (logger_state->sec_before_starting > 0) {
                logger_state->sec_before_starting--;
                if (logger_state->sec_before_starting == 0) {
                    enable_sensor(logger_state);
                }
                break;
            }
            if(displaying_curr_step_count) {
                step_count = get_step_count();
                if (step_count != logger_state->step_count_prev) {
                    allow_sleeping(false, logger_state);
                    logger_state->sec_inactivity = 1;
                    logger_state->step_count_prev = display_step_count_now(logger_state->sensor_seen);
                } else {
                    if (logger_state->sec_inactivity >= sec_inactivity_allow_sleep) {
                        allow_sleeping(true, logger_state);
                    } else {
                        logger_state->sec_inactivity++;
                    }
                }
            } else {
                allow_sleeping(true, logger_state);
            }
            break;
        case EVENT_BACKGROUND_TASK:
            step_count = get_step_count();
            if (STEP_COUNTER_DISPLAY_NO_STEP_DAYS || step_count != 0) {
                _step_counter_face_log_data(logger_state, step_count);
                logger_state->display_index = (logger_state->display_index + 1) % (logger_state->data_points + 1);
            }
            movement_reset_step_count();
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void step_counter_face_resign(void *context) {
    (void) context;
    movement_set_step_count_keep_on(false);
    movement_cancel_background_task();
}

movement_watch_face_advisory_t step_counter_face_advise(void *context) {
    (void) context;
    movement_watch_face_advisory_t retval = { 0 };
    watch_date_time_t date_time = movement_get_local_date_time();
    // To reset the step count at midnight (or 11:59 as a hack to retain the current day's data)
    retval.wants_background_task = (date_time.unit.hour == 23 && date_time.unit.minute == 59);
    return retval;
}