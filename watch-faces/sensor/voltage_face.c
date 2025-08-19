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

#define VOLTAGE_LOGGING_CYC (VOLTAGE_NUM_DATA_POINTS + 1)

static void _voltage_face_update_display(void) {
    float voltage = (float)watch_get_vcc_voltage() / 1000.0;

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "BAT", "BA");
    watch_display_float_with_best_effort(voltage, " V");
}

static void _voltage_face_blink_display(bool update_now) {
    watch_date_time_t date_time = movement_get_local_date_time();
    if (date_time.unit.second % 5 == 0 || update_now) {
        _voltage_face_update_display();
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    }
    else if (date_time.unit.second % 5 == 4) {
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
        return;
    }
}

static void _voltage_face_log_data(voltage_face_state_t *logger_state) {
    watch_date_time_t date_time = movement_get_local_date_time();
    size_t pos = logger_state->data_points % VOLTAGE_NUM_DATA_POINTS;

    logger_state->data[pos].timestamp.reg = date_time.reg;
    float voltage = (float)watch_get_vcc_voltage() / 1000.0;
    logger_state->data[pos].voltage = voltage;
    logger_state->data_points++;
}

static void _voltage_face_logging_update_display(voltage_face_state_t *logger_state, bool clock_mode_24h, bool from_btn) {
    int8_t pos = (logger_state->data_points - 1 - logger_state->display_index) % VOLTAGE_NUM_DATA_POINTS;
    char buf[7];

    watch_clear_indicator(WATCH_INDICATOR_24H);
    watch_clear_indicator(WATCH_INDICATOR_PM);
    watch_clear_colon();

    if (logger_state->display_index == VOLTAGE_NUM_DATA_POINTS){
        _voltage_face_blink_display(from_btn);
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "BAT  ", "BA  ");
        return;
    }
    watch_clear_indicator(WATCH_INDICATOR_SIGNAL);

    if (pos < 0) {
        // no data at this index
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "BAT", "BA");
        watch_clear_decimal_if_available();
        watch_display_text(WATCH_POSITION_BOTTOM, "no dat");
        sprintf(buf, "%2d", logger_state->display_index);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    } else if (logger_state->ts_ticks) {
        // we are displaying the timestamp in response to a button press
        watch_date_time_t date_time = logger_state->data[pos].timestamp;
        watch_set_colon();
        if (clock_mode_24h) {
            watch_set_indicator(WATCH_INDICATOR_24H);
        } else {
            if (date_time.unit.hour > 11) watch_set_indicator(WATCH_INDICATOR_PM);
            date_time.unit.hour %= 12;
            if (date_time.unit.hour == 0) date_time.unit.hour = 12;
        }
        watch_display_text(WATCH_POSITION_TOP_LEFT, "AT");
        sprintf(buf, "%2d", date_time.unit.day);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
        sprintf(buf, "%2d%02d%02d", date_time.unit.hour, date_time.unit.minute, date_time.unit.second);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    } else {
        // we are displaying the voltage
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "BAT", "BA");
        sprintf(buf, "%2d", logger_state->display_index);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
        watch_display_float_with_best_effort(logger_state->data[pos].voltage, " V");
    }
}

void voltage_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(voltage_face_state_t));
        memset(*context_ptr, 0, sizeof(voltage_face_state_t));
    }
}

void voltage_face_activate(void *context) {
    voltage_face_state_t *logger_state = (voltage_face_state_t *)context;
    logger_state->display_index = VOLTAGE_NUM_DATA_POINTS;
    logger_state->ts_ticks = 0;
}

bool voltage_face_loop(movement_event_t event, void *context) {
    voltage_face_state_t *logger_state = (voltage_face_state_t *)context;
    bool displaying_curr_volt = logger_state->display_index == VOLTAGE_NUM_DATA_POINTS;
    switch (event.event_type) {
        case EVENT_LIGHT_LONG_PRESS:
            // light button shows the timestamp, but if you need the light, long press it.
            movement_illuminate_led();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_BUTTON_UP:
            logger_state->display_index = (logger_state->display_index + VOLTAGE_LOGGING_CYC - 1) % VOLTAGE_LOGGING_CYC;
            logger_state->ts_ticks = 0;
            _voltage_face_logging_update_display(logger_state, movement_clock_mode_24h(), true);
            break;
        case EVENT_ALARM_BUTTON_UP:
            logger_state->display_index = (logger_state->display_index + 1) % VOLTAGE_LOGGING_CYC;
            logger_state->ts_ticks = 0;
            _voltage_face_logging_update_display(logger_state, movement_clock_mode_24h(), true);
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (displaying_curr_volt) break;
            else logger_state->ts_ticks = 2;
            _voltage_face_logging_update_display(logger_state, movement_clock_mode_24h(), true);
            break;
        case EVENT_ACTIVATE:
            if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();
            _voltage_face_logging_update_display(logger_state, movement_clock_mode_24h(), true);
            break;
        case EVENT_TICK:
            if(displaying_curr_volt) _voltage_face_update_display();
            else if (logger_state->ts_ticks && --logger_state->ts_ticks == 0) {
                _voltage_face_logging_update_display(logger_state, movement_clock_mode_24h(), false);
            }
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            // clear seconds area (on classic LCD) and start tick animation if necessary
            if (!watch_sleep_animation_is_running()) {
                watch_display_text_with_fallback(WATCH_POSITION_SECONDS, " V", "  ");
                watch_start_sleep_animation(1000);
            }
            // update once an hour
            if (date_time.unit.minute == 0 && displaying_curr_volt) {
                watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
                _voltage_face_update_display();
                watch_display_text_with_fallback(WATCH_POSITION_SECONDS, " V", "  ");
            }
            break;
        case EVENT_BACKGROUND_TASK:
            _voltage_face_log_data(logger_state);
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

movement_watch_face_advisory_t voltage_face_advise(void *context) {
    (void) context;
    movement_watch_face_advisory_t retval = { 0 };

    // this will get called at the top of each minute, so all we check is if we're at the top of the hour as well.
    // if we are, we ask for a background task.
    retval.wants_background_task = movement_get_local_date_time().unit.minute == 0;

    return retval;
}
