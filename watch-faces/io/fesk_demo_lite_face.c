/*
 * MIT License
 *
 * Copyright (c) 2025 Eirik S. Morland
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
#include "fesk_demo_lite_face.h"

#include "fesk_session.h"

void fesk_demo_lite_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(fesk_demo_lite_state_t));
        memset(*context_ptr, 0, sizeof(fesk_demo_lite_state_t));
    }
}

void fesk_demo_lite_face_activate(void *context) {
    fesk_demo_lite_state_t *state = (fesk_demo_lite_state_t *)context;
}

bool fesk_demo_lite_face_loop(movement_event_t event, void *context) {
    fesk_demo_lite_state_t *state = (fesk_demo_lite_state_t *)context;

    switch (event.event_type) {
        case EVENT_ALARM_BUTTON_UP:
            {
                fesk_session_config_t config = fesk_session_config_defaults();
                config.static_message = "test";
                fesk_session_init(&state->session, &config);
                fesk_session_start(&state->session);
            }
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return fesk_session_is_idle(&state->session);
}

void fesk_demo_lite_face_resign(void *context) {
    (void) context;
}

