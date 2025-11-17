/*
 * MIT License
 *
 * Copyright (c) 2025 Joey Castillo
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
#include "settings_face.h"
#include "watch.h"

typedef struct {
    const char *string;
    const char *fallback;
} text_t;

static const text_t mode_names[MOVEMENT_NUM_TUNE_MODES] = {
    [MOVEMENT_TUNE_MODE_CHIME] = {
        .string = "CHM",
        .fallback = "CH",
    },
    [MOVEMENT_TUNE_MODE_ALARM] = {
        .string = "ALM",
        .fallback = "AL",
    },
    [MOVEMENT_TUNE_MODE_TIMER] = {
        .string = "TIMer",
        .fallback = "CD",
    },
};

static size_t mode_index = 0;
static size_t tune_index = 0;

static void find_tune_index(void) {
    const int8_t *ptr = movement_selected_signal_tunes[mode_index];
    for (tune_index = 0; movement_signal_tunes[tune_index]; ++tune_index) {
        if (movement_signal_tunes[tune_index] == ptr) {
            return;
        }
    }
    tune_index = 0;
    return;
}

static void advance_mode(void) {
    mode_index += 1;
    if (mode_index == MOVEMENT_NUM_TUNE_MODES) {
        mode_index = 0;
    }
    find_tune_index();
}

static void advance_tune(void) {
    tune_index += 1;
    if (!movement_signal_tunes[tune_index]) {
        tune_index = 0;
    }
    movement_selected_signal_tunes[mode_index] = movement_signal_tunes[tune_index];
}

static void play_tune(void) {
    movement_play_signal_tune(mode_index);
}

static void display(void) {
    watch_clear_display();

    // Top: mode name
    watch_display_text_with_fallback(WATCH_POSITION_TOP, mode_names[mode_index].string, mode_names[mode_index].fallback);

    // Bottom: signal tune index
    char buf[3];
    snprintf(buf, sizeof(buf), "%02lu", tune_index + 1);
    watch_display_text(WATCH_POSITION_MINUTES, buf);
}

void signal_tunes_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    (void) context_ptr;
}

void signal_tunes_face_activate(void *context) {
    (void) context;

    mode_index = 0;
    find_tune_index();
}

bool signal_tunes_face_loop(movement_event_t event, void *context) {
    settings_state_t *state = (settings_state_t *)context;

    switch (event.event_type) {
        case EVENT_LIGHT_BUTTON_DOWN:
            advance_mode();
            // fall through
        case EVENT_TICK:
        case EVENT_ACTIVATE:
            display();
            break;
        case EVENT_ALARM_BUTTON_UP:
            advance_tune();
            display();
            play_tune();
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void signal_tunes_face_resign(void *context) {
    (void) context;
}
