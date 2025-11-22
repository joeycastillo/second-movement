/*
 * MIT License
 *
 * Copyright (c) 2025 Gábor Nyéki
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include "baby_kicks_face.h"
#include "watch.h"
#include "watch_utility.h"

static inline void _play_failure_sound_if_beep_is_on() {
    if (movement_button_should_sound()) {
        watch_buzzer_play_note(BUZZER_NOTE_E7, 10);
    }
}

static inline void _play_successful_increment_sound_if_beep_is_on() {
    if (movement_button_should_sound()) {
        watch_buzzer_play_note(BUZZER_NOTE_E6, 10);
    }
}

static inline void _play_successful_decrement_sound_if_beep_is_on() {
    if (movement_button_should_sound()) {
        watch_buzzer_play_note(BUZZER_NOTE_D6, 10);
    }
}

static inline void _play_button_sound_if_beep_is_on() {
    if (movement_button_should_sound()) {
        watch_buzzer_play_note(BUZZER_NOTE_C7, 10);
    }
}

/** @brief Predicate for whether the counter has been started.
  */
static inline bool _is_running(baby_kicks_state_t *state) {
    return state->start > 0;
}

/** @brief Gets the current time, and caches it for re-use.
  */
static inline watch_date_time_t *_get_now(baby_kicks_state_t *state) {
    if (state->now.unit.year == 0) {
        state->now = movement_get_local_date_time();
    }

    return &state->now;
}

/** @brief Clears the current time.  Should only be called at the end of
  *        `baby_kicks_face_loop`.
  */
static inline void _clear_now(baby_kicks_state_t *state) {
    if (state->now.unit.year > 0) {
        memset(&state->now, 0, sizeof(state->now));
    }
}

/** @brief Calculates the number of minutes since the timer was started.
  * @return If the counter has been started, then the number of full
  *         minutes that have elapsed.  If it has not been started, then
  *         255.
  */
static inline uint32_t _elapsed_minutes(baby_kicks_state_t *state) {
    if (!_is_running(state)) {
        return 0xff;
    }

    watch_date_time_t *now = _get_now(state);

    return (
        watch_utility_date_time_to_unix_time(*now, 0) - state->start
    ) / 60;
}

/** @brief Predicate for whether the counter has started but run for too
  *        long.
  */
static inline bool _has_timed_out(baby_kicks_state_t *state) {
    return _elapsed_minutes(state) > BABY_KICKS_TIMEOUT;
}

/** @brief Determines what we should display based on `state`.  Should
  *        only be called from `baby_kicks_face_loop`.
  */
static void _update_display_mode(baby_kicks_state_t *state) {
    if (watch_sleep_animation_is_running()) {
        state->mode = BABY_KICKS_MODE_LE_MODE;
    } else if (!_is_running(state)) {
        state->mode = BABY_KICKS_MODE_SPLASH;
    } else if (_has_timed_out(state)) {
        state->mode = BABY_KICKS_MODE_TIMED_OUT;
    } else {
        state->mode = BABY_KICKS_MODE_ACTIVE;
    }
}

/** @brief Starts the counter.
  * @details Sets the start time which will be used to calculate the
  *          elapsed minutes.
  */
static inline void _start(baby_kicks_state_t *state) {
    watch_date_time_t *now = _get_now(state);
    uint32_t now_unix = watch_utility_date_time_to_unix_time(*now, 0);

    state->start = now_unix;
}

/** @brief Resets the counter.
  * @details Zeros out the watch face state and clears the undo ring
  *          buffer.  Effectively sets `state->mode` to
  *          `BABY_KICKS_MODE_SPLASH`.
  */
static void _reset(baby_kicks_state_t *state) {
    memset(state, 0, sizeof(baby_kicks_state_t));
    memset(
        state->undo_buffer.stretches,
        0xff,
        sizeof(state->undo_buffer.stretches)
    );
}

/** @brief Records a movement.
  * @details Increments the movement counter, and along with it, if
  *          necessary, the counter of one-minute stretches.  Also adds
  *          the movement to the undo buffer.
  */
