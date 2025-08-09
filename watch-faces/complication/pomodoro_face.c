/*
 * MIT License
 *
 * Copyright (c) 2025 Álvaro Ferrero
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pomodoro_face.h"
#include "watch.h"
#include "watch_utility.h"
#include <stdlib.h>
#include <string.h>

static const uint8_t settings[2][3] = {{1, 1, 2}, {25, 5, 15}};

static void _pomodoro_face_pause_timer(pomodoro_state_t *state) {
  movement_cancel_background_task_for_face(state->watch_face_index);
}

static void _pomodoro_face_start_timer(pomodoro_state_t *state) {
  uint32_t now = watch_utility_date_time_to_unix_time(
      movement_get_utc_date_time(), movement_get_current_timezone_offset());
  state->target_ts =
      watch_utility_offset_timestamp(now, 0, state->min, state->sec);
  state->now_ts = now;
  watch_date_time_t target_dt = watch_utility_date_time_from_unix_time(
      state->target_ts, movement_get_current_timezone_offset());
  movement_schedule_background_task_for_face(state->watch_face_index,
                                             target_dt);
}

static void _pomodoro_face_update_lcd(pomodoro_state_t *state) {
  char buf[9];
  uint32_t delta;
  div_t res;
  char mode[6];
  if (state->status != pomodoro_status_ready) {
    if (state->mode == pomodoro_mode_focus) {
      strcpy(mode, "focus");
    } else if (state->mode == pomodoro_mode_break ||
               state->mode == pomodoro_mode_long_break) {
      strcpy(mode, "break");
    }
    watch_display_text_with_fallback(WATCH_POSITION_TOP, mode, mode);

    delta = state->target_ts - state->now_ts;
    res = div(delta, 60);
    state->sec = res.rem;
    state->min = res.quot;
    sprintf(buf, "00%02d%02d", state->min, state->sec);

    watch_display_text(WATCH_POSITION_BOTTOM, buf);
  } else {
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "POM", "PO");
    char times[7];

    sprintf(times, "%02d%02d%02d", settings[state->setting][0],
            settings[state->setting][1], settings[state->setting][2]);

    watch_display_text(WATCH_POSITION_BOTTOM, times);
  }
}

void pomodoro_face_setup(uint8_t watch_face_index, void **context_ptr) {
  (void)watch_face_index;
  if (*context_ptr == NULL) {
    *context_ptr = malloc(sizeof(pomodoro_state_t));
    memset(*context_ptr, 0, sizeof(pomodoro_state_t));
    pomodoro_state_t *state = (pomodoro_state_t *)*context_ptr;
    state->watch_face_index = watch_face_index;
    state->status = pomodoro_status_ready;
    state->setting = 0;
    // Do any one-time tasks in here; the inside of this conditional happens
    // only at boot.
  }
  // Do any pin or peripheral setup here; this will be called whenever the watch
  // wakes from deep sleep.
}

void pomodoro_face_activate(void *context) {
  pomodoro_state_t *state = (pomodoro_state_t *)context;
  if (state->status == pomodoro_status_running) {
    watch_date_time_t now = movement_get_utc_date_time();
    state->now_ts = watch_utility_date_time_to_unix_time(
        now, movement_get_current_timezone_offset());
  }
  watch_set_colon();
  movement_request_tick_frequency(1);
  // Handle any tasks related to your watch face coming on screen.
}

bool pomodoro_face_loop(movement_event_t event, void *context) {
  pomodoro_state_t *state = (pomodoro_state_t *)context;

  switch (event.event_type) {
  case EVENT_ACTIVATE:
    // Show your initial UI here.
    _pomodoro_face_update_lcd(state);
    break;
  case EVENT_TICK:
    // If needed, update your display here.
    if (state->status == pomodoro_status_running) {
      state->now_ts++;
    }
    _pomodoro_face_update_lcd(state);
    break;
  case EVENT_LIGHT_BUTTON_UP:
    // You can use the Light button for your own purposes. Note that by default,
    // Movement will also illuminate the LED in response to
    // EVENT_LIGHT_BUTTON_DOWN; to suppress that behavior, add an empty case for
    // EVENT_LIGHT_BUTTON_DOWN.
    break;
  case EVENT_ALARM_BUTTON_UP:
    // Just in case you have need for another button.
    if (state->status == pomodoro_status_ready) {
      state->status = pomodoro_status_running;
      state->min = settings[state->setting][0];
      state->mode = pomodoro_mode_focus;
      _pomodoro_face_start_timer(state);
    } else if (state->status == pomodoro_status_running) {
      state->status = pomodoro_status_pause;
      _pomodoro_face_pause_timer(state);
    } else if (state->status == pomodoro_status_pause) {
      state->status = pomodoro_status_running;
      _pomodoro_face_start_timer(state);
    }
    _pomodoro_face_update_lcd(state);
    break;
  case EVENT_BACKGROUND_TASK:
    movement_play_alarm();
    if (state->mode == pomodoro_mode_focus) {
      state->count++;
      if (state->count == 4) {
        state->count = 0;
        state->mode = pomodoro_mode_long_break;
        state->min = settings[state->setting][2];
        _pomodoro_face_start_timer(state);
      } else {
        state->mode = pomodoro_mode_break;
        state->min = settings[state->setting][1];
        _pomodoro_face_start_timer(state);
      }
    } else if (state->mode == pomodoro_mode_break) {
      state->mode = pomodoro_mode_focus;
      state->min = settings[state->setting][0];
      _pomodoro_face_start_timer(state);
    } else if (state->mode == pomodoro_mode_long_break) {
      state->mode = pomodoro_mode_focus;
      state->min = settings[state->setting][1];
      _pomodoro_face_start_timer(state);
    }
    break;
  case EVENT_TIMEOUT:
    // Your watch face will receive this event after a period of inactivity. If
    // it makes sense to resign, you may uncomment this line to move back to the
    // first watch face in the list: movement_move_to_face(0);
    break;
  case EVENT_LOW_ENERGY_UPDATE:
    // If you did not resign in EVENT_TIMEOUT, you can use this event to update
    // the display once a minute. Avoid displaying fast-updating values like
    // seconds, since the display won't update again for 60 seconds. You should
    // also consider starting the tick animation, to show the wearer that this
    // is sleep mode: watch_start_sleep_animation(500);
    break;
  default:
    // Movement's default loop handler will step in for any cases you don't
    // handle above:
    // * EVENT_LIGHT_BUTTON_DOWN lights the LED
    // * EVENT_MODE_BUTTON_UP moves to the next watch face in the list
    // * EVENT_MODE_LONG_PRESS returns to the first watch face (or skips to the
    // secondary watch face, if configured) You can override any of these
    // behaviors by adding a case for these events to this switch statement.
    return movement_default_loop_handler(event);
  }

  // return true if the watch can enter standby mode. Generally speaking, you
  // should always return true.
  return true;
}

void pomodoro_face_resign(void *context) {
  (void)context;

  // handle any cleanup before your watch face goes off-screen.
}
