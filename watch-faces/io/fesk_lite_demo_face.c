/*
 * MIT License
 *
 * Copyright (c) 2025
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to do so, subject to the
 * following conditions:
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

#include "fesk_lite_demo_face.h"
#include "fesk_session.h"
#include "watch.h"

typedef struct {
    fesk_session_t session;
} fesk_lite_state_t;

static const char _fesk_lite_message[] = "test";

static void _fesk_lite_on_ready(void *user_data) {
    (void)user_data;
    watch_display_text(WATCH_POSITION_BOTTOM, " TEST ");
}

void fesk_lite_demo_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr != NULL) {
        return;
    }

    fesk_lite_state_t *state = malloc(sizeof(fesk_lite_state_t));
    if (!state) {
        return;
    }

    fesk_session_config_t config = fesk_session_config_defaults();
    config.static_message = _fesk_lite_message;
    config.on_transmission_end = _fesk_lite_on_ready;
    config.on_cancelled = _fesk_lite_on_ready;
    config.user_data = state;

    fesk_session_init(&state->session, &config);
    *context_ptr = state;
}

void fesk_lite_demo_face_activate(void *context) {
    fesk_lite_state_t *state = (fesk_lite_state_t *)context;
    if (!state) {
        return;
    }

    // Session is ready, no preparation needed
}

bool fesk_lite_demo_face_loop(movement_event_t event, void *context) {
    fesk_lite_state_t *state = (fesk_lite_state_t *)context;
    if (!state) {
        return true;
    }

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            break;

        case EVENT_ALARM_BUTTON_UP:
            if (fesk_session_is_idle(&state->session)) {
                fesk_session_start(&state->session);
            }
            break;

        case EVENT_MODE_BUTTON_UP:
            if (fesk_session_is_idle(&state->session)) {
                movement_move_to_next_face();
            }
            break;

        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void fesk_lite_demo_face_resign(void *context) {
    fesk_lite_state_t *state = (fesk_lite_state_t *)context;
    if (!state) {
        return;
    }

    fesk_session_cancel(&state->session);
}
