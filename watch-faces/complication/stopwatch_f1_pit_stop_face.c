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
#include "stopwatch_f1_pit_stop_face.h"

static void reset_match(stopwatch_f1_pit_stop_state_t *state) {
    memset(state, 0, sizeof(*state));
    swg_timer_reset(&state->timer);
    if (movement_button_should_sound()) {
        watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
    }
}

static uint16_t average_for(const stopwatch_f1_pit_stop_state_t *state, uint8_t player) {
    uint32_t sum = 0;
    uint8_t n = state->attempt[player];
    if (n == 0) {
        return 0;
    }
    for (uint8_t i = 0; i < n; i++) {
        sum += state->times[player][i];
    }
    return (uint16_t)(sum / n);
}

static void on_stop(stopwatch_f1_pit_stop_state_t *state) {
    uint8_t p = state->current_player;
    uint8_t idx = state->attempt[p];
    uint32_t hundredths = swg_timer_elapsed_hundredths(&state->timer);
    if (hundredths > 9999) {
        hundredths = 9999;
    }
    state->last_time = (uint16_t)hundredths;
    if (idx < STOPWATCH_F1_ATTEMPTS) {
        state->times[p][idx] = state->last_time;
        state->attempt[p]++;
    }

    if (state->attempt[0] >= STOPWATCH_F1_ATTEMPTS && state->attempt[1] >= STOPWATCH_F1_ATTEMPTS) {
        state->match_over = true;
    } else if (state->attempt[p] >= STOPWATCH_F1_ATTEMPTS) {
        state->current_player ^= 1;
    } else {
        state->current_player ^= 1;
        if (state->attempt[state->current_player] >= STOPWATCH_F1_ATTEMPTS) {
            state->current_player ^= 1;
        }
    }

    if (movement_button_should_sound()) {
        watch_buzzer_play_note_with_volume(BUZZER_NOTE_C8, 40, movement_button_volume());
    }
}

static void update_display(stopwatch_f1_pit_stop_state_t *state) {
    char buf[8];

    watch_clear_display();
    watch_set_colon();

    if (state->match_over) {
        uint16_t avg0 = average_for(state, 0);
        uint16_t avg1 = average_for(state, 1);
        watch_display_text(WATCH_POSITION_TOP_LEFT, "WN");
        if (avg0 == avg1) {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "TI");
        } else if (avg0 < avg1) {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "P1");
        } else {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "P2");
        }
        watch_set_indicator(WATCH_INDICATOR_LAP);
        sprintf(buf, "%02u", (unsigned)(avg0 % 100));
        watch_display_text(WATCH_POSITION_HOURS, buf);
        sprintf(buf, "%02u", (unsigned)(avg1 % 100));
        watch_display_text(WATCH_POSITION_MINUTES, buf);
        sprintf(buf, "%02u", (unsigned)(state->last_time % 100));
        watch_display_text(WATCH_POSITION_SECONDS, buf);
        return;
    }

    sprintf(buf, "P%u", (unsigned)(state->current_player + 1));
    watch_display_text(WATCH_POSITION_TOP_LEFT, buf);
    sprintf(buf, "A%u", (unsigned)(state->attempt[state->current_player] + 1));
    if (state->attempt[state->current_player] >= STOPWATCH_F1_ATTEMPTS) {
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "OK");
    } else {
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }
    watch_clear_indicator(WATCH_INDICATOR_LAP);

    if (state->timer.running) {
        swg_format_hundredths(swg_timer_elapsed_hundredths(&state->timer), buf);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
        return;
    }

    uint16_t avg = average_for(state, state->current_player);
    sprintf(buf, "%02u", (unsigned)(avg % 100));
    watch_display_text(WATCH_POSITION_HOURS, buf);
    sprintf(buf, "%02u", (unsigned)(state->last_time / 100 % 100));
    watch_display_text(WATCH_POSITION_MINUTES, buf);
    sprintf(buf, "%02u", (unsigned)(state->last_time % 100));
    watch_display_text(WATCH_POSITION_SECONDS, buf);
}

void stopwatch_f1_pit_stop_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(stopwatch_f1_pit_stop_state_t));
        memset(*context_ptr, 0, sizeof(stopwatch_f1_pit_stop_state_t));
    }
}

void stopwatch_f1_pit_stop_face_activate(void *context) {
    stopwatch_f1_pit_stop_state_t *state = (stopwatch_f1_pit_stop_state_t *)context;
    movement_request_tick_frequency(1);
    update_display(state);
}

bool stopwatch_f1_pit_stop_face_loop(movement_event_t event, void *context) {
    stopwatch_f1_pit_stop_state_t *state = (stopwatch_f1_pit_stop_state_t *)context;

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
            if (state->attempt[state->current_player] >= STOPWATCH_F1_ATTEMPTS) {
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

void stopwatch_f1_pit_stop_face_resign(void *context) {
    stopwatch_f1_pit_stop_state_t *state = (stopwatch_f1_pit_stop_state_t *)context;
    if (state->timer.running) {
        swg_timer_stop(&state->timer);
    }
    movement_request_tick_frequency(1);
}
