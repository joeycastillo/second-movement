/*
 * MIT License
 *
 * Copyright (c) 2022 Josh Berson, building on Wesley Ellis’ countdown_face.c
 * Copyright (c) 2025 Joey Castillo
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
#include "alarm_face.h"
#include "watch.h"
#include "watch_utility.h"

//
// Private
//

static void _alarm_face_display_alarm_time(alarm_face_state_t *state) {
    uint8_t hour = state->hour;

    if ( movement_clock_mode_24h() )
        watch_set_indicator(WATCH_INDICATOR_24H);
    else {
        if ( hour >= 12 ) watch_set_indicator(WATCH_INDICATOR_PM);
        else watch_clear_indicator(WATCH_INDICATOR_PM);
        hour = hour % 12 ? hour % 12 : 12;
    }

    static char lcdbuf[7];
    sprintf(lcdbuf, "%2d%02d  ", hour, state->minute);

    watch_display_text(WATCH_POSITION_BOTTOM, lcdbuf);
}

static inline void button_beep() {
    // play a beep as confirmation for a button press (if applicable)
    if (movement_button_should_sound()) watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
}

//
// Exported
//

void alarm_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(alarm_face_state_t));
        alarm_face_state_t *state = (alarm_face_state_t *)*context_ptr;
        memset(*context_ptr, 0, sizeof(alarm_face_state_t));

        // default to an 8:00 AM alarm time.
        state->hour = 8;
    }
}

void alarm_face_activate(void *context) {
    alarm_face_state_t *state = (alarm_face_state_t *)context;
    state->setting_mode = ALARM_FACE_SETTING_MODE_NONE;
}
void alarm_face_resign(void *context) {
    (void) context;
}

bool alarm_face_loop(movement_event_t event, void *context) {
    alarm_face_state_t *state = (alarm_face_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "ALM", "AL");
            if (state->alarm_is_on) watch_set_indicator(WATCH_INDICATOR_SIGNAL);
            watch_set_colon();
            _alarm_face_display_alarm_time(state);
            break;
        case EVENT_TICK:
            // No action needed for tick events in normal mode; we displayed our stuff in EVENT_ACTIVATE.
            if (state->setting_mode == ALARM_FACE_SETTING_MODE_NONE)
                break;

            // but in settings mode, we need to blink up the parameter we're setting.
            _alarm_face_display_alarm_time(state);
            if (event.subsecond % 2 == 0) 
                watch_display_text((state->setting_mode == ALARM_FACE_SETTING_MODE_SETTING_HOUR) ? WATCH_POSITION_HOURS : WATCH_POSITION_MINUTES, "  ");
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            switch (state->setting_mode) {
                case ALARM_FACE_SETTING_MODE_NONE:
                    // If we're not in a setting mode, turn on the LED like normal.
                    movement_illuminate_led();
                    break;
                case ALARM_FACE_SETTING_MODE_SETTING_HOUR:
                    // If we're setting the hour, advance to minute set mode.
                    state->setting_mode = ALARM_FACE_SETTING_MODE_SETTING_MINUTE;
                    break;
                case ALARM_FACE_SETTING_MODE_SETTING_MINUTE:
                    // If we're setting the minute, advance back to normal mode and cancel fast tick.
                    state->setting_mode = ALARM_FACE_SETTING_MODE_NONE;
                    movement_request_tick_frequency(1);
                    // beep to confirm setting.
                    button_beep();
                    // also turn the alarm on since they just set it.
                    state->alarm_is_on = 1;
                    movement_set_alarm_enabled(true);
                    watch_set_indicator(WATCH_INDICATOR_SIGNAL);
                    _alarm_face_display_alarm_time(state);
                    break;
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->setting_mode == ALARM_FACE_SETTING_MODE_NONE) {
                // in normal mode, toggle alarm on/off.
                state->alarm_is_on ^= 1;
                if ( state->alarm_is_on ) {
                    watch_set_indicator(WATCH_INDICATOR_SIGNAL);
                    movement_set_alarm_enabled(true);
                } else {
                    watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
                    movement_set_alarm_enabled(false);
                }
            }
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            switch (state->setting_mode) {
                case ALARM_FACE_SETTING_MODE_NONE:
                    // nothing to do here, alarm toggle is handled in EVENT_ALARM_BUTTON_UP.
                    break;
                case ALARM_FACE_SETTING_MODE_SETTING_HOUR:
                    // increment hour, wrap around to 0 at 23.
                    state->hour = (state->hour + 1) % 24;
                    break;
                case ALARM_FACE_SETTING_MODE_SETTING_MINUTE:
                    // increment minute, wrap around to 0 at 59.
                    state->minute = (state->minute + 1) % 60;
                    break;
            }
            _alarm_face_display_alarm_time(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (state->setting_mode == ALARM_FACE_SETTING_MODE_NONE) {
                // long press in normal mode: move to hour setting mode, request fast tick.
                state->setting_mode = ALARM_FACE_SETTING_MODE_SETTING_HOUR;
                movement_request_tick_frequency(4);
                button_beep();
            }
            break;
        case EVENT_BACKGROUND_TASK:
            movement_play_alarm();
                // 2022-07-23: Thx @joeycastillo for the dedicated “alarm” signal
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

movement_watch_face_advisory_t alarm_face_advise(void *context) {
    alarm_face_state_t *state = (alarm_face_state_t *)context;
    movement_watch_face_advisory_t retval = { 0 };

    if ( state->alarm_is_on ) {
        watch_date_time_t now = movement_get_local_date_time();
        retval.wants_background_task = (state->hour==now.unit.hour && state->minute==now.unit.minute);
        // We’re at the mercy of the advise handler
        // In Safari, the emulator triggers at the ›end‹ of the minute
        // Converting to Unix timestamps and taking a difference between now and wake
        // is not an easy win — because the timestamp for wake has to rely on now
        // for its date. So first we’d have to see if the TOD of wake is after that
        // of now. If it is, take tomorrow’s date, calculating month and year rollover
        // if need be.
    }

    return retval;
}
