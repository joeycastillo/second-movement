/*
 * MIT License
 *
 * Copyright (c) 2026 Giannis Prekas
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
#include <stdio.h>
#include "stopwatch_bullseye_face.h"

static void reset_match(stopwatch_bullseye_state_t *state) {
    memset(state, 0, sizeof(*state));
    swg_timer_reset(&state->timer);
    state->round = 1;
    if (movement_button_should_sound()) {
        watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
    }
}

static uint16_t delta_from_target(uint32_t hundredths) {
    if (hundredths >= SWG_TARGET_10_HUNDREDTHS) {
        return (uint16_t)(hundredths - SWG_TARGET_10_HUNDREDTHS);
    }
    return (uint16_t)(SWG_TARGET_10_HUNDREDTHS - hundredths);
}

static void on_stop(stopwatch_bullseye_state_t *state) {
    uint32_t hundredths = swg_timer_elapsed_hundredths(&state->timer);
    state->last_delta = delta_from_target(hundredths);
    state->total[state->current_player] += state->last_delta;

    if (state->current_player == 1) {
        if (state->round >= STOPWATCH_BULLSEYE_ROUNDS) {
            state->match_over = true;
        } else {
            state->round++;
        }
    }
    state->current_player ^= 1;

    if (movement_button_should_sound()) {
        watch_buzzer_play_note_with_volume(BUZZER_NOTE_C8, 40, movement_button_volume());
    }
}

static void display_hundredths_as_ss_hh(uint16_t hundredths) {
    char buf[4];
    uint16_t capped = hundredths;
    if (capped > 9999) {
        capped = 9999;
    }
    sprintf(buf, "%02u", (unsigned)(capped / 100));
    watch_display_text(WATCH_POSITION_HOURS, buf);
    sprintf(buf, "%02u", (unsigned)(capped % 100));
    watch_display_text(WATCH_POSITION_MINUTES, buf);
}

static void update_display(stopwatch_bullseye_state_t *state) {
    char buf[8];

    watch_clear_display();
    watch_set_colon();

    if (state->match_over) {
        watch_display_text(WATCH_POSITION_TOP_LEFT, "WN");
        if (state->total[0] == state->total[1]) {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "TI");
        } else if (state->total[0] < state->total[1]) {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "P1");
        } else {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "P2");
        }
        watch_set_indicator(WATCH_INDICATOR_LAP);
    } else {
        sprintf(buf, "P%u", (unsigned)(state->current_player + 1));
        watch_display_text(WATCH_POSITION_TOP_LEFT, buf);
        sprintf(buf, "R%u", (unsigned)state->round);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
        watch_clear_indicator(WATCH_INDICATOR_LAP);
    }

    if (state->timer.running) {
        swg_format_hundredths(swg_timer_elapsed_hundredths(&state->timer), buf);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
        return;
    }

    if (state->match_over) {
        uint16_t t0 = state->total[0] > 9999 ? 9999 : state->total[0];
        uint16_t t1 = state->total[1] > 9999 ? 9999 : state->total[1];
        sprintf(buf, "%02u", (unsigned)(t0 / 100));
        watch_display_text(WATCH_POSITION_HOURS, buf);
        sprintf(buf, "%02u", (unsigned)(t1 / 100));
        watch_display_text(WATCH_POSITION_MINUTES, buf);
        watch_display_text(WATCH_POSITION_SECONDS, "Sc");
        return;
    }

    /* Last error from 10.00s, e.g. 761 hundredths -> 07:61 Er (7.61s away). */
    display_hundredths_as_ss_hh(state->last_delta);
    watch_display_text(WATCH_POSITION_SECONDS, "Er");
}

void stopwatch_bullseye_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(stopwatch_bullseye_state_t));
        memset(*context_ptr, 0, sizeof(stopwatch_bullseye_state_t));
        ((stopwatch_bullseye_state_t *)*context_ptr)->round = 1;
    }
}

void stopwatch_bullseye_face_activate(void *context) {
    stopwatch_bullseye_state_t *state = (stopwatch_bullseye_state_t *)context;
    movement_request_tick_frequency(1);
    update_display(state);
}

bool stopwatch_bullseye_face_loop(movement_event_t event, void *context) {
    stopwatch_bullseye_state_t *state = (stopwatch_bullseye_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            update_display(state);
            break;
        case EVENT_TICK:
            if (state->timer.running) {
                update_display(state);
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if (!state->timer.running) {
                reset_match(state);
                movement_request_tick_frequency(1);
                update_display(state);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->match_over) {
                break;
            }
            if (state->timer.running) {
                swg_timer_stop(&state->timer);
                movement_request_tick_frequency(1);
                on_stop(state);
                update_display(state);
            } else {
                swg_timer_start(&state->timer);
                movement_request_tick_frequency(SWG_UI_TICK_HZ);
                update_display(state);
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            reset_match(state);
            movement_request_tick_frequency(1);
            update_display(state);
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void stopwatch_bullseye_face_resign(void *context) {
    stopwatch_bullseye_state_t *state = (stopwatch_bullseye_state_t *)context;
    if (state->timer.running) {
        swg_timer_stop(&state->timer);
    }
    movement_request_tick_frequency(1);
}