static inline void _increment_counts(baby_kicks_state_t *state) {
    watch_date_time_t *now = _get_now(state);
    uint32_t now_unix = watch_utility_date_time_to_unix_time(*now, 0);

    /* Add movement to the undo ring buffer. */
    state->undo_buffer.stretches[state->undo_buffer.head] =
        state->stretch_count;
    state->undo_buffer.head++;
    state->undo_buffer.head %= sizeof(state->undo_buffer.stretches);

    state->movement_count++;

    if (state->stretch_count == 0
        || state->latest_stretch_start + 60 < now_unix) {
        /* Start new stretch. */
        state->latest_stretch_start = now_unix;
        state->stretch_count++;
    }
}

/** @brief Undoes the last movement.
  * @details Decrements the movement counter and, if necessary, the
  *          counter of one-minute stretches.
  * @return True if and only if there was a movement to undo.
  */
static inline bool _successfully_undo(baby_kicks_state_t *state) {
    uint8_t latest_mvmt, pre_undo_stretch_count;

    /* The latest movement is stored one position before `.head`. */
    if (state->undo_buffer.head == 0) {
        latest_mvmt = sizeof(state->undo_buffer.stretches) - 1;
    } else {
        latest_mvmt = state->undo_buffer.head - 1;
    }

    pre_undo_stretch_count =
        state->undo_buffer.stretches[latest_mvmt];

    if (pre_undo_stretch_count == 0xff) {
        /* Nothing to undo. */
        return false;
    } else if (pre_undo_stretch_count < state->stretch_count) {
        state->latest_stretch_start = 0;
        state->stretch_count--;
    }

    state->movement_count--;

    state->undo_buffer.stretches[latest_mvmt] = 0xff;
    state->undo_buffer.head = latest_mvmt;

    return true;
}

/** @brief Updates the display with the movement counts if the counter
  *        has been started.
  */
static inline void _display_counts(baby_kicks_state_t *state) {
    if (!_is_running(state)) {
        watch_display_text(WATCH_POSITION_BOTTOM, "baby  ");
        watch_clear_colon();
    } else {
        char buf[9];

        snprintf(
            buf,
            sizeof(buf),
            "%2d%4d",
            state->stretch_count,
            state->movement_count
        );

        watch_display_text(WATCH_POSITION_BOTTOM, buf);
        watch_set_colon();
    }
}

/** @brief Updates the display with the number of minutes since the
  *        timer was started.
  * @details If more than `BABY_KICKS_TIMEOUT` minutes have elapsed,
  *          then it displays "TO".
  */
static void _display_elapsed_minutes(baby_kicks_state_t *state) {
    if (!_is_running(state)) {
        watch_display_text(WATCH_POSITION_TOP_LEFT, "  ");
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    } else if (_has_timed_out(state)) {
        watch_display_text(WATCH_POSITION_TOP_LEFT, "TO");
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    } else {
        /* NOTE We display the elapsed minutes in two parts.  This is
         * because on the classic LCD, neither the "weekday digits" nor
         * the "day digits" position is suitable to display the elapsed
         * minutes:
         *
         * - The classic LCD cannot display 2, 4, 5, 6, or 9 as the last
         *   digit in the "weekday digits" position.
         * - It cannot display any number greater than 3 as the first
         *   digit in the "day digits" position.
         *
         * As a workaround, we split the elapsed minutes into 30-minute
         * "laps."  The elapsed minutes in the current "lap" are shown
         * in the "day digits" position.  This is any number between 0
         * and 29.  The elapsed minutes in past "laps" are shown in the
         * "weekday digits" position.  This is either nothing, 30, 60,
         * or 90.
         *
         * The sum of the numbers shown in the two positions is equal to
         * the total elapsed minutes.
         */

        char buf[5];
        uint32_t elapsed_minutes = _elapsed_minutes(state);
        uint8_t multiple = elapsed_minutes / 30;
        uint8_t remainder = elapsed_minutes % 30;

        if (multiple == 0) {
            watch_display_text(WATCH_POSITION_TOP_LEFT, "  ");
        } else {
            snprintf(buf, sizeof(buf), "%2d", multiple * 30);
            watch_display_text(WATCH_POSITION_TOP_LEFT, buf);
        }

        snprintf(buf, sizeof(buf), "%2d", remainder);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }
}

static void _update_display(baby_kicks_state_t *state) {
    _display_counts(state);
    _display_elapsed_minutes(state);
}

