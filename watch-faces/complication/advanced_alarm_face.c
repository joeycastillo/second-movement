/*
 * MIT License
 *
 * Copyright (c) 2022 Andreas Nebinger
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

#include "advanced_alarm_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_common_display.h"
#include "delay.h"

typedef enum {
    alarm_setting_idx_alarm,
    alarm_setting_idx_day,
    alarm_setting_idx_hour,
    alarm_setting_idx_minute,
    alarm_setting_idx_pitch,
    alarm_setting_idx_beeps
} alarm_setting_idx_t;

static const char _dow_strings_classic[ALARM_DAY_STATES + 1][2] ={"AL",  "MO",  "TU",  "WE",  "TH",  "FR",  "SA",  "SU",  "ED",  "1t",  "MF",  "WN"};
static const char _dow_strings_custom[ALARM_DAY_STATES + 1][3] ={ "AL ", "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN", "DAY", "1t ", "M-F", "WKD"};
static const uint8_t _beeps_blink_idx = 9;
static const watch_buzzer_note_t _buzzer_notes[3] = {BUZZER_NOTE_B6, BUZZER_NOTE_C8, BUZZER_NOTE_A8};

// Volume is indicated by the three segments 5D, 5G and 5A
// This mapping is for classic LCD; if custom LCD is in use, we change it in the setup function.
static uint8_t _buzzer_segdata[3][2] = {{0, 3}, {1, 3}, {2, 2}};

static int8_t _wait_ticks;

static uint8_t _get_weekday_idx(watch_date_time_t date_time) {
    date_time.unit.year += 20;
    if (date_time.unit.month <= 2) {
        date_time.unit.month += 12;
        date_time.unit.year--;
    }
    return (date_time.unit.day + 13 * (date_time.unit.month + 1) / 5 + date_time.unit.year + date_time.unit.year / 4 + 525 - 2) % 7;
}

static void _alarm_set_signal(alarm_state_t *state) {
    if (state->alarm[state->alarm_idx].enabled)
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    else
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
}

static void _alarm_show_alarm_on_text(alarm_state_t *state) {
    watch_display_text(WATCH_POSITION_SECONDS, state->alarm[state->alarm_idx].enabled ? "on" : "--");
}

static void _advanced_alarm_face_draw(alarm_state_t *state, uint8_t subsecond) {
    char buf[12];
    bool set_leading_zero = movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_024H;

    uint8_t i = 0;
    if (state->is_setting) {
        // display the actual day indicating string for the current alarm
        i = state->alarm[state->alarm_idx].day + 1;
    }
    //handle am/pm for hour display
    uint8_t h = state->alarm[state->alarm_idx].hour;
    if (!movement_clock_mode_24h()) {
        if (h >= 12) {
            watch_set_indicator(WATCH_INDICATOR_PM);
            h %= 12;
        } else {
            watch_clear_indicator(WATCH_INDICATOR_PM);
        }
        if (h == 0) h = 12;
    } else {
        watch_set_indicator(WATCH_INDICATOR_24H);
    }

    // blink items if in settings mode
    bool blinking = state->is_setting && subsecond % 2 && state->setting_state < alarm_setting_idx_pitch && !state->alarm_quick_ticks;
    if (state->setting_state == alarm_setting_idx_alarm && blinking) {
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    } else {
        sprintf(buf, "%2d", (state->alarm_idx + 1));
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }
    if (state->setting_state == alarm_setting_idx_day && blinking) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "   ", "  ");
    } else {
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, _dow_strings_custom[i], _dow_strings_classic[i]);
    }
    if (state->setting_state == alarm_setting_idx_hour && blinking) {
        watch_display_text(WATCH_POSITION_HOURS, "  ");
    } else {
        sprintf(buf, set_leading_zero? "%02d" : "%2d", h);
        watch_display_text(WATCH_POSITION_HOURS, buf);
    }
    if (state->setting_state == alarm_setting_idx_minute && blinking) {
        watch_display_text(WATCH_POSITION_MINUTES, "  ");
    } else {
        sprintf(buf, "%02d", state->alarm[state->alarm_idx].minute);
        watch_display_text(WATCH_POSITION_MINUTES, buf);
    }

    if (state->is_setting) {
        watch_display_text(WATCH_POSITION_SECONDS, "  ");
    // draw pitch level indicator
        if ((subsecond % 2) == 0 || (state->setting_state != alarm_setting_idx_pitch)) {
        for (i = 0; i <= state->alarm[state->alarm_idx].pitch && i < 3; i++)
            watch_set_pixel(_buzzer_segdata[i][0], _buzzer_segdata[i][1]);
        }
        // draw beep rounds indicator
        if ((subsecond % 2) == 0 || (state->setting_state != alarm_setting_idx_beeps)) {
            if (state->alarm[state->alarm_idx].beeps == ALARM_MAX_BEEP_ROUNDS - 1)
                watch_display_character('L', _beeps_blink_idx);
            else {
                if (state->alarm[state->alarm_idx].beeps == 0)
                    watch_display_character('o', _beeps_blink_idx);
                else
                    watch_display_character(state->alarm[state->alarm_idx].beeps + 48, _beeps_blink_idx);
            }
        }
    }
    else {
        _alarm_show_alarm_on_text(state);
    }

    // set alarm indicator
    _alarm_set_signal(state);
}

static void _alarm_initiate_setting(alarm_state_t *state, uint8_t subsecond) {
    state->is_setting = true;
    state->setting_state = 0;
    movement_request_tick_frequency(4);
    _advanced_alarm_face_draw(state, subsecond);
}

static void _alarm_resume_setting(alarm_state_t *state, uint8_t subsecond) {
    state->is_setting = false;
    movement_request_tick_frequency(1);
    _advanced_alarm_face_draw(state, subsecond);
}

static void _alarm_update_alarm_enabled(alarm_state_t *state) {
    // save indication for active alarms to movement settings
    bool active_alarms = false;
    watch_date_time_t now;
    bool now_init = false;
    uint8_t weekday_idx;
    uint16_t now_minutes_of_day;
    uint16_t alarm_minutes_of_day;
    for (uint8_t i = 0; i < ALARM_ALARMS; i++) {
        if (state->alarm[i].enabled) {
            // figure out if alarm is to go off in the next 24 h
            if (state->alarm[i].day == ALARM_DAY_EACH_DAY || state->alarm[i].day == ALARM_DAY_ONE_TIME) {
            active_alarms = true;
            break;
            } else {
                if (!now_init) {
                    now = movement_get_local_date_time();
                    now_init = true;
                    weekday_idx = _get_weekday_idx(now);
                    now_minutes_of_day = now.unit.hour * 60 + now.unit.minute;
                }
                alarm_minutes_of_day = state->alarm[i].hour * 60 + state->alarm[i].minute;
                // no more shortcuts: check days and times for all possible cases...
                if ((state->alarm[i].day == weekday_idx && alarm_minutes_of_day >= now_minutes_of_day)
                    || ((weekday_idx + 1) % 7 == state->alarm[i].day && alarm_minutes_of_day <= now_minutes_of_day) 
                    || (state->alarm[i].day == ALARM_DAY_WORKDAY && (weekday_idx < 4
                        || (weekday_idx == 4 && alarm_minutes_of_day >= now_minutes_of_day)
                        || (weekday_idx == 6 && alarm_minutes_of_day <= now_minutes_of_day)))
                    || (state->alarm[i].day == ALARM_DAY_WEEKEND && (weekday_idx == 5
                        || (weekday_idx == 6 && alarm_minutes_of_day >= now_minutes_of_day)
                        || (weekday_idx == 4 && alarm_minutes_of_day <= now_minutes_of_day)))) {
                    active_alarms = true;
                    break;
                }
            }
        }
    }
    movement_set_alarm_enabled(active_alarms);
}

static void _alarm_play_short_beep(uint8_t pitch_idx) {
    // play a short double beep
    watch_buzzer_play_note(_buzzer_notes[pitch_idx], 50);
    watch_buzzer_play_note(BUZZER_NOTE_REST, 50);
    watch_buzzer_play_note(_buzzer_notes[pitch_idx], 70);
}

static void _alarm_indicate_beep(alarm_state_t *state) {
    // play an example for the current beep setting
    if (state->alarm[state->alarm_idx].beeps == 0) {
        // short double beep
        _alarm_play_short_beep(state->alarm[state->alarm_idx].pitch);
    } else {
        // regular alarm beep
        movement_play_alarm_beeps(1, _buzzer_notes[state->alarm[state->alarm_idx].pitch]);
    }
}

static void _abort_quick_ticks(alarm_state_t *state) {
    // abort counting quick ticks
    if (state->alarm_quick_ticks) {
        state->alarm[state->alarm_idx].enabled = true;
        state->alarm_quick_ticks = false;
        movement_request_tick_frequency(4);
    }
}

void advanced_alarm_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(alarm_state_t));
        alarm_state_t *state = (alarm_state_t *)*context_ptr;
        memset(*context_ptr, 0, sizeof(alarm_state_t));
        // initialize the default alarm values
        for (uint8_t i = 0; i < ALARM_ALARMS; i++) {
            state->alarm[i].day = ALARM_DAY_EACH_DAY;
            state->alarm[i].beeps = 5;
            state->alarm[i].pitch = 1;
        }
        state->alarm_handled_minute = -1;
        _wait_ticks = -1;

        if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
            _buzzer_segdata[0][0] = 1;
            _buzzer_segdata[0][1] = 5;
            _buzzer_segdata[1][0] = 2;
            _buzzer_segdata[1][1] = 5;
            _buzzer_segdata[2][0] = 3;
            _buzzer_segdata[2][1] = 10;
        }
    }
}

void advanced_alarm_face_activate(void *context) {
    (void) context;
    watch_set_colon();
}

void advanced_alarm_face_resign(void *context) {
    alarm_state_t *state = (alarm_state_t *)context;
    state->is_setting = false;
    _alarm_update_alarm_enabled(state);
    watch_set_led_off();
    state->alarm_quick_ticks = false;
    _wait_ticks = -1;
    movement_request_tick_frequency(1);
}

movement_watch_face_advisory_t advanced_alarm_face_advise(void *context) {
    alarm_state_t *state = (alarm_state_t *)context;
    movement_watch_face_advisory_t retval = { 0 };

    watch_date_time_t now = movement_get_local_date_time();
    // just a failsafe: never fire more than one alarm within a minute
    if (state->alarm_handled_minute == now.unit.minute) return retval;
    state->alarm_handled_minute = now.unit.minute;
    // check the rest
    for (uint8_t i = 0; i < ALARM_ALARMS; i++) {
        if (state->alarm[i].enabled) {
            if (state->alarm[i].minute == now.unit.minute) {
                if (state->alarm[i].hour == now.unit.hour) {
                    state->alarm_playing_idx = i;
                    if (state->alarm[i].day == ALARM_DAY_EACH_DAY || state->alarm[i].day == ALARM_DAY_ONE_TIME) retval.wants_background_task = true;
                    uint8_t weekday_idx = _get_weekday_idx(now);
                    if (state->alarm[i].day == weekday_idx) retval.wants_background_task = true;
                    if (state->alarm[i].day == ALARM_DAY_WORKDAY && weekday_idx < 5) retval.wants_background_task = true;
                    if (state->alarm[i].day == ALARM_DAY_WEEKEND && weekday_idx >= 5) retval.wants_background_task = true;
                }
            }
        }
    }

    if (retval.wants_background_task) {
        // FIXME for #SecondMovement: I think that this replicates the previous behavior of returning true in the conditionals above,
        // but would like to do more testing
        return retval;
    }

    state->alarm_handled_minute = -1;
    // update the movement's alarm indicator five times an hour
    if (now.unit.minute % 12 == 0) _alarm_update_alarm_enabled(state);
    return retval;
}

bool advanced_alarm_face_loop(movement_event_t event, void *context) {
    alarm_state_t *state = (alarm_state_t *)context;

    switch (event.event_type) {
    case EVENT_TICK:
        if (state->alarm_quick_ticks) {
            // we are in fast cycling mode
            if (state->setting_state == alarm_setting_idx_hour) {
                        state->alarm[state->alarm_idx].hour = (state->alarm[state->alarm_idx].hour + 1) % 24;
            } else if (state->setting_state == alarm_setting_idx_minute) {
                        state->alarm[state->alarm_idx].minute = (state->alarm[state->alarm_idx].minute + 1) % 60;
            } else _abort_quick_ticks(state);
        } else if (!state->is_setting) {
            if (_wait_ticks >= 0) _wait_ticks++;
            if (_wait_ticks == 2) {
                // extra long press of alarm button
                _wait_ticks = -1;
                if (state->alarm_idx) {
                    // revert change of enabled flag and show it briefly
                    state->alarm[state->alarm_idx].enabled ^= 1;
                    _alarm_set_signal(state);
                    _alarm_show_alarm_on_text(state);
                    delay_ms(275);
                    state->alarm_idx = 0;
                }
            } else break; // no need to do anything when we are not in settings mode and no quick ticks are running
        }
        // fall through
    case EVENT_ACTIVATE:
        _advanced_alarm_face_draw(state, event.subsecond);
        break;
    case EVENT_LIGHT_BUTTON_UP:
        if (!state->is_setting) {
            movement_illuminate_led();
            _alarm_initiate_setting(state, event.subsecond);
            break;
        }
        state->setting_state += 1;
        if (state->setting_state >= ALARM_SETTING_STATES) {
            // we have done a full settings cycle, so resume to normal
            _alarm_resume_setting(state, event.subsecond);
        }
        break;
    case EVENT_LIGHT_LONG_PRESS:
        if (state->is_setting) {
            _alarm_resume_setting(state, event.subsecond);
        } else {
            _alarm_initiate_setting(state, event.subsecond);
        }
        break;
    case EVENT_ALARM_BUTTON_UP:
        if (!state->is_setting) {
            // stop wait ticks counter
            _wait_ticks = -1;
            // cycle through the alarms
            state->alarm_idx = (state->alarm_idx + 1) % (ALARM_ALARMS);
        } else {
            // handle the settings behaviour
            switch (state->setting_state) {
            case alarm_setting_idx_alarm:
                // alarm selection
                state->alarm_idx = (state->alarm_idx + 1) % (ALARM_ALARMS);
                break;
            case alarm_setting_idx_day:
                // day selection
                state->alarm[state->alarm_idx].day = (state->alarm[state->alarm_idx].day + 1) % (ALARM_DAY_STATES);
                break;
            case alarm_setting_idx_hour:
                // hour selection
                _abort_quick_ticks(state);
                state->alarm[state->alarm_idx].hour = (state->alarm[state->alarm_idx].hour + 1) % 24;
                break;
            case alarm_setting_idx_minute:
                // minute selection
                _abort_quick_ticks(state);
                state->alarm[state->alarm_idx].minute = (state->alarm[state->alarm_idx].minute + 1) % 60;
                break;
            case alarm_setting_idx_pitch:
                // pitch level
                state->alarm[state->alarm_idx].pitch = (state->alarm[state->alarm_idx].pitch + 1) % 3;
                // play sound to show user what this is for
                _alarm_indicate_beep(state);
                break;
            case alarm_setting_idx_beeps:
                // number of beeping rounds selection
                state->alarm[state->alarm_idx].beeps = (state->alarm[state->alarm_idx].beeps + 1) % ALARM_MAX_BEEP_ROUNDS;
                // play sounds when user reaches 'short' length and also one time on regular beep length
                if (state->alarm[state->alarm_idx].beeps <= 1) _alarm_indicate_beep(state);
                break;
            default:
                break;
            }
            // auto enable an alarm if user sets anything
            if (state->setting_state > alarm_setting_idx_alarm) state->alarm[state->alarm_idx].enabled = true;
        }
        _advanced_alarm_face_draw(state, event.subsecond);
        break;
    case EVENT_ALARM_LONG_PRESS:
        if (!state->is_setting) {
            // toggle the enabled flag for current alarm
            state->alarm[state->alarm_idx].enabled ^= 1;
            // start wait ticks counter
            _wait_ticks = 0;
        } else {
            // handle the long press settings behaviour
            switch (state->setting_state) {
            case alarm_setting_idx_alarm:
                // alarm selection
                state->alarm_idx = 0;
                break;
            case alarm_setting_idx_minute:
            case alarm_setting_idx_hour:
                // initiate fast cycling for hour or minute settings
                movement_request_tick_frequency(8);
                state->alarm_quick_ticks = true;
                break;
            default:
                break;
            }
        }
        _advanced_alarm_face_draw(state, event.subsecond);
        break;
    case EVENT_ALARM_LONG_UP:
        if (state->is_setting) {
            if (state->setting_state == alarm_setting_idx_hour || state->setting_state == alarm_setting_idx_minute)
                _abort_quick_ticks(state);
        } else _wait_ticks = -1;
        break;
    case EVENT_BACKGROUND_TASK:
        // play alarm
        if (state->alarm[state->alarm_playing_idx].beeps == 0) {
            // short beep
            if (watch_is_buzzer_or_led_enabled()) {
                _alarm_play_short_beep(state->alarm[state->alarm_playing_idx].pitch);
            } else {
                // enable, play beep and disable buzzer again
                watch_enable_buzzer();
                _alarm_play_short_beep(state->alarm[state->alarm_playing_idx].pitch);
                watch_disable_buzzer();
            }
        } else {
            // regular alarm beeps
            movement_play_alarm_beeps((state->alarm[state->alarm_playing_idx].beeps == (ALARM_MAX_BEEP_ROUNDS - 1) ? 20 : state->alarm[state->alarm_playing_idx].beeps), 
                                  _buzzer_notes[state->alarm[state->alarm_playing_idx].pitch]);
        }
        // one time alarm? -> erase it
        if (state->alarm[state->alarm_playing_idx].day == ALARM_DAY_ONE_TIME) {
            state->alarm[state->alarm_playing_idx].day = ALARM_DAY_EACH_DAY;
            state->alarm[state->alarm_playing_idx].minute = state->alarm[state->alarm_playing_idx].hour = 0;
            state->alarm[state->alarm_playing_idx].beeps = 5;
            state->alarm[state->alarm_playing_idx].pitch = 1;
            state->alarm[state->alarm_playing_idx].enabled = false;
            _alarm_update_alarm_enabled(state);
        }
        break;
    case EVENT_TIMEOUT:
        movement_move_to_face(0);
        break;
    case EVENT_LIGHT_BUTTON_DOWN:
        // don't light up every time light is hit
        break;
    default:
        movement_default_loop_handler(event);
        break;
    }

    return true;
}
