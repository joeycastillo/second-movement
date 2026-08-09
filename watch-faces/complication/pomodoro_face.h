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

#pragma once

#include "movement.h"

/*
 * POMODORO TIMER face
 *
 * Heavily based on the COUNTDOWN face, this faces uses the same base to setup a
 * infinite cycle of a Pomodoro Timer for studying or concentration time.
 *
 * The Initial UI shows the letters POM (or PO in the classic screen) and the
 * numbers of the selected times for the pomodoro session. In the hours section
 * it shows the focus minutes, in the minutes shows the time for the breaks and
 * in the seconds it shows the time for the long break after 4 pomodoros.
 *
 * Buttons:
 * - A long press on the alarm button in this screen changes the minutes for the
 * session.
 * - Pressing the alarm button will initiate the timer with the chosen settings.
 * The words "focus" and "break" (or "FO" and "BR" in the classic screen) will
 * be shown in the screen to know which mode the timer is.
 * - When the timer is running a press in the alarm button will stop the timer.
 * - The light button will reset the face when the timer is stopped.
 *
 * Every time the timer finishes a beep will sound and the next timer will start
 * inmediatly.
 *
 */

typedef enum {
  pomodoro_status_ready,
  pomodoro_status_running,
  pomodoro_status_pause
} pomodoro_status_t;

typedef enum {
  pomodoro_mode_focus,
  pomodoro_mode_break,
  pomodoro_mode_long_break
} pomodoro_mode_t;

typedef struct {
  pomodoro_status_t status;
  pomodoro_mode_t mode;
  uint8_t watch_face_index;
  uint8_t setting;
  uint32_t now_ts;
  uint32_t target_ts;
  uint8_t sec;
  uint8_t min;
  uint8_t count;
} pomodoro_state_t;

void pomodoro_face_setup(uint8_t watch_face_index, void **context_ptr);
void pomodoro_face_activate(void *context);
bool pomodoro_face_loop(movement_event_t event, void *context);
void pomodoro_face_resign(void *context);

#define pomodoro_face                                                          \
  ((const watch_face_t){                                                       \
      pomodoro_face_setup,                                                     \
      pomodoro_face_activate,                                                  \
      pomodoro_face_loop,                                                      \
      pomodoro_face_resign,                                                    \
      NULL,                                                                    \
  })
