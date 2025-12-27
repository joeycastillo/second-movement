/*
 * MIT License
 *
 * Copyright (c) 2025 Giorgio Ciacchella
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
#include "watch_radio_alarm_face.h"

//
// Private
//

static void _watch_radio_alarm_face_display_alarm_time(watch_radio_alarm_face_state_t *state) {
    uint8_t hour = state->hour;

    if ( movement_clock_mode_24h() )
        watch_set_indicator(WATCH_INDICATOR_24H);
    else {
        if ( hour >= 12 ) watch_set_indicator(WATCH_INDICATOR_PM);
        else watch_clear_indicator(WATCH_INDICATOR_PM);
        hour = hour % 12 ? hour % 12 : 12;
    }

    const char periods[][7] = {"AP", "A ", " P"};
    static char lcdbuf[7];
    sprintf(lcdbuf, "%2d%02d%s", hour, state->minute, periods[state->period]);

    watch_display_text(WATCH_POSITION_BOTTOM, lcdbuf);
}

static inline void button_beep() {
    // play a beep as confirmation for a button press (if applicable)
    if (movement_button_should_sound()) watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
}

//
// Exported
//

void watch_radio_alarm_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(watch_radio_alarm_face_state_t));
        memset(*context_ptr, 0, sizeof(watch_radio_alarm_face_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

void watch_radio_alarm_face_activate(void *context) {
    watch_radio_alarm_face_state_t *state = (watch_radio_alarm_face_state_t *)context;
    // Handle any tasks related to your watch face coming on screen.
    state->setting_mode = WATCH_RADIO_ALARM_FACE_SETTING_MODE_NONE;
}

bool watch_radio_alarm_face_loop(movement_event_t event, void *context) {
    watch_radio_alarm_face_state_t *state = (watch_radio_alarm_face_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            // Show your initial UI here.
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "ALM", "AL");
            if (state->alarm_is_on) watch_set_indicator(WATCH_INDICATOR_SIGNAL);
            watch_set_colon();
            _watch_radio_alarm_face_display_alarm_time(state);
            break;
        case EVENT_TICK:
            // No action needed for tick events in normal mode; we displayed our stuff in EVENT_ACTIVATE.
            if (state->setting_mode == WATCH_RADIO_ALARM_FACE_SETTING_MODE_NONE)
                break;
            
            // but in settings mode, we need to blink up the parameter we're setting.
            _watch_radio_alarm_face_display_alarm_time(state);
            if (event.subsecond % 2 == 0) {
                const watch_position_t blink_positions[] = {0, WATCH_POSITION_HOURS, WATCH_POSITION_MINUTES, WATCH_POSITION_SECONDS};
                watch_display_text(blink_positions[state->setting_mode], "  ");
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // You can use the Light button for your own purposes. Note that by default, Movement will also
            // illuminate the LED in response to EVENT_LIGHT_BUTTON_DOWN; to suppress that behavior, add an
            // empty case for EVENT_LIGHT_BUTTON_DOWN.
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            switch (state->setting_mode) {
                case WATCH_RADIO_ALARM_FACE_SETTING_MODE_NONE:
                    // If we're not in a setting mode, turn on the LED like normal.
                    movement_illuminate_led();
                    break;
                case WATCH_RADIO_ALARM_FACE_SETTING_MODE_SETTING_HOUR:
                    // If we're setting the hour, advance to minute set mode.
                    state->setting_mode = WATCH_RADIO_ALARM_FACE_SETTING_MODE_SETTING_MINUTE;
                    break;
                case WATCH_RADIO_ALARM_FACE_SETTING_MODE_SETTING_MINUTE:
                    // If we're setting the minute, advance to period set mode
                    state->setting_mode = WATCH_RADIO_ALARM_FACE_SETTING_MODE_SETTING_PERIOD;
                    break;
                case WATCH_RADIO_ALARM_FACE_SETTING_MODE_SETTING_PERIOD:
                    // If we're setting the period, advance back to normal mode and cancel fast tick.
                    state->setting_mode = WATCH_RADIO_ALARM_FACE_SETTING_MODE_NONE;
                    movement_request_tick_frequency(1);
                    // beep to confirm setting.
                    button_beep();
                    // also turn the alarm on since they just set it.
                    state->alarm_is_on = 1;
                    movement_set_alarm_enabled(true);
                    watch_set_indicator(WATCH_INDICATOR_SIGNAL);
                    _watch_radio_alarm_face_display_alarm_time(state);
                    break;
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            // Just in case you have need for another button.
            if (state->setting_mode == WATCH_RADIO_ALARM_FACE_SETTING_MODE_NONE) {
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
                case WATCH_RADIO_ALARM_FACE_SETTING_MODE_NONE:
                    // nothing to do here, alarm toggle is handled in EVENT_ALARM_BUTTON_UP.
                    break;
                case WATCH_RADIO_ALARM_FACE_SETTING_MODE_SETTING_HOUR:
                    // increment hour, wrap around to 0 at 11.
                    state->hour = (state->hour + 1) % 12;
                    break;
                case WATCH_RADIO_ALARM_FACE_SETTING_MODE_SETTING_MINUTE:
                    // increment minute, wrap around to 0 at 59.
                    state->minute = (state->minute + 1) % 60;
                    break;
                case WATCH_RADIO_ALARM_FACE_SETTING_MODE_SETTING_PERIOD:
                    // toggle AMPM/AM/PM period, wrap around to AMPM at PM.
                    state->period = (state->period + 1) % 3;
                    break;
            }
            _watch_radio_alarm_face_display_alarm_time(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (state->setting_mode == WATCH_RADIO_ALARM_FACE_SETTING_MODE_NONE) {
                // long press in normal mode: move to hour setting mode, request fast tick.
                state->setting_mode = WATCH_RADIO_ALARM_FACE_SETTING_MODE_SETTING_HOUR;
                movement_request_tick_frequency(4);
                button_beep();
            }
            break;
        case EVENT_BACKGROUND_TASK:
            movement_play_alarm();
                // 2022-07-23: Thx @joeycastillo for the dedicated “alarm” signal
            break;
        case EVENT_TIMEOUT:
            // Your watch face will receive this event after a period of inactivity. If it makes sense to resign,
            // you may uncomment this line to move back to the first watch face in the list:
            movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            // If you did not resign in EVENT_TIMEOUT, you can use this event to update the display once a minute.
            // Avoid displaying fast-updating values like seconds, since the display won't update again for 60 seconds.
            // You should also consider starting the tick animation, to show the wearer that this is sleep mode:
            // watch_start_sleep_animation(500);
            break;
        default:
            // Movement's default loop handler will step in for any cases you don't handle above:
            // * EVENT_LIGHT_BUTTON_DOWN lights the LED
            // * EVENT_MODE_BUTTON_UP moves to the next watch face in the list
            // * EVENT_MODE_LONG_PRESS returns to the first watch face (or skips to the secondary watch face, if configured)
            // You can override any of these behaviors by adding a case for these events to this switch statement.
            return movement_default_loop_handler(event);
    }

    // return true if the watch can enter standby mode. Generally speaking, you should always return true.
    return true;
}

void watch_radio_alarm_face_resign(void *context) {
    (void) context;
    // handle any cleanup before your watch face goes off-screen.
}

movement_watch_face_advisory_t watch_radio_alarm_face_advise(void *context) {
    watch_radio_alarm_face_state_t *state = (watch_radio_alarm_face_state_t *)context;
    movement_watch_face_advisory_t retval = { 0 };

    if ( state->alarm_is_on ) {
        watch_date_time_t now = movement_get_local_date_time();
        bool wants_background_task_am = false, wants_background_task_pm = false;
        if (state->period != WATCH_RADIO_ALARM_FACE_PERIOD_PM) {
            wants_background_task_am = (state->hour==now.unit.hour && state->minute==now.unit.minute);
        }
        if (state->period != WATCH_RADIO_ALARM_FACE_PERIOD_AM) {
            wants_background_task_pm = (state->hour+12==now.unit.hour && state->minute==now.unit.minute);
        }
        retval.wants_background_task = wants_background_task_am || wants_background_task_pm;
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
