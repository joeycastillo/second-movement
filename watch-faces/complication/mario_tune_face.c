/*
 * MIT License
 *
 * Copyright (c) 2026
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

#include "mario_tune_face.h"
#include "watch.h"

static void _mario_tune_face_update_display(mario_tune_face_state_t *state) {
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "MARIO", "MA");
    watch_display_text(WATCH_POSITION_BOTTOM, state->playing ? "PLAY" : "STOP");
}

void mario_tune_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(mario_tune_face_state_t));
        memset(*context_ptr, 0, sizeof(mario_tune_face_state_t));
    }
}

void mario_tune_face_activate(void *context) {
    mario_tune_face_state_t *state = (mario_tune_face_state_t *)context;
    if (state != NULL) {
        watch_set_colon();
        _mario_tune_face_update_display(state);
    }
}

bool mario_tune_face_loop(movement_event_t event, void *context) {
    mario_tune_face_state_t *state = (mario_tune_face_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _mario_tune_face_update_display(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->playing) {
                watch_buzzer_abort_sequence();
                state->playing = false;
            } else {
                movement_play_signal();
                state->playing = true;
            }
            _mario_tune_face_update_display(state);
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void mario_tune_face_resign(void *context) {
    (void)context;
}
