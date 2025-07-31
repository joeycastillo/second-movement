/*
 * MIT License
 *
 * Copyright (c) 2022 Wesley Ellis
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
#include "stopwatch_face.h"
#include "watch.h"
#include "watch_utility.h"

// distant future for background task: January 1, 2083
// see stopwatch_face_activate for details
static const watch_date_time_t distant_future = {
    .unit = {0, 0, 0, 1, 1, 63}
};

void stopwatch_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(stopwatch_state_t));
        memset(*context_ptr, 0, sizeof(stopwatch_state_t));
    }
}

static void _stopwatch_face_update_display(stopwatch_state_t *stopwatch_state, bool show_seconds) {
    if (stopwatch_state->running) {
        watch_date_time_t now = watch_rtc_get_date_time();
        uint32_t now_timestamp = watch_utility_date_time_to_unix_time(now, 0);
        uint32_t start_timestamp = watch_utility_date_time_to_unix_time(stopwatch_state->start_time, 0);
        stopwatch_state->seconds_counted = now_timestamp - start_timestamp;
    }

    if (stopwatch_state->seconds_counted >= 3456000) {
        // display maxes out just shy of 40 days, thanks to the limit on the day digits (0-39)
        stopwatch_state->running = false;
        movement_cancel_background_task();
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "39");
        watch_display_text(WATCH_POSITION_BOTTOM, "235959");
        return;
    }

    watch_duration_t duration = watch_utility_seconds_to_duration(stopwatch_state->seconds_counted);
    char buf[14];

    sprintf(buf, "%02d%02d  ", duration.hours, duration.minutes);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);

    if (duration.days != 0) {
        sprintf(buf, "%2d", (uint8_t)duration.days);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }

    if (show_seconds) {
        sprintf(buf, "%02d", duration.seconds);
        watch_display_text(WATCH_POSITION_SECONDS, buf);
    }
}

void stopwatch_face_activate(void *context) {
    if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();

    stopwatch_state_t *stopwatch_state = (stopwatch_state_t *)context;
    if (stopwatch_state->running) {
        // because the low power update happens on the minute mark, and the wearer could start
        // the stopwatch anytime, the low power update could fire up to 59 seconds later than
        // we need it to, causing the stopwatch to display stale data.
        // So let's schedule a background task that will never fire. This will keep the watch
        // from entering low energy mode while the stopwatch is on screen. This background task
        // will remain scheduled until the stopwatch stops OR this watch face resigns.
        movement_schedule_background_task(distant_future);
    }
}

bool stopwatch_face_loop(movement_event_t event, void *context) {
    stopwatch_state_t *stopwatch_state = (stopwatch_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_set_colon();
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "STW", "ST");
            // fall through
        case EVENT_TICK:
            if (stopwatch_state->start_time.reg == 0) {
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
                watch_display_text(WATCH_POSITION_BOTTOM, "000000");
            } else {
                _stopwatch_face_update_display(stopwatch_state, true);
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            movement_illuminate_led();
            if (!stopwatch_state->running) {
                stopwatch_state->start_time.reg = 0;
                stopwatch_state->seconds_counted = 0;
                watch_display_text(WATCH_POSITION_BOTTOM, "000000");
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
            }
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            if (movement_button_should_sound()) {
                watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
            }
            stopwatch_state->running = !stopwatch_state->running;
            if (stopwatch_state->running) {
                // we're running now, so we need to set the start_time.
                if (stopwatch_state->start_time.reg == 0) {
                    // if starting from the reset state, easy: we start now.
                    stopwatch_state->start_time = watch_rtc_get_date_time();
                } else {
                    // if resuming with time already on the clock, the original start time isn't valid anymore!
                    // so let's fetch the current time...
                    uint32_t timestamp = watch_utility_date_time_to_unix_time(watch_rtc_get_date_time(), 0);
                    // ...subtract the seconds we've already counted...
                    timestamp -= stopwatch_state->seconds_counted;
                    // and resume from the "virtual" start time that's that many seconds ago.
                    stopwatch_state->start_time = watch_utility_date_time_from_unix_time(timestamp, 0);
                }
                // schedule our keepalive task when running...
                movement_schedule_background_task(distant_future);
            } else {
                // and cancel it when stopped.
                movement_cancel_background_task();
            }
            break;
        case EVENT_TIMEOUT:
            // explicitly ignore the timeout event so we stay on screen
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            if (!watch_sleep_animation_is_running()) watch_start_sleep_animation(1000);
            if (!stopwatch_state->running) {
                // since the tick animation is running, displaying the stopped time could be misleading,
                // as it could imply that the stopwatch is running. instead, show a blank display to
                // indicate that we are in sleep mode.
                watch_display_text(WATCH_POSITION_BOTTOM, "----  ");
            } else {
                // this OTOH shouldn't happen anymore; if we're running, we shouldn't enter low energy mode
                _stopwatch_face_update_display(stopwatch_state, false);
                watch_set_indicator(WATCH_INDICATOR_BELL);
            }
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void stopwatch_face_resign(void *context) {
    (void) context;

    // regardless of whether we're running or stopped, cancel the task
    // that was keeping us awake while on screen.
    movement_cancel_background_task();
}
