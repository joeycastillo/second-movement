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
#include "stopwatch_blackjack_face.h"

#define SWBJ_POINTS_TO_WIN 3

static void start_player_turn(stopwatch_blackjack_state_t *state) {
    state->bank[state->current_player] = 0;
    state->last_draw = 0;
    state->busted = false;
    state->phase = SWBJ_NEED_DRAW;
}

static void reset_match(stopwatch_blackjack_state_t *state) {
    memset(state, 0, sizeof(*state));
    swg_timer_reset(&state->timer);
    start_player_turn(state);
    if (movement_button_should_sound()) {
        watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
    }
}

static void finish_player(stopwatch_blackjack_state_t *state) {
    uint8_t p = state->current_player;
    state->final_score[p] = state->busted ? 0 : state->bank[p];
    state->players_finished++;

    if (state->players_finished >= 2) {
        if (state->final_score[0] > state->final_score[1]) {
            state->points[0]++;
        } else if (state->final_score[1] > state->final_score[0]) {
            state->points[1]++;
        }
        if (state->points[0] >= SWBJ_POINTS_TO_WIN || state->points[1] >= SWBJ_POINTS_TO_WIN) {
            state->phase = SWBJ_MATCH_OVER;
        } else {
            state->phase = SWBJ_ROUND_OVER;
            state->players_finished = 0;
            state->current_player = 0;
            start_player_turn(state);
        }
        return;
    }

    state->current_player = 1;
    start_player_turn(state);
}

static void apply_draw(stopwatch_blackjack_state_t *state) {
    uint8_t draw = swg_last_two_digits(swg_timer_elapsed_hundredths(&state->timer));
    uint8_t p = state->current_player;
    state->last_draw = draw;
    uint16_t total = (uint16_t)state->bank[p] + draw;
    if (total > 100) {
        state->bank[p] = (uint8_t)total;
        state->busted = true;
        finish_player(state);
    } else {
        state->bank[p] = (uint8_t)total;
        state->phase = SWBJ_DECIDE;
    }
    if (movement_button_should_sound()) {
        watch_buzzer_play_note_with_volume(BUZZER_NOTE_C8, 40, movement_button_volume());
    }
}

static void update_display(stopwatch_blackjack_state_t *state) {
    char buf[8];

    watch_clear_display();
    watch_set_colon();

    if (state->phase == SWBJ_MATCH_OVER) {
        watch_display_text(WATCH_POSITION_TOP_LEFT, "WN");
        if (state->points[0] > state->points[1]) {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "P1");
        } else if (state->points[1] > state->points[0]) {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "P2");
        } else {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "TI");
        }
        watch_set_indicator(WATCH_INDICATOR_LAP);
    } else {
        sprintf(buf, "P%u", (unsigned)(state->current_player + 1));
        watch_display_text(WATCH_POSITION_TOP_LEFT, buf);
        if (state->phase == SWBJ_DECIDE) {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "HS");
        } else if (state->busted) {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "BU");
        } else {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "HT");
        }
        watch_clear_indicator(WATCH_INDICATOR_LAP);
    }

    if (state->timer.running) {
        swg_format_hundredths(swg_timer_elapsed_hundredths(&state->timer), buf);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
        return;
    }

    sprintf(buf, "%02u", (unsigned)state->points[0]);
    watch_display_text(WATCH_POSITION_HOURS, buf);
    sprintf(buf, "%02u", (unsigned)(state->bank[state->current_player] % 100));
    if (state->phase == SWBJ_MATCH_OVER) {
        sprintf(buf, "%02u", (unsigned)state->points[1]);
        watch_display_text(WATCH_POSITION_MINUTES, buf);
    } else {
        watch_display_text(WATCH_POSITION_MINUTES, buf);
    }
    sprintf(buf, "%02u", (unsigned)state->last_draw);
    watch_display_text(WATCH_POSITION_SECONDS, buf);
}

void stopwatch_blackjack_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(stopwatch_blackjack_state_t));
        memset(*context_ptr, 0, sizeof(stopwatch_blackjack_state_t));
        start_player_turn((stopwatch_blackjack_state_t *)*context_ptr);
    }
}

void stopwatch_blackjack_face_activate(void *context) {
    stopwatch_blackjack_state_t *state = (stopwatch_blackjack_state_t *)context;
    movement_request_tick_frequency(1);
    update_display(state);
}

bool stopwatch_blackjack_face_loop(movement_event_t event, void *context) {
    stopwatch_blackjack_state_t *state = (stopwatch_blackjack_state_t *)context;

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
            if (state->phase == SWBJ_DECIDE && !state->timer.running) {
                finish_player(state);
                update_display(state);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->phase == SWBJ_MATCH_OVER) {
                break;
            }
            if (state->timer.running) {
                swg_timer_stop(&state->timer);
                movement_request_tick_frequency(1);
                apply_draw(state);
                update_display(state);
            } else if (state->phase == SWBJ_NEED_DRAW || state->phase == SWBJ_DECIDE) {
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

void stopwatch_blackjack_face_resign(void *context) {
    stopwatch_blackjack_state_t *state = (stopwatch_blackjack_state_t *)context;
    if (state->timer.running) {
        swg_timer_stop(&state->timer);
    }
    movement_request_tick_frequency(1);
}
