/*
 * MIT License
 *
 * Copyright (c) 2024 Joseph Bryant
 * Copyright (c) 2023 Konrad Rieck
 * Copyright (c) 2022 Wesley Ellis
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
#include "kitchen_timer_face.h"
#include "watch.h"
#include "watch_utility.h"

#define KT_SELECTIONS 3
#define DEFAULT_MINUTES 3
#define TAP_DETECTION_SECONDS 5

// We need a scheduled background task to keep the watch from entering deep
// sleep while a timer is in overtime. This date is far enough in the future
// that it will never actually fire, but keeps the system awake.
static const watch_date_time_t distant_future = {
    .unit = {0, 0, 0, 1, 1, 63}
};

static bool quick_ticks_running;

static void abort_quick_ticks(kitchen_timer_state_t *state) {
    if (quick_ticks_running) {
        quick_ticks_running = false;
        // Restore the appropriate tick frequency for the current timer's mode
        if (state->timers[state->timer_idx].mode == kt_setting)
            movement_request_tick_frequency(4);
        else
            movement_request_tick_frequency(1);
    }
}

static void abort_tap_detection(kitchen_timer_state_t *state) {
    state->tap_detection_ticks = 0;
    movement_disable_tap_detection_if_available();
}

// Save the current countdown values so we can restore them after a reset
static inline void store_countdown(kitchen_timer_slot_t *slot) {
    slot->set_hours = slot->hours;
    slot->set_minutes = slot->minutes;
    slot->set_seconds = slot->seconds;
}

// Restore the previously saved countdown values (e.g. after pausing or resetting)
static inline void load_countdown(kitchen_timer_slot_t *slot) {
    slot->hours = slot->set_hours;
    slot->minutes = slot->set_minutes;
    slot->seconds = slot->set_seconds;
}

static inline void button_beep(void) {
    if (movement_button_should_sound()) watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
}

// With multiple independent timers, only one background task can be scheduled
// at a time. This function ensures we always schedule the most urgent one:
// the earliest running timer's expiry. If no timers are running but some are
// in overtime, we schedule a distant-future task to prevent deep sleep.
static void reschedule_nearest(kitchen_timer_state_t *state) {
    uint32_t earliest = 0;
    bool has_running = false;
    bool has_overtime = false;

    for (uint8_t i = 0; i < KT_MAX_TIMERS; i++) {
        if (state->timers[i].mode == kt_running) {
            if (!has_running || state->timers[i].target_ts < earliest) {
                earliest = state->timers[i].target_ts;
                has_running = true;
            }
        } else if (state->timers[i].mode == kt_overtime || state->timers[i].mode == kt_overtime_paused) {
            has_overtime = true;
        }
    }

    if (has_running) {
        watch_date_time_t target_dt = watch_utility_date_time_from_unix_time(earliest, movement_get_current_timezone_offset());
        movement_schedule_background_task_for_face(state->watch_face_index, target_dt);
    } else if (has_overtime) {
        movement_schedule_background_task_for_face(state->watch_face_index, distant_future);
    } else {
        movement_cancel_background_task_for_face(state->watch_face_index);
    }
}

static void schedule_countdown(kitchen_timer_state_t *state, uint8_t idx) {
    kitchen_timer_slot_t *slot = &state->timers[idx];
    // Sync now_ts from the RTC to avoid drift from tick-based approximation
    uint32_t new_now = watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), movement_get_current_timezone_offset());
    slot->target_ts = watch_utility_offset_timestamp(new_now, slot->hours, slot->minutes, slot->seconds);
    state->now_ts = new_now;
    reschedule_nearest(state);
}

static void enter_overtime(kitchen_timer_state_t *state, uint8_t idx, bool play_alarm) {
    kitchen_timer_slot_t *slot = &state->timers[idx];
    if (play_alarm) movement_play_alarm();
    slot->mode = kt_overtime;
    slot->overtime_start_ts = watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), movement_get_current_timezone_offset());
    slot->overtime_seconds = 0;
    // Only change tick frequency if this timer is currently displayed,
    // otherwise we'd disrupt the viewing timer's display cadence
    if (state->timer_idx == idx) {
        movement_request_tick_frequency(1);
    }
    reschedule_nearest(state);
}

static void start(kitchen_timer_state_t *state, uint8_t idx) {
    state->timers[idx].mode = kt_running;
    schedule_countdown(state, idx);
    // If the timer already expired (e.g. zero-length), enter overtime now
    // because a background task scheduled for a past time may never fire
    if (state->timers[idx].target_ts <= state->now_ts) {
        enter_overtime(state, idx, false);
    }
}

static void reset_from_overtime(kitchen_timer_state_t *state, uint8_t idx) {
    kitchen_timer_slot_t *slot = &state->timers[idx];
    // Only touch display state if this timer is currently visible
    if (state->timer_idx == idx) {
        movement_request_tick_frequency(1);
        watch_clear_indicator(WATCH_INDICATOR_LAP);
    }
    load_countdown(slot);
    slot->mode = kt_reset;
    reschedule_nearest(state);
}

static void draw(kitchen_timer_state_t *state, uint8_t subsecond) {
    char buf[16];
    kitchen_timer_slot_t *slot = &state->timers[state->timer_idx];

    uint32_t delta;
    div_t result;

    switch (slot->mode) {
        case kt_running:
            // Derive h:m:s from the difference between target and now,
            // rather than decrementing fields, to stay in sync with the RTC
            if (slot->target_ts <= state->now_ts)
                delta = 0;
            else
                delta = slot->target_ts - state->now_ts;
            result = div(delta, 60);
            slot->seconds = result.rem;
            result = div(result.quot, 60);
            slot->hours = result.quot;
            slot->minutes = result.rem;
            sprintf(buf, "%2d%02d%02d", slot->hours, slot->minutes, slot->seconds);
            break;
        case kt_reset:
            watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
            sprintf(buf, "%2d%02d%02d", slot->hours, slot->minutes, slot->seconds);
            break;
        case kt_paused:
            watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
            sprintf(buf, "%2d%02d%02d", slot->hours, slot->minutes, slot->seconds);
            if (subsecond % 2) {
                buf[0] = buf[1] = buf[2] = buf[3] = buf[4] = buf[5] = ' ';
            }
            break;
        case kt_setting:
            sprintf(buf, "%2d%02d%02d", slot->hours, slot->minutes, slot->seconds);
            // Blink the selected field to indicate which one is being edited;
            // suppress blinking during quick ticks so digits remain readable
            if (!quick_ticks_running && subsecond % 2) {
                switch(state->selection) {
                    case 0:
                        buf[0] = buf[1] = ' ';
                        break;
                    case 1:
                        buf[2] = buf[3] = ' ';
                        break;
                    case 2:
                        buf[4] = buf[5] = ' ';
                        break;
                    default:
                        break;
                }
            }
            break;
        case kt_overtime: {
                uint32_t ot = slot->overtime_seconds;
                uint8_t ot_hours = ot / 3600;
                uint8_t ot_minutes = (ot % 3600) / 60;
                uint8_t ot_seconds = ot % 60;
                sprintf(buf, "%2d%02d%02d", ot_hours, ot_minutes, ot_seconds);
                watch_set_indicator(WATCH_INDICATOR_LAP);
            }
            break;
        case kt_overtime_paused: {
                uint32_t ot = slot->overtime_seconds;
                uint8_t ot_hours = ot / 3600;
                uint8_t ot_minutes = (ot % 3600) / 60;
                uint8_t ot_seconds = ot % 60;
                sprintf(buf, "%2d%02d%02d", ot_hours, ot_minutes, ot_seconds);
                watch_set_indicator(WATCH_INDICATOR_LAP);
                if (subsecond % 2) {
                    buf[0] = buf[1] = buf[2] = buf[3] = buf[4] = buf[5] = ' ';
                }
            }
            break;
    }

    watch_display_text(WATCH_POSITION_BOTTOM, buf);

    // SIGNAL indicator doubles as tap-detection feedback; show it while
    // the accelerometer is listening for taps
    if (state->tap_detection_ticks) {
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    }
}

static void draw_title(kitchen_timer_state_t *state) {
    char buf[4];
    watch_display_text(WATCH_POSITION_TOP_LEFT, "KT");
    // Show 1-based timer number so users see "1"–"4" instead of "0"–"3"
    sprintf(buf, "%2d", state->timer_idx + 1);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
}

static void pause_timer(kitchen_timer_state_t *state, uint8_t idx) {
    state->timers[idx].mode = kt_paused;
    watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    if (state->timer_idx == idx) {
        movement_request_tick_frequency(2);
    }
    reschedule_nearest(state);
}

static void reset_timer(kitchen_timer_state_t *state, uint8_t idx) {
    state->timers[idx].mode = kt_reset;
    load_countdown(&state->timers[idx]);
    reschedule_nearest(state);
}

static void settings_increment(kitchen_timer_slot_t *slot, uint8_t selection) {
    switch(selection) {
        case 0:
            slot->hours = (slot->hours + 1) % 24;
            break;
        case 1:
            slot->minutes = (slot->minutes + 1) % 60;
            break;
        case 2:
            slot->seconds = (slot->seconds + 1) % 60;
            break;
        default:
            break;
    }
}

// Check if any timer needs the watch to stay awake (running or overtime)
static bool any_timer_active(kitchen_timer_state_t *state) {
    for (uint8_t i = 0; i < KT_MAX_TIMERS; i++) {
        if (state->timers[i].mode == kt_running || state->timers[i].mode == kt_overtime || state->timers[i].mode == kt_overtime_paused)
            return true;
    }
    return false;
}

// Separate from any_timer_active because overtime timers don't need now_ts
// incremented — they compute elapsed time directly from the RTC
static bool any_timer_running(kitchen_timer_state_t *state) {
    for (uint8_t i = 0; i < KT_MAX_TIMERS; i++) {
        if (state->timers[i].mode == kt_running)
            return true;
    }
    return false;
}

// Tick frequency depends on what the user is currently looking at:
// paused states need 2 Hz for number blink, settings needs 4 Hz for field
// blink, everything else is fine at 1 Hz
static void update_tick_frequency_for_current(kitchen_timer_state_t *state) {
    switch (state->timers[state->timer_idx].mode) {
        case kt_paused:
        case kt_overtime_paused:
            movement_request_tick_frequency(2);
            break;
        case kt_setting:
            movement_request_tick_frequency(4);
            break;
        default:
            movement_request_tick_frequency(1);
            break;
    }
}

void kitchen_timer_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(kitchen_timer_state_t));
        kitchen_timer_state_t *state = (kitchen_timer_state_t *)*context_ptr;
        memset(*context_ptr, 0, sizeof(kitchen_timer_state_t));
        state->watch_face_index = watch_face_index;
        for (uint8_t i = 0; i < KT_MAX_TIMERS; i++) {
            state->timers[i].minutes = DEFAULT_MINUTES;
            state->timers[i].mode = kt_reset;
            store_countdown(&state->timers[i]);
        }
    }
}

void kitchen_timer_face_activate(void *context) {
    kitchen_timer_state_t *state = (kitchen_timer_state_t *)context;
    kitchen_timer_slot_t *slot = &state->timers[state->timer_idx];

    // Resync now_ts from the RTC because tick-based increments stopped
    // while this face was inactive
    if (any_timer_running(state)) {
        watch_date_time_t now = movement_get_utc_date_time();
        state->now_ts = watch_utility_date_time_to_unix_time(now, movement_get_current_timezone_offset());
    }

    if (slot->mode == kt_running) {
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    } else if (slot->mode == kt_overtime) {
        // Recompute elapsed overtime from the RTC since ticks weren't
        // running while the face was inactive
        uint32_t now_ts = watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), movement_get_current_timezone_offset());
        slot->overtime_seconds = now_ts - slot->overtime_start_ts;
        movement_request_tick_frequency(1);
        watch_set_colon();
        quick_ticks_running = false;
        return;
    } else if (slot->mode == kt_overtime_paused) {
        watch_set_indicator(WATCH_INDICATOR_LAP);
        movement_request_tick_frequency(2);
        watch_set_colon();
        quick_ticks_running = false;
        return;
    }

    watch_set_colon();
    update_tick_frequency_for_current(state);
    quick_ticks_running = false;

    // Only offer tap detection when the timer isn't counting down,
    // since taps are used for quick time entry on idle timers
    if (slot->mode != kt_running && movement_enable_tap_detection_if_available()) {
        state->tap_detection_ticks = TAP_DETECTION_SECONDS;
        state->has_tapped_once = false;
    }
}

bool kitchen_timer_face_loop(movement_event_t event, void *context) {
    kitchen_timer_state_t *state = (kitchen_timer_state_t *)context;
    kitchen_timer_slot_t *slot = &state->timers[state->timer_idx];

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();
            draw_title(state);
            draw(state, event.subsecond);
            break;
        case EVENT_TICK:
            if (quick_ticks_running) {
                if (HAL_GPIO_BTN_ALARM_read())
                    settings_increment(slot, state->selection);
                else
                    abort_quick_ticks(state);
            }

            // Only increment on subsecond == 0 to avoid double-counting
            // when tick frequency is >1 Hz (e.g. during overtime blink)
            if (any_timer_running(state) && event.subsecond == 0) {
                state->now_ts++;
            }

            // Overtime elapsed time is computed from the RTC on demand,
            // not from now_ts, to stay accurate regardless of tick rate
            if (slot->mode == kt_overtime) {
                uint32_t now_ts = watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), movement_get_current_timezone_offset());
                slot->overtime_seconds = now_ts - slot->overtime_start_ts;
            }

            if (state->tap_detection_ticks > 0) {
                state->tap_detection_ticks--;
                if (state->tap_detection_ticks == 0) movement_disable_tap_detection_if_available();
            }

            draw(state, event.subsecond);
            break;
        case EVENT_MODE_BUTTON_UP:
            abort_quick_ticks(state);
            movement_move_to_next_face();
            break;
        case EVENT_LIGHT_BUTTON_UP:
            switch(slot->mode) {
                case kt_running:
                case kt_reset:
                case kt_paused:
                case kt_overtime:
                case kt_overtime_paused:
                    state->timer_idx = (state->timer_idx + 1) % KT_MAX_TIMERS;
                    slot = &state->timers[state->timer_idx];
                    update_tick_frequency_for_current(state);
                    // Clear LAP from the previous timer before drawing the new
                    // one, otherwise it lingers on non-overtime timers
                    watch_clear_indicator(WATCH_INDICATOR_LAP);
                    draw_title(state);
                    if (slot->mode == kt_running) {
                        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
                    } else if (slot->mode == kt_overtime) {
                        // Recompute from RTC so the display is immediately
                        // correct, not stale from when overtime first started
                        uint32_t now_ts = watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), movement_get_current_timezone_offset());
                        slot->overtime_seconds = now_ts - slot->overtime_start_ts;
                    } else if (slot->mode == kt_overtime_paused) {
                        watch_set_indicator(WATCH_INDICATOR_LAP);
                    }
                    break;
                case kt_setting:
                    state->selection++;
                    if(state->selection >= KT_SELECTIONS) {
                        state->selection = 0;
                        slot->mode = kt_reset;
                        store_countdown(slot);
                        movement_request_tick_frequency(1);
                        button_beep();
                    }
                    break;
            }
            draw(state, event.subsecond);
            break;
        case EVENT_ALARM_BUTTON_UP:
            switch(slot->mode) {
                case kt_running:
                    pause_timer(state, state->timer_idx);
                    button_beep();
                    break;
                case kt_reset:
                case kt_paused:
                    // A zero-length timer will enter overtime immediately,
                    // acting as a "count up from now" stopwatch
                    abort_tap_detection(state);
                    start(state, state->timer_idx);
                    button_beep();
                    watch_set_indicator(WATCH_INDICATOR_SIGNAL);
                    break;
                case kt_setting:
                    settings_increment(slot, state->selection);
                    break;
                case kt_overtime: {
                    uint32_t now_ts = watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), movement_get_current_timezone_offset());
                    slot->overtime_seconds = now_ts - slot->overtime_start_ts;
                    slot->mode = kt_overtime_paused;
                    watch_set_indicator(WATCH_INDICATOR_LAP);
                    movement_request_tick_frequency(2);
                    button_beep();
                    break;
                }
                case kt_overtime_paused: {
                    uint32_t now_ts = watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), movement_get_current_timezone_offset());
                    slot->overtime_start_ts = now_ts - slot->overtime_seconds;
                    slot->mode = kt_overtime;
                    movement_request_tick_frequency(1);
                    button_beep();
                    break;
                }
            }
            draw(state, event.subsecond);
            break;
        case EVENT_ALARM_LONG_PRESS:
            switch(slot->mode) {
                case kt_reset:
                    abort_tap_detection(state);
                    slot->mode = kt_setting;
                    movement_request_tick_frequency(4);
                    button_beep();
                    break;
                case kt_setting:
                    quick_ticks_running = true;
                    movement_request_tick_frequency(8);
                    break;
                case kt_paused:
                    reset_timer(state, state->timer_idx);
                    button_beep();
                    break;
                case kt_running:
                    reset_timer(state, state->timer_idx);
                    movement_request_tick_frequency(1);
                    button_beep();
                    break;
                case kt_overtime:
                case kt_overtime_paused:
                    reset_from_overtime(state, state->timer_idx);
                    button_beep();
                    break;
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (slot->mode == kt_setting) {
                // Clear from the selected field onward to allow quick zeroing
                switch (state->selection) {
                    case 0:
                        slot->hours = 0;
                        // intentional fallthrough
                    case 1:
                        slot->minutes = 0;
                        // intentional fallthrough
                    case 2:
                        slot->seconds = 0;
                        break;
                }
            } else {
                movement_illuminate_led();
            }
            break;
        case EVENT_ALARM_LONG_UP:
            abort_quick_ticks(state);
            break;
        case EVENT_BACKGROUND_TASK:
            // Refresh now_ts from the RTC because tick-based approximation
            // may have drifted, and we need an accurate comparison here
            state->now_ts = watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), movement_get_current_timezone_offset());
            // Check all timers — multiple may have expired if their targets
            // were close together or the watch was asleep
            for (uint8_t i = 0; i < KT_MAX_TIMERS; i++) {
                if (state->timers[i].mode == kt_running && state->timers[i].target_ts <= state->now_ts) {
                    enter_overtime(state, i, true);
                }
            }
            break;
        case EVENT_TIMEOUT:
            // Auto-save and exit settings on timeout so edits aren't lost
            if (slot->mode == kt_setting) {
                state->selection = 0;
                slot->mode = kt_reset;
                store_countdown(slot);
                movement_request_tick_frequency(1);
            }
            // Stay on this face if any timer is active — the user likely
            // wants to see it when they next look at the watch
            if (!any_timer_active(state)) {
                movement_move_to_face(0);
            }
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            if (watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC) {
                watch_display_text(WATCH_POSITION_SECONDS, "  ");
            }
            if (!watch_sleep_animation_is_running()) watch_start_sleep_animation(1000);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_SINGLE_TAP:
            // First tap resets to 1 minute as a sensible starting point;
            // subsequent taps add a minute for quick approximate timing
            if (state->has_tapped_once == false) {
                state->has_tapped_once = true;
                slot->hours = 0;
                slot->minutes = 1;
                slot->seconds = 0;
            } else {
                slot->minutes = slot->minutes < 59 ? slot->minutes + 1 : slot->minutes;
            }
            state->tap_detection_ticks = TAP_DETECTION_SECONDS;
            draw(state, event.subsecond);
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void kitchen_timer_face_resign(void *context) {
    kitchen_timer_state_t *state = (kitchen_timer_state_t *)context;
    kitchen_timer_slot_t *slot = &state->timers[state->timer_idx];

    // Save any in-progress settings so they aren't lost when navigating away
    if (slot->mode == kt_setting) {
        state->selection = 0;
        slot->mode = kt_reset;
        store_countdown(slot);
    }

    // Reset to 1 Hz since we no longer need fast ticks for display effects;
    // background tasks handle overtime/countdown while the face is inactive
    movement_request_tick_frequency(1);
    abort_tap_detection(state);
}
