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
#include "set_time_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "zones.h"

#define SET_TIME_FACE_NUM_SETTINGS (7)
const char set_time_face_titles[SET_TIME_FACE_NUM_SETTINGS][3] = {"HR", "M1", "SE", "YR", "MO", "DA", "  "};

typedef enum {
    SET_TIME_HOUR = 0,
    SET_TIME_MIN,
    SET_TIME_SEC,
    SET_TIME_YEAR,
    SET_TIME_MONTH,
    SET_TIME_DAY,
    SET_TIME_TZ
} set_time_pages;

static bool _quick_ticks_running;
static int32_t current_offset;

static void _handle_alarm_button(watch_date_time_t date_time, uint8_t current_page) {
    // handles short or long pressing of the alarm button

    switch (current_page) {
        case SET_TIME_HOUR:
            date_time.unit.hour = (date_time.unit.hour + 1) % 24;
            break;
        case SET_TIME_MIN:
            date_time.unit.minute = (date_time.unit.minute + 1) % 60;
            break;
        case SET_TIME_SEC:
            date_time.unit.second = 0;
            break;
        case SET_TIME_YEAR:
            date_time.unit.year = ((date_time.unit.year % 60) + 1);
            break;
        case SET_TIME_MONTH:
            date_time.unit.month = (date_time.unit.month % 12) + 1;
            break;
        case SET_TIME_DAY:
            date_time.unit.day = date_time.unit.day + 1;
            break;
        case SET_TIME_TZ:
            movement_set_timezone_index(movement_get_timezone_index() + 1);
            if (movement_get_timezone_index() >= NUM_ZONE_NAMES) movement_set_timezone_index(0);
            current_offset = movement_get_current_timezone_offset_for_zone(movement_get_timezone_index());
            break;
    }
    if (date_time.unit.day > days_in_month(date_time.unit.month, date_time.unit.year + WATCH_RTC_REFERENCE_YEAR))
        date_time.unit.day = 1;
    movement_set_local_date_time(date_time);
}

static void _abort_quick_ticks() {
    if (_quick_ticks_running) {
        _quick_ticks_running = false;
        movement_request_tick_frequency(4);
    }
}

void set_time_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) *context_ptr = malloc(sizeof(uint8_t));
}

void set_time_face_activate(void *context) {
    *((uint8_t *)context) = 0;
    movement_request_tick_frequency(4);
    _quick_ticks_running = false;
    current_offset = movement_get_current_timezone_offset();
}

bool set_time_face_loop(movement_event_t event, void *context) {
    uint8_t current_page = *((uint8_t *)context);
    watch_date_time_t date_time = movement_get_local_date_time();

    switch (event.event_type) {
        case EVENT_TICK:
            if (_quick_ticks_running) {
                if (HAL_GPIO_BTN_ALARM_read()) _handle_alarm_button(date_time, current_page);
                else _abort_quick_ticks();
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (current_page != SET_TIME_SEC) {
                _quick_ticks_running = true;
                movement_request_tick_frequency(8);
            }
            break;
        case EVENT_ALARM_LONG_UP:
            _abort_quick_ticks();
            break;
        case EVENT_MODE_BUTTON_UP:
            _abort_quick_ticks();
            movement_move_to_next_face();
            return false;
        case EVENT_LIGHT_BUTTON_DOWN:
            current_page = (current_page + 1) % SET_TIME_FACE_NUM_SETTINGS;
            if (current_page == SET_TIME_TZ && movement_update_dst_offset_cache())
                current_offset = movement_get_current_timezone_offset();
            *((uint8_t *)context) = current_page;
            break;
        case EVENT_ALARM_BUTTON_UP:
            _abort_quick_ticks();
            _handle_alarm_button(date_time, current_page);
            break;
        case EVENT_TIMEOUT:
            _abort_quick_ticks();
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    char buf[11];
    watch_display_text(WATCH_POSITION_TOP_LEFT, (char *) set_time_face_titles[current_page]);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    if (current_page < SET_TIME_YEAR) {
        watch_set_colon();
        if (movement_clock_mode_24h()) {
            watch_set_indicator(WATCH_INDICATOR_24H);
            sprintf(buf, "%2d%02d%02d", date_time.unit.hour, date_time.unit.minute, date_time.unit.second);
        } else {
            sprintf(buf, "%2d%02d%02d", (date_time.unit.hour % 12) ? (date_time.unit.hour % 12) : 12, date_time.unit.minute, date_time.unit.second);
            if (date_time.unit.hour < 12) watch_clear_indicator(WATCH_INDICATOR_PM);
            else watch_set_indicator(WATCH_INDICATOR_PM);
        }
    } else if (current_page < SET_TIME_TZ) {
        watch_clear_colon();
        watch_clear_indicator(WATCH_INDICATOR_24H);
        watch_clear_indicator(WATCH_INDICATOR_PM);
        sprintf(buf, "%2d%02d%02d", date_time.unit.year + 20, date_time.unit.month, date_time.unit.day);
    } else {
        watch_display_text(WATCH_POSITION_TOP_RIGHT, " Z");
        if (current_offset < 0) watch_display_text(WATCH_POSITION_TOP_LEFT, "- ");
        else watch_display_text(WATCH_POSITION_TOP_LEFT, "* ");
        if (event.subsecond % 2) {
            uint8_t hours = abs(current_offset) / 3600;
            uint8_t minutes = (abs(current_offset) % 3600) / 60;

            sprintf(buf, "%2d%02d  ", hours % 100, minutes % 100);
            watch_set_colon();
        } else {
            sprintf(buf, "%s", (char *) (3 + zone_names + 11 * movement_get_timezone_index()));
            watch_clear_colon();
        }
    }

    watch_display_text(WATCH_POSITION_BOTTOM, buf);

    // blink up the parameter we're setting
    if (event.subsecond % 2 && !_quick_ticks_running) {
        switch (current_page) {
            case SET_TIME_HOUR:
            case SET_TIME_YEAR:
                watch_display_text(WATCH_POSITION_HOURS, "  ");
                break;
            case SET_TIME_MIN:
            case SET_TIME_MONTH:
                watch_display_text(WATCH_POSITION_MINUTES, "  ");
                break;
            case SET_TIME_SEC:
            case SET_TIME_DAY:
                watch_display_text(WATCH_POSITION_SECONDS, "  ");
                break;
        }
    }

    return true;
}

void set_time_face_resign(void *context) {
    (void) context;
    watch_set_led_off();
    movement_store_settings();
    movement_update_dst_offset_cache();
}
