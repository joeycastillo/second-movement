/*
 * MIT License
 *
 * Copyright (c) 2025 Ola Kåre Risa
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
#include "watch_utility.h"
#include "binary_face.h"
#include "watch_common_display.h"

// 2.4 volts seems to offer adequate warning of a low battery condition?
// refined based on user reports and personal observations; may need further adjustment.
#ifndef CLOCK_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD
#define CLOCK_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD 2400
#endif

static void clock_indicate(watch_indicator_t indicator, bool on) {
    if (on) {
        watch_set_indicator(indicator);
    } else {
        watch_clear_indicator(indicator);
    }
}

static void clock_indicate_alarm() {
    clock_indicate(WATCH_INDICATOR_SIGNAL, movement_alarm_enabled());
}

static void clock_indicate_time_signal(binary_state_t *state) {
    clock_indicate(WATCH_INDICATOR_BELL, state->time_signal_enabled);
}

static void clock_indicate_24h() {
    clock_indicate(WATCH_INDICATOR_24H, !!movement_clock_mode_24h());
}

static bool clock_is_pm(watch_date_time_t date_time) {
    return date_time.unit.hour >= 12;
}

static void clock_indicate_pm(watch_date_time_t date_time) {
    if (movement_clock_mode_24h()) { return; }
    clock_indicate(WATCH_INDICATOR_PM, clock_is_pm(date_time));
}

static void clock_indicate_low_available_power(binary_state_t *state) {
    // Set the low battery indicator if battery power is low
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        // interlocking arrows imply "exchange" the battery.
        clock_indicate(WATCH_INDICATOR_ARROWS, state->battery_low);
    } else {
        // LAP indicator on classic LCD is an adequate fallback.
        clock_indicate(WATCH_INDICATOR_LAP, state->battery_low);
    }
}

static watch_date_time_t clock_24h_to_12h(watch_date_time_t date_time) {
    date_time.unit.hour %= 12;

    if (date_time.unit.hour == 0) {
        date_time.unit.hour = 12;
    }

    return date_time;
}

static void clock_check_battery_periodically(binary_state_t *state, watch_date_time_t date_time) {
    // check the battery voltage once a day
    if (date_time.unit.day == state->last_battery_check) { return; }

    state->last_battery_check = date_time.unit.day;

    uint16_t voltage = watch_get_vcc_voltage();

    state->battery_low = voltage < CLOCK_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD;

    clock_indicate_low_available_power(state);
}

static void clock_toggle_time_signal(binary_state_t *state) {
    state->time_signal_enabled = !state->time_signal_enabled;
    clock_indicate_time_signal(state);
}

static const uint8_t HOUR_16_POS = 0;
static const uint8_t HOUR_8_POS = 1;
static const uint8_t HOUR_4_POS = 10;
static const uint8_t HOUR_2_POS = 2;
static const uint8_t HOUR_1_POS = 3;
static const uint8_t MINUTE_32_POS = 4;
static const uint8_t MINUTE_16_POS = 5;
static const uint8_t MINUTE_8_POS = 6;
static const uint8_t MINUTE_4_POS = 7;
static const uint8_t MINUTE_2_POS = 8;
static const uint8_t MINUTE_1_POS = 9;


static bool displayBinaryMinutes(watch_date_time_t current, watch_date_time_t previous) {
    if ((current.reg >> 6) == (previous.reg >> 6)) {
        // everything before seconds is the same, don't waste cycles setting those segments.
        return true;

    } else if ((current.reg >> 12) == (previous.reg >> 12)) {
        // everything before minutes is the same.

        watch_display_character(current.unit.minute / 32 + '0', MINUTE_32_POS);
        watch_display_character(current.unit.minute / 16 % 2 + '0', MINUTE_16_POS);
        watch_display_character(current.unit.minute / 8 % 2 + '0', MINUTE_8_POS);
        watch_display_character(current.unit.minute / 4 % 2 + '0', MINUTE_4_POS);
        watch_display_character(current.unit.minute / 2 % 2 + '0', MINUTE_2_POS);
        watch_display_character(current.unit.minute % 2 + '0', MINUTE_1_POS);

        return true;

    } else {
        // other stuff changed; let's do it all.
        return false;
    }
}

static uint8_t decToHex(uint8_t decMod16) {
    if (decMod16<10) {
        return decMod16 + '0';
    } else if (decMod16 == 11 || decMod16 == 13) {
        return decMod16 % 10 + 'a';
    } else {
        return decMod16 % 10 + 'A';
    }
}

static void displayHoursClassicalDisplay(uint32_t hour) {
    if (hour / 16 > 0) {
        watch_display_character('1', HOUR_2_POS);
    } else {
        watch_display_character(' ', HOUR_2_POS);
    }
    watch_display_character(decToHex(hour % 16), HOUR_1_POS);
}


static void displayBinaryAll(watch_date_time_t current, bool mode24h) {
    
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC) {
        displayHoursClassicalDisplay(current.unit.hour);
    } else {
        if (current.unit.hour / 16 > 0) {
            watch_display_character('1', HOUR_16_POS);
        } else {
            watch_display_character(' ', HOUR_16_POS);            
        }
        watch_display_character(current.unit.hour / 8 % 2 + '0', HOUR_8_POS);
        watch_display_character(current.unit.hour / 4 % 2 + '0', HOUR_4_POS);
        watch_display_character(current.unit.hour / 2 % 2 + '0', HOUR_2_POS);
        watch_display_character(current.unit.hour % 2 + '0', HOUR_1_POS);
    }
    watch_display_character(current.unit.minute / 32 + '0', MINUTE_32_POS);
    watch_display_character(current.unit.minute / 16 % 2 + '0', MINUTE_16_POS);
    watch_display_character(current.unit.minute / 8 % 2 + '0', MINUTE_8_POS);
    watch_display_character(current.unit.minute / 4 % 2 + '0', MINUTE_4_POS);
    watch_display_character(current.unit.minute / 2 % 2 + '0', MINUTE_2_POS);
    watch_display_character(current.unit.minute % 2 + '0', MINUTE_1_POS);
}

static void displayBinary(binary_state_t *state, watch_date_time_t current) {
    if (!displayBinaryMinutes(current, state->date_time.previous)) {
        if (movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_12H) {
            clock_indicate_pm(current);
            current = clock_24h_to_12h(current);
            displayBinaryAll(current, false);
        } else {
            displayBinaryAll(current, true);
        }
    }
}



void binary_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;

    if (*context_ptr != NULL) {
        free(*context_ptr);
        *context_ptr = NULL;
    }

    *context_ptr = malloc(sizeof(binary_state_t));
    if (*context_ptr == NULL) {
        return;
    }

    memset(*context_ptr, 0, sizeof(binary_state_t));
    binary_state_t *state = (binary_state_t *) *context_ptr;

    state->time_signal_enabled = false;
    state->watch_face_index = watch_face_index;
}


void binary_face_activate(void *context) {
    binary_state_t *state = (binary_state_t *)context;
    clock_indicate_24h();
    clock_indicate_time_signal(state);
    clock_indicate_alarm();
    
    // this ensures that none of the timestamp fields will match, so we can re-render them all.
    state->date_time.previous.reg = 0xFFFFFFFF;
}

bool binary_face_loop(movement_event_t event, void *context) {
    binary_state_t *state = (binary_state_t *)context;
    watch_date_time_t current;

    switch (event.event_type) {
        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
        case EVENT_ACTIVATE:
            current = movement_get_local_date_time();

            displayBinary(state, current);
            clock_check_battery_periodically(state, current);

            state->date_time.previous = current;

            break;
        case EVENT_ALARM_LONG_PRESS:
            clock_toggle_time_signal(state);
            break;
        case EVENT_BACKGROUND_TASK:
            // uncomment this line to snap back to the clock face when the hour signal sounds:
            // movement_move_to_face(state->watch_face_index);
            movement_play_signal();
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void binary_face_resign(void *context) {
    (void) context;
}

