/*
 * MIT License
 *
 * Copyright (c) 2023 Bernd Plontsch
 * Copyright (c) 2025 Daniel Bergman
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
#include "breathing_face.h"
#include "watch.h"

typedef struct {
    uint8_t current_stage;
    bool sound_on;
    bool light_on;
    uint8_t led_on_state; // 0 = LED off, 1 = LED on
} breathing_state_t;

static void beep_in (breathing_state_t *state);
static void beep_in_hold (breathing_state_t *state);
static void beep_out (breathing_state_t *state);
static void beep_out_hold (breathing_state_t *state);
static void update_indicators(breathing_state_t *state);

void breathing_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index; // Unused parameter
    if (*context_ptr == NULL) {
        breathing_state_t *state = malloc(sizeof(breathing_state_t));
        state->current_stage = 0;
        state->sound_on = true;  // Default to sound on for new instances
        state->light_on = true;  // Default to light on for new instances
        state->led_on_state = 0;
        *context_ptr = state;
    }
}

void breathing_face_activate(void *context) {
    breathing_state_t *state = (breathing_state_t *)context;
    state->current_stage = 0;
    update_indicators(state);
}

const int NOTE_LENGTH = 80;
const int LED_BLINK_FREQUENCY = 8;

void beep_in(breathing_state_t *state) {
    const watch_buzzer_note_t notes[] = {
        BUZZER_NOTE_C4,
        BUZZER_NOTE_D4,
        BUZZER_NOTE_E4,
    };
    const uint16_t durations[] = {
        NOTE_LENGTH,
        NOTE_LENGTH,
        NOTE_LENGTH
    };
    for(size_t i = 0, count = sizeof(notes) / sizeof(notes[0]); i < count; i++) {
        if (state->light_on) {
            watch_set_led_green();
            state->led_on_state = 1;
        }
        watch_buzzer_play_note(notes[i], durations[i]);
    }
}

void beep_in_hold(breathing_state_t *state) {
    const watch_buzzer_note_t notes[] = {
        BUZZER_NOTE_E4,
        BUZZER_NOTE_REST,
        BUZZER_NOTE_E4,
    };
    const uint16_t durations[] = {
        NOTE_LENGTH,
        NOTE_LENGTH * 2,
        NOTE_LENGTH,
    };
    for(size_t i = 0, count = sizeof(notes) / sizeof(notes[0]); i < count; i++) {
        if (state->light_on && notes[i] != BUZZER_NOTE_REST) {
            watch_set_led_red();
            state->led_on_state = 1;
        }
        watch_buzzer_play_note(notes[i], durations[i]);
    }
}

void beep_out(breathing_state_t *state) {
    const watch_buzzer_note_t notes[] = {
        BUZZER_NOTE_E4,
        BUZZER_NOTE_D4,
        BUZZER_NOTE_C4,
    };
    const uint16_t durations[] = {
        NOTE_LENGTH,
        NOTE_LENGTH,
        NOTE_LENGTH,
    };
    for(size_t i = 0, count = sizeof(notes) / sizeof(notes[0]); i < count; i++) {
        if (state->light_on) {
            watch_set_led_green();
            state->led_on_state = 1;
        }
        watch_buzzer_play_note(notes[i], durations[i]);
    }
}

void beep_out_hold(breathing_state_t *state) {
    const watch_buzzer_note_t notes[] = {
        BUZZER_NOTE_C4,
        BUZZER_NOTE_REST * 2,
        BUZZER_NOTE_C4,
    };
    const uint16_t durations[] = {
        NOTE_LENGTH,
        NOTE_LENGTH,
        NOTE_LENGTH,
    };
    for(size_t i = 0, count = sizeof(notes) / sizeof(notes[0]); i < count; i++) {
        if (state->light_on && notes[i] != BUZZER_NOTE_REST) {
            watch_set_led_red();
            state->led_on_state = 1;
        }
        watch_buzzer_play_note(notes[i], durations[i]);
    }
}

static void update_indicators(breathing_state_t *state) {
    if (state->sound_on) {
        watch_set_indicator(WATCH_INDICATOR_BELL);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_BELL);
    }
    if (state->light_on) {
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    }
}

bool breathing_face_loop(movement_event_t event, void *context) {
    breathing_state_t *state = (breathing_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            if (state->led_on_state == 1) {
                watch_set_led_off();
                state->led_on_state = 0;
            }

            switch (state->current_stage)
            {
            case 0: { watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Breath", "Breath"); if (state->sound_on) beep_in(state); } break;
            case 1: watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "ln   3", "In   3"); break;
            case 2: watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "ln   2", "In   2"); break;
            case 3: watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "ln   1", "In   1"); break;
            
            case 4: { watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Hold 4", "Hold 4"); if (state->sound_on) beep_in_hold(state); } break;
            case 5: watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Hold 3", "Hold 3"); break;
            case 6: watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Hold 2", "Hold 2"); break;               
            case 7: watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Hold 1", "Hold 1"); break;

            case 8: { watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Out  4", "Ou t 4"); if (state->sound_on) beep_out(state); } break;
            case 9: watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Out  3", "Ou t 3"); break;
            case 10: watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Out  2", "Ou t 2"); break;
            case 11: watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Out  1", "Ou t 1"); break;         
            
            case 12: { watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Hold 4", "Hold 4"); if (state->sound_on) beep_out_hold(state); } break;
            case 13: watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Hold 3", "Hold 3"); break;
            case 14: watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Hold 2", "Hold 2"); break;     
            case 15: watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Hold 1", "Hold 1"); break;     
            default:
                break;
            }

            // and increment it so that it will update on the next tick.
            state->current_stage = (state->current_stage + 1) % 16;

            break;
        case EVENT_ALARM_BUTTON_UP:
            state->sound_on = !state->sound_on;
            update_indicators(state);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            state->light_on = !state->light_on;
            update_indicators(state);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            // Override built-in LED function to prevent interference with our LED control
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            // We don't want to go to sleep while we're breathing.
            movement_request_wake();
            break;
        case EVENT_TIMEOUT:
            // We stay in this face until the user chooses to exit
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void breathing_face_resign(void *context) {
    (void) context; // Silence unused parameter warning
    watch_set_led_off();
}
