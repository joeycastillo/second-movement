/* SPDX-License-Identifier: MIT */

/*
 * MIT License
 *
 * Copyright © 2021-2023 Joey Castillo <joeycastillo@utexas.edu> <jose.castillo@gmail.com>
 * Copyright © 2022 David Keck <davidskeck@users.noreply.github.com>
 * Copyright © 2022 TheOnePerson <a.nebinger@web.de>
 * Copyright © 2023 Jeremy O'Brien <neutral@fastmail.com>
 * Copyright © 2023 Mikhail Svarichevsky <3@14.by>
 * Copyright © 2023 Wesley Aptekar-Cassels <me@wesleyac.com>
 * Copyright © 2024 Matheus Afonso Martins Moreira <matheus.a.m.moreira@gmail.com>
 * Copyright © 2026 Chris Gillings <git@ned-ludd.com>
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
#include "new_earth_time_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_common_display.h"

// 2.4 volts seems to offer adequate warning of a low battery condition?
// refined based on user reports and personal observations; may need further adjustment.
#ifndef NEW_EARTH_TIME_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD
#define NEW_EARTH_TIME_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD 2400
#endif

static void new_earth_time_indicate(watch_indicator_t indicator, bool on) {
    if (on) {
        watch_set_indicator(indicator);
    } else {
        watch_clear_indicator(indicator);
    }
}

static void new_earth_time_indicate_low_available_power(new_earth_time_state_t *state) {
    // Set the low battery indicator if battery power is low
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        // interlocking arrows imply "exchange" the battery.
        new_earth_time_indicate(WATCH_INDICATOR_ARROWS, state->battery_low);
    } else {
        // LAP indicator on classic LCD is an adequate fallback.
        new_earth_time_indicate(WATCH_INDICATOR_LAP, state->battery_low);
    }
}

static void new_earth_time_check_battery_periodically(new_earth_time_state_t *state, watch_date_time_t date_time) {
    // check the battery voltage once a day
    if (date_time.unit.day == state->last_battery_check) { return; }

    state->last_battery_check = date_time.unit.day;

    uint16_t voltage = watch_get_vcc_voltage();

    state->battery_low = voltage < NEW_EARTH_TIME_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD;

    new_earth_time_indicate_low_available_power(state);
}

static void new_earth_time_display_all(watch_date_time_t date_time) {
    char buf[8 + 1];
    int net_degrees;
    int net_seconds;
    float net_time;
    net_time = date_time.unit.hour * 15 + date_time.unit.minute * 0.25 + date_time.unit.second * 0.0041667;
    net_degrees = (int)net_time;
    net_seconds = (int)((net_time-net_degrees) * 60);

    snprintf(
        buf,
        sizeof(buf),
        "%2d%3d#%02d",
        date_time.unit.day,
        net_degrees,
        net_seconds
    );

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "NET", "NT");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    watch_display_text(WATCH_POSITION_BOTTOM, buf + 2);
}

static void new_earth_time_display_new_earth_time(watch_date_time_t current) {
    new_earth_time_display_all(current);
}

void new_earth_time_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(new_earth_time_state_t));
        new_earth_time_state_t *state = (new_earth_time_state_t *) *context_ptr;
        state->time_signal_enabled = false;
        state->watch_face_index = watch_face_index;
    }
}

void new_earth_time_face_activate(void *context) {
    new_earth_time_state_t *state = (new_earth_time_state_t *) context;

    // new_earth_time_indicate_time_signal(false);

    // watch_set_colon();

    // this ensures that none of the timestamp fields will match, so we can re-render them all.
    state->date_time.previous.reg = 0xFFFFFFFF;
}

bool new_earth_time_face_loop(movement_event_t event, void *context) {
    new_earth_time_state_t *state = (new_earth_time_state_t *) context;
    watch_date_time_t current;

    switch (event.event_type) {
        case EVENT_LOW_ENERGY_UPDATE:
            break;
        case EVENT_TICK:
        case EVENT_ACTIVATE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "NET", "NT");
            current = movement_get_utc_date_time();

            new_earth_time_display_new_earth_time(current);

            new_earth_time_check_battery_periodically(state, current);

            state->date_time.previous = current;

            break;
        case EVENT_ALARM_LONG_PRESS:
            break;
        case EVENT_BACKGROUND_TASK:
            // uncomment this line to snap back to the new_earth_time face when the hour signal sounds:
            // movement_move_to_face(state->watch_face_index);
            // movement_play_signal();
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void new_earth_time_face_resign(void *context) {
    (void) context;
}

movement_watch_face_advisory_t new_earth_time_face_advise(void *context) {
    movement_watch_face_advisory_t retval = { 0 };
    new_earth_time_state_t *state = (new_earth_time_state_t *) context;

    if (state->time_signal_enabled) {
        watch_date_time_t date_time = movement_get_utc_date_time();
        retval.wants_background_task = date_time.unit.minute == 0;
    }

    return retval;
}
