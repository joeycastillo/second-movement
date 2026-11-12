/*
 * MIT License
 *
 * Copyright (c) 2025 second-movement contributors
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zodiac_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "zodiac.h"

// Index of the sign the Sun is in now.
static uint8_t get_current_zodiac_sign(void) {
    return zodiac_sign_at((int64_t)movement_get_utc_timestamp());
}

void zodiac_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(zodiac_face_state_t));
        if (*context_ptr == NULL) return;
        memset(*context_ptr, 0, sizeof(zodiac_face_state_t));
    }
}

static void _zodiac_face_display(zodiac_face_state_t *state) {
    // Display the sign and the local date the Sun enters it, nearest to now
    const char *current_sign = zodiac_signs[state->current_sign_index].name;
    uint8_t start_month, start_day;
    zodiac_sign_start_date(state->current_sign_index, (int64_t)movement_get_utc_timestamp(),
                           movement_get_current_timezone_offset(), &start_month, &start_day);

    char bottom[9];
    snprintf(bottom, sizeof(bottom), "%02u%02uSt", start_month, start_day);

    watch_display_text_with_fallback(WATCH_POSITION_TOP, current_sign, zodiac_signs[state->current_sign_index].abbreviation);
    watch_display_text(WATCH_POSITION_BOTTOM, bottom);
}

void zodiac_face_activate(void *context) {
    if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();

    zodiac_face_state_t *state = (zodiac_face_state_t *)context;
    if (state == NULL) // setup's malloc failed; the loop shows the error
        return;

    // Setup runs once at boot, before the clock is set, so the sign must be
    // recomputed here or the face would show the boot-time sign forever.
    state->current_sign_index = get_current_zodiac_sign();
}

bool zodiac_face_loop(movement_event_t event, void *context) {
    zodiac_face_state_t *state = (zodiac_face_state_t *)context;

    // setup's malloc failed: show it, but let MODE still leave the face
    if (state == NULL) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "Zodia", "ZC");
        watch_display_text(WATCH_POSITION_BOTTOM, "Error");
        return movement_default_loop_handler(event);
    }

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _zodiac_face_display(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            // Move to the next zodiac sign
            state->current_sign_index = (state->current_sign_index + 1) % ZODIAC_SIGN_COUNT;
            _zodiac_face_display(state);
            break;

        case EVENT_LOW_ENERGY_UPDATE:
            if (!watch_sleep_animation_is_running())
                watch_start_sleep_animation(1000);
            break;

        case EVENT_LIGHT_BUTTON_UP:
            // Move to the previous zodiac sign
            if (state->current_sign_index == 0) {
                state->current_sign_index = ZODIAC_SIGN_COUNT - 1;
            } else {
                state->current_sign_index--;
            }
            _zodiac_face_display(state);
            break;

        case EVENT_LIGHT_LONG_PRESS:
            movement_illuminate_led();
            break;

        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;

        case EVENT_ALARM_BUTTON_DOWN:
        case EVENT_ALARM_LONG_PRESS:
        case EVENT_LIGHT_BUTTON_DOWN:
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void zodiac_face_resign(void *context) {
    (void)context;
}