static inline void _start_sleep_face() {
    if (!watch_sleep_animation_is_running()) {
        watch_display_text(WATCH_POSITION_TOP_LEFT, "  ");
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
        watch_display_text(WATCH_POSITION_BOTTOM, "baby  ");

        watch_clear_colon();

        watch_start_sleep_animation(500);
    }
}

static inline void _stop_sleep_face() {
    if (watch_sleep_animation_is_running()) {
        watch_stop_sleep_animation();
    }
}

void baby_kicks_face_setup(uint8_t watch_face_index,
                           void **context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(baby_kicks_state_t));
        _reset(*context_ptr);
    }
}

void baby_kicks_face_activate(void *context) {
    (void) context;

    _stop_sleep_face();
}

void baby_kicks_face_resign(void *context) {
    baby_kicks_state_t *state = (baby_kicks_state_t *)context;

    state->currently_displayed = false;
}

bool baby_kicks_face_loop(movement_event_t event, void *context) {
    baby_kicks_state_t *state = (baby_kicks_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            state->currently_displayed = true;
            _update_display_mode(state);
            _update_display(state);
            break;
        case EVENT_ALARM_BUTTON_UP:     /* Start or increment. */
            /* Update `state->mode` in case we have a running counter
             * that has just timed out. */
            _update_display_mode(state);

            switch (state->mode) {
                case BABY_KICKS_MODE_SPLASH:
                    _start(state);
                    _update_display_mode(state);
                    _update_display(state);
                    _play_button_sound_if_beep_is_on();
                    break;
                case BABY_KICKS_MODE_ACTIVE:
                    _increment_counts(state);
                    _update_display(state);
                    _play_successful_increment_sound_if_beep_is_on();
                    break;
                case BABY_KICKS_MODE_TIMED_OUT:
                    _play_failure_sound_if_beep_is_on();
                    break;
                case BABY_KICKS_MODE_LE_MODE:   /* fallthrough */
                default:
                    break;
            }
            break;
        case EVENT_ALARM_LONG_PRESS:    /* Undo. */
            _update_display_mode(state);

            switch (state->mode) {
                case BABY_KICKS_MODE_ACTIVE:
                    if (!_successfully_undo(state)) {
                        _play_failure_sound_if_beep_is_on();
                    } else {
                        _update_display(state);
                        _play_successful_decrement_sound_if_beep_is_on();
                    }
                    break;
                case BABY_KICKS_MODE_SPLASH:    /* fallthrough */
                case BABY_KICKS_MODE_TIMED_OUT:
                    _play_failure_sound_if_beep_is_on();
                    break;
                case BABY_KICKS_MODE_LE_MODE:   /* fallthrough */
                default:
                    break;
            }
            break;
        case EVENT_MODE_LONG_PRESS:     /* Reset. */
            _update_display_mode(state);

            switch (state->mode) {
                case BABY_KICKS_MODE_ACTIVE:    /* fallthrough */
                case BABY_KICKS_MODE_TIMED_OUT:
                    _reset(state);

                    /* This shows the splash screen because `_reset`
                     * sets `state->mode` to `BABY_KICKS_MODE_SPLASH`.
                     */
                    _update_display(state);

                    _play_button_sound_if_beep_is_on();
                    break;
                case BABY_KICKS_MODE_SPLASH:
                    _play_failure_sound_if_beep_is_on();
                    break;
                case BABY_KICKS_MODE_LE_MODE:   /* fallthrough */
                default:
                    break;
            }
            break;
        case EVENT_BACKGROUND_TASK:     /* Update minute display. */
            _update_display_mode(state);

            switch (state->mode) {
                case BABY_KICKS_MODE_ACTIVE:    /* fallthrough */
                case BABY_KICKS_MODE_TIMED_OUT:
                    if (state->currently_displayed) {
                        _display_elapsed_minutes(state);
                    }
                    break;
                case BABY_KICKS_MODE_LE_MODE:   /* fallthrough */
                case BABY_KICKS_MODE_SPLASH:    /* fallthrough */
                default:
                    break;
            }
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            _start_sleep_face();
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    _clear_now(state);

    return true;
}

movement_watch_face_advisory_t baby_kicks_face_advise(void *context) {
    movement_watch_face_advisory_t retval = { 0 };
    baby_kicks_state_t *state = (baby_kicks_state_t *)context;

    retval.wants_background_task =
        state->mode == BABY_KICKS_MODE_ACTIVE;

    return retval;
}
