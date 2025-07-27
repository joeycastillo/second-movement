/*
 * MIT License
 *
 * Copyright (c) 2025 Alessandro Genova
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
#include "tunes_face.h"

#include "movement_custom_signal_tunes.h"
#include "movement_custom_alarm_tunes.h"

//
// Private
//

typedef enum {
    TUNES_FACE_ALARM_MODE = 0,
    TUNES_FACE_SIGNAL_MODE,
} tunes_face_mode_t;

typedef struct {
    tunes_face_mode_t mode;
    uint8_t signal_tune_index;
    uint8_t alarm_tune_index;
    bool playing;
} tunes_face_state_t;

static void _tunes_face_update_display(tunes_face_state_t *state) {

    char buf[7] = "      ";
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    watch_clear_indicator(WATCH_INDICATOR_BELL);

    if (state->mode == TUNES_FACE_SIGNAL_MODE) {
        if (state->signal_tune_index == movement_custom_signal_tunes_get_active_tune_index()) {
            watch_set_indicator(WATCH_INDICATOR_BELL);
        }

        watch_display_text(WATCH_POSITION_TOP_LEFT, "SI");
        sprintf(buf, "%2d", state->signal_tune_index);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
        watch_display_text(WATCH_POSITION_BOTTOM, signal_tunes_names[state->signal_tune_index]);
    } else {
        if (state->alarm_tune_index == movement_custom_alarm_tunes_get_active_tune_index()) {
            watch_set_indicator(WATCH_INDICATOR_SIGNAL);
        }

        watch_display_text(WATCH_POSITION_TOP_LEFT, "AL");
        sprintf(buf, "%2d", state->alarm_tune_index);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
        watch_display_text(WATCH_POSITION_BOTTOM, alarm_tunes_names[state->alarm_tune_index]);
    }
}

static void _tunes_face_set_active_tune(tunes_face_state_t *state) {
    if (state->mode == TUNES_FACE_SIGNAL_MODE) {
        movement_custom_signal_tunes_set_active_tune_index(state->signal_tune_index);
    } else {
        movement_custom_alarm_tunes_set_active_tune_index(state->alarm_tune_index);
    }
}

static void _tunes_face_play_tune(tunes_face_state_t *state) {
    if (state->mode == TUNES_FACE_SIGNAL_MODE) {
        movement_play_sequence(signal_tunes[state->signal_tune_index], 1);
    } else {
        movement_play_sequence(alarm_tunes[state->alarm_tune_index], 1);
    }
}

static void _tunes_face_next_tune(tunes_face_state_t *state) {
    if (state->mode == TUNES_FACE_SIGNAL_MODE) {
        state->signal_tune_index++;
        if (state->signal_tune_index >= MOVEMENT_N_SIGNAL_TUNES || !signal_tunes[state->signal_tune_index]) {
            state->signal_tune_index = 0;
        }
    } else {
        state->alarm_tune_index++;
        if (state->alarm_tune_index >= MOVEMENT_N_ALARM_TUNES || !alarm_tunes[state->alarm_tune_index]) {
            state->alarm_tune_index = 0;
        }
    }
}

static void _tunes_face_stop_playing(void) {
    static const int8_t abort_tune[] = {0};
    movement_play_sequence(abort_tune, 1);
}

static void _tunes_face_prev_tune(tunes_face_state_t *state) {
    if (state->mode == TUNES_FACE_SIGNAL_MODE) {
        state->signal_tune_index = (state->signal_tune_index + MOVEMENT_N_SIGNAL_TUNES - 1) % MOVEMENT_N_SIGNAL_TUNES;
        while (!signal_tunes[state->signal_tune_index]) {
            state->signal_tune_index--;
            if (state->signal_tune_index == 0) {
                break;
            }
        }
    } else {
        state->alarm_tune_index = (state->alarm_tune_index + MOVEMENT_N_ALARM_TUNES - 1) % MOVEMENT_N_ALARM_TUNES;
        while (!alarm_tunes[state->alarm_tune_index]) {
            state->alarm_tune_index--;
            if (state->alarm_tune_index == 0) {
                break;
            }
        }
    }
}

static void _tunes_face_switch_mode(tunes_face_state_t *state) {
    if (state->mode == TUNES_FACE_SIGNAL_MODE) {
        state->mode = TUNES_FACE_ALARM_MODE;
    } else {
        state->mode = TUNES_FACE_SIGNAL_MODE;
    }
}

//
// Exported
//

void tunes_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(tunes_face_state_t));
        tunes_face_state_t *state = (tunes_face_state_t *)*context_ptr;
        memset(*context_ptr, 0, sizeof(tunes_face_state_t));
    }
}

void tunes_face_activate(void *context) {
    tunes_face_state_t *state = (tunes_face_state_t *)context;
    state->alarm_tune_index = movement_custom_alarm_tunes_get_active_tune_index();
    state->signal_tune_index = movement_custom_signal_tunes_get_active_tune_index();
}
void tunes_face_resign(void *context) {
    (void) context;
}

bool tunes_face_loop(movement_event_t event, void *context) {
    tunes_face_state_t *state = (tunes_face_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _tunes_face_update_display(state);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            // no light
            break;
        case EVENT_LIGHT_BUTTON_UP:
            _tunes_face_prev_tune(state);
            _tunes_face_update_display(state);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            _tunes_face_switch_mode(state);
            _tunes_face_update_display(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            _tunes_face_next_tune(state);
            _tunes_face_update_display(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            _tunes_face_play_tune(state);
            break;
        case EVENT_ALARM_REALLY_LONG_PRESS:
            _tunes_face_stop_playing();
            _tunes_face_set_active_tune(state);
            _tunes_face_update_display(state);
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}
