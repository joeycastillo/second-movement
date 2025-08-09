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
 * A DESCRIPTION OF YOUR WATCH FACE
 *
 * and a description of how use it
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
