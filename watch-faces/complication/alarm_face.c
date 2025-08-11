/*
 * MIT License
 *
 * Copyright (c) 2022 Josh Berson, building on Wesley Ellis’ countdown_face.c
 * Copyright (c) 2025 Joey Castillo
 * Copyright (c) 2025 Alessandro Genova
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
    uint8_t hour = state->alarms[state->alarm_index].hour;
    uint8_t minute = state->alarms[state->alarm_index].minute;
    bool enabled = state->alarms[state->alarm_index].enabled;

    if (state->alarm_index == ALARM_FACE_SNOOZE_ALARM_INDEX) {
        watch_set_indicator(WATCH_INDICATOR_LAP);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_LAP);
    }

    static char lcdbuf[7];

    if (state->alarm_index == ALARM_FACE_CHIME_INDEX) {
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");

        sprintf(lcdbuf, "  %02d%s", minute, enabled ? "on" : "  ");
    } else {
        sprintf(lcdbuf, "%2d", state->alarm_index + 1);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, lcdbuf);

        if ( movement_clock_mode_24h() )
            watch_set_indicator(WATCH_INDICATOR_24H);
        else {
            if ( hour >= 12 ) watch_set_indicator(WATCH_INDICATOR_PM);
            else watch_clear_indicator(WATCH_INDICATOR_PM);
            hour = hour % 12 ? hour % 12 : 12;
        }

        sprintf(lcdbuf, "%2d%02d%s", hour, minute, enabled ? "on" : "  ");
    }

    watch_display_text(WATCH_POSITION_BOTTOM, lcdbuf);
}

static inline void button_beep() {
    // play a beep as confirmation for a button press (if applicable)
    if (movement_button_should_sound()) watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
}

static bool any_alarm_is_on(alarm_face_state_t *state) {
    bool alarm_on = false;

    for (uint8_t i = 0; i < ALARM_FACE_NUM_ALARMS; i++) {
        if (i != ALARM_FACE_CHIME_INDEX) {
            alarm_on = alarm_on || state->alarms[i].enabled;
        }
    }

    return alarm_on;
}

static bool chime_is_on(alarm_face_state_t *state) {
    return state->alarms[ALARM_FACE_CHIME_INDEX].enabled;
}

static void _alarm_face_update_indicators(alarm_face_state_t *state) {
    if ( any_alarm_is_on(state) ) {
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
        movement_set_alarm_enabled(true);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
        movement_set_alarm_enabled(false);
    }

    if ( chime_is_on(state) ) {
        watch_set_indicator(WATCH_INDICATOR_BELL);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_BELL);
    }
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
        for (uint8_t i = 0; i < ALARM_FACE_NUM_ALARMS; i++) {
            state->alarms[i].hour = 8;
        }
    }
}

void alarm_face_activate(void *context) {
    alarm_face_state_t *state = (alarm_face_state_t *)context;
    state->setting_mode = ALARM_FACE_SETTING_MODE_NONE;
    state->quick_increase = 0;

    // Don't play remaining snooze alarms if the user enters this face
    state->next_snooze_alarm.hour = state->alarms[ALARM_FACE_SNOOZE_ALARM_INDEX].hour;
    state->next_snooze_alarm.minute = state->alarms[ALARM_FACE_SNOOZE_ALARM_INDEX].minute;
    state->remaining_snooze_repetitions = ALARM_FACE_SNOOZE_REPETITIONS;
}
void alarm_face_resign(void *context) {
    alarm_face_state_t *state = (alarm_face_state_t *)context;

    // Just in case we exit the alarm setting for the snooze alarm by pressing mode or timeout,
    // sync up the next_snooze_alarm.
    state->next_snooze_alarm.hour = state->alarms[ALARM_FACE_SNOOZE_ALARM_INDEX].hour;
    state->next_snooze_alarm.minute = state->alarms[ALARM_FACE_SNOOZE_ALARM_INDEX].minute;
}

bool alarm_face_loop(movement_event_t event, void *context) {
    alarm_face_state_t *state = (alarm_face_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "ALM", "AL");
            if (any_alarm_is_on(state)) watch_set_indicator(WATCH_INDICATOR_SIGNAL);
            if ( chime_is_on(state) ) watch_set_indicator(WATCH_INDICATOR_BELL);
            watch_set_colon();
            _alarm_face_display_alarm_time(state);
            break;
        case EVENT_TICK:
            switch (state->setting_mode) {
                // No action needed for tick events in normal mode; we displayed our stuff in EVENT_ACTIVATE.
                case ALARM_FACE_SETTING_MODE_NONE:
                    break;

                // in settings mode, we need to either quick increase or blink up the parameter we're setting.
                case ALARM_FACE_SETTING_MODE_SETTING_HOUR:
                    if (state->quick_increase) {
                        // increment hour, wrap around to 0 at 23.
                        state->alarms[state->alarm_index].hour = (state->alarms[state->alarm_index].hour + 1) % 24;
                        _alarm_face_display_alarm_time(state);
                    } else {
                        _alarm_face_display_alarm_time(state);
                        if (event.subsecond % 2 == 0) {
                            watch_display_text(WATCH_POSITION_HOURS, "  ");
                        }
                    }
                    break;

                case ALARM_FACE_SETTING_MODE_SETTING_MINUTE:
                    if (state->quick_increase) {
                        // increment minute, wrap around to 0 at 59.
                        state->alarms[state->alarm_index].minute = (state->alarms[state->alarm_index].minute + 1) % 60;
                        _alarm_face_display_alarm_time(state);
                    } else {
                        _alarm_face_display_alarm_time(state);
                        if (event.subsecond % 2 == 0) {
                            watch_display_text(WATCH_POSITION_MINUTES, "  ");
                        }
                    }
                    break;
            }

            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            switch (state->setting_mode) {
                case ALARM_FACE_SETTING_MODE_NONE:
                    state->alarm_index  = (state->alarm_index + 1) % ALARM_FACE_NUM_ALARMS;
                    _alarm_face_display_alarm_time(state);
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

                    // If we just finished setting the snooze alarm, sync it up
                    if (state->alarm_index == ALARM_FACE_SNOOZE_ALARM_INDEX) {
                        state->next_snooze_alarm.hour = state->alarms[ALARM_FACE_SNOOZE_ALARM_INDEX].hour;
                        state->next_snooze_alarm.minute = state->alarms[ALARM_FACE_SNOOZE_ALARM_INDEX].minute;
                        state->remaining_snooze_repetitions = ALARM_FACE_SNOOZE_REPETITIONS;
                    }

                    _alarm_face_display_alarm_time(state);
                    break;
            }
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            switch (state->setting_mode) {
                case ALARM_FACE_SETTING_MODE_NONE:
                    state->alarms[state->alarm_index].enabled ^= 1;
                    _alarm_face_update_indicators(state);

                    break;
                case ALARM_FACE_SETTING_MODE_SETTING_HOUR:
                    // increment hour, wrap around to 0 at 23.
                    state->alarms[state->alarm_index].hour = (state->alarms[state->alarm_index].hour + 1) % 24;
                    break;
                case ALARM_FACE_SETTING_MODE_SETTING_MINUTE:
                    // increment minute, wrap around to 0 at 59.
                    state->alarms[state->alarm_index].minute = (state->alarms[state->alarm_index].minute + 1) % 60;
                    break;
            }
            _alarm_face_display_alarm_time(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            switch (state->setting_mode) {
                case ALARM_FACE_SETTING_MODE_NONE:
                    if (state->alarm_index == ALARM_FACE_CHIME_INDEX) {
                        state->setting_mode = ALARM_FACE_SETTING_MODE_SETTING_MINUTE;
                    } else {
                        state->setting_mode = ALARM_FACE_SETTING_MODE_SETTING_HOUR;
                    }
                    // also turn the alarm on since we are setting it.
                    state->alarms[state->alarm_index].enabled = 1;
                    _alarm_face_update_indicators(state);

                    movement_request_tick_frequency(4);
                    button_beep();
                    break;
                case ALARM_FACE_SETTING_MODE_SETTING_HOUR:
                case ALARM_FACE_SETTING_MODE_SETTING_MINUTE:
                    state->quick_increase = 1;
                    movement_request_tick_frequency(8);
                    break;
                default:
                    break;
            }
            break;
        case EVENT_ALARM_LONG_UP:
            switch (state->setting_mode) {
                case ALARM_FACE_SETTING_MODE_SETTING_HOUR:
                case ALARM_FACE_SETTING_MODE_SETTING_MINUTE:
                    state->quick_increase = 0;
                    movement_request_tick_frequency(4);
                    break;
                default:
                    break;
            }
            break;
        case EVENT_BACKGROUND_TASK:
            if (state->play_alarm) {
                movement_play_alarm();
                // 2022-07-23: Thx @joeycastillo for the dedicated “alarm” signal
            } else if (state->play_signal) {
                movement_play_signal();
            }

            state->play_alarm = false;
            state->play_signal = false;
            
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

    if ( chime_is_on(state) || any_alarm_is_on(state) ) {
        watch_date_time_t now = movement_get_local_date_time();

        bool play_alarm = false;
        bool play_signal = false;

        for (uint8_t i = 0; i < ALARM_FACE_NUM_ALARMS; i++) {
            if (!state->alarms[i].enabled) {
                continue;
            }

            if (i == ALARM_FACE_CHIME_INDEX) {
                play_signal = state->alarms[ALARM_FACE_CHIME_INDEX].minute==now.unit.minute;
            } else if (i == ALARM_FACE_SNOOZE_ALARM_INDEX) {
                bool play_snooze_alarm = (state->next_snooze_alarm.hour==now.unit.hour && state->next_snooze_alarm.minute==now.unit.minute);
                if (play_snooze_alarm) {
                    state->remaining_snooze_repetitions -= 1;

                    if (state->remaining_snooze_repetitions > 0) {
                        // Repeate the snooze alarm in 5 minutes
                        state->next_snooze_alarm.minute += ALARM_FACE_SNOOZE_DELAY;
                        if (state->next_snooze_alarm.minute >= 60) {
                            state->next_snooze_alarm.minute %= 60;
                            state->next_snooze_alarm.hour += 1;
                        }
                    } else {
                        // We have repeated the snooze alarms the max number allowed, don't play it again until tomorrow
                        state->next_snooze_alarm.hour = state->alarms[ALARM_FACE_SNOOZE_ALARM_INDEX].hour;
                        state->next_snooze_alarm.minute = state->alarms[ALARM_FACE_SNOOZE_ALARM_INDEX].minute;
                        state->remaining_snooze_repetitions = ALARM_FACE_SNOOZE_REPETITIONS;
                    }
                }
                play_alarm = play_alarm || play_snooze_alarm;
            } else {
                play_alarm = play_alarm || (state->alarms[i].hour==now.unit.hour && state->alarms[i].minute==now.unit.minute);
            }
        }

        state->play_alarm = play_alarm;
        state->play_signal = play_signal;

        retval.wants_background_task = play_alarm || play_signal;
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
