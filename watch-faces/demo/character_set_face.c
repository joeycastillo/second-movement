/*
 * MIT License
 *
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
#include "character_set_face.h"
#include "watch.h"
#include "watch_common_display.h"

void character_set_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(character_set_state_t));
        character_set_state_t *state = (character_set_state_t *)*context_ptr;
        state->current_char = '@';
        state->quick_ticks_running = false;
    }
}

void character_set_face_activate(void *context) {
    character_set_state_t *state = (character_set_state_t *)context;
    state->current_char = '@';
    state->quick_ticks_running = false;
    movement_request_tick_frequency(0);
}

static void _character_set_face_update_display(char c) {
    char buf[7];
    memset(buf, c, 6);
    buf[6] = '\0';
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, buf, buf);
    watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, buf, buf);
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

bool character_set_face_loop(movement_event_t event, void *context) {
    character_set_state_t *state = (character_set_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _character_set_face_update_display(state->current_char);
            break;

        case EVENT_TICK:
            // Handle fast scroll if quick ticks are running
            if (state->quick_ticks_running) {
                if (HAL_GPIO_BTN_ALARM_read()) {
                    // ALARM button held - scroll forward
                    state->current_char = state->current_char + 1;
                    if (state->current_char & 0x80) state->current_char = ' ';
                } else if (HAL_GPIO_BTN_LIGHT_read()) {
                    // LIGHT button held - scroll backward
                    state->current_char = state->current_char - 1;
                    if (state->current_char < ' ') state->current_char = 0x7F;
                } else {
                    // Button released, stop quick ticks
                    state->quick_ticks_running = false;
                    movement_request_tick_frequency(0);
                    break;
                }

                // Update display
                _character_set_face_update_display(state->current_char);
            }
            break;

        case EVENT_ALARM_BUTTON_UP:
            // Short press - advance one character
            state->current_char = state->current_char + 1;
            if (state->current_char & 0x80) state->current_char = ' ';
            _character_set_face_update_display(state->current_char);
            break;

        case EVENT_ALARM_LONG_PRESS:
            // Start fast scroll forward
            if (!state->quick_ticks_running) {
                state->quick_ticks_running = true;
                movement_request_tick_frequency(8);  // 8 Hz for fast scroll
            }
            break;

        case EVENT_LIGHT_BUTTON_UP:
            // Short press - go back one character
            state->current_char = state->current_char - 1;
            if (state->current_char < ' ') state->current_char = 0x7F;
            _character_set_face_update_display(state->current_char);
            break;

        case EVENT_LIGHT_LONG_PRESS:
            // Start fast scroll backward
            if (!state->quick_ticks_running) {
                state->quick_ticks_running = true;
                movement_request_tick_frequency(8);  // 8 Hz for fast scroll
            }
            break;

        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;

        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void character_set_face_resign(void *context) {
    character_set_state_t *state = (character_set_state_t *)context;
    // Stop quick ticks if active
    if (state->quick_ticks_running) {
        state->quick_ticks_running = false;
        movement_request_tick_frequency(1);
    }
}
