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
#include "stopwatch_football_face.h"

static const char *result_code(stopwatch_football_result_t result) {
    switch (result) {
        case SOC_RESULT_GOAL: return "GL";
        case SOC_RESULT_PENALTY: return "PN";
        case SOC_RESULT_CORNER: return "CN";
        case SOC_RESULT_FREE_KICK: return "FK";
        case SOC_RESULT_MISS: return "MS";
        case SOC_RESULT_SAVE: return "SV";
        case SOC_RESULT_YELLOW: return "YC";
        case SOC_RESULT_RED: return "RC";
        case SOC_RESULT_OFFSIDE: return "OS";
        case SOC_RESULT_CLEARED: return "CL";
        case SOC_RESULT_BLOCKED: return "BL";
        case SOC_RESULT_PK_GOAL: return "PG";
        case SOC_RESULT_PK_MISS: return "PM";
        default: return "  ";
    }
}

static void reset_match(stopwatch_football_state_t *state) {
    memset(state, 0, sizeof(*state));
    swg_timer_reset(&state->timer);
    state->half = 1;
    state->phase = SOC_PHASE_OPEN;
    if (movement_button_should_sound()) {
        watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
    }
}

static void switch_player(stopwatch_football_state_t *state) {
    state->current_player ^= 1;
}

static bool half_complete(const stopwatch_football_state_t *state) {
    return state->turns[0] >= STOPWATCH_FOOTBALL_TURNS_PER_HALF
        && state->turns[1] >= STOPWATCH_FOOTBALL_TURNS_PER_HALF;
}

static bool shootout_decided(const stopwatch_football_state_t *state) {
    uint8_t rem0 = STOPWATCH_FOOTBALL_PK_ATTEMPTS - state->pk_taken[0];
    uint8_t rem1 = STOPWATCH_FOOTBALL_PK_ATTEMPTS - state->pk_taken[1];
    if (state->pk_made[0] > state->pk_made[1] + rem1) {
        return true;
    }
    if (state->pk_made[1] > state->pk_made[0] + rem0) {
        return true;
    }
    return state->pk_taken[0] >= STOPWATCH_FOOTBALL_PK_ATTEMPTS
        && state->pk_taken[1] >= STOPWATCH_FOOTBALL_PK_ATTEMPTS;
}

static void after_possession(stopwatch_football_state_t *state, bool count_turn) {
    if (count_turn && state->phase != SOC_PHASE_SHOOTOUT) {
        if (state->turns[state->current_player] < STOPWATCH_FOOTBALL_TURNS_PER_HALF) {
            state->turns[state->current_player]++;
        }
    }

    if (state->phase == SOC_PHASE_SHOOTOUT) {
        switch_player(state);
        if (shootout_decided(state)) {
            state->phase = SOC_PHASE_MATCH_OVER;
        }
        return;
    }

    if (half_complete(state)) {
        if (state->half == 1) {
            state->half = 2;
            state->turns[0] = 0;
            state->turns[1] = 0;
            state->current_player = 0;
            state->phase = SOC_PHASE_OPEN;
            return;
        }
        if (state->score[0] == state->score[1]) {
            state->phase = SOC_PHASE_SHOOTOUT;
            state->pk_made[0] = 0;
            state->pk_made[1] = 0;
            state->pk_taken[0] = 0;
            state->pk_taken[1] = 0;
            state->current_player = 0;
            state->last_result = SOC_RESULT_NONE;
            return;
        }
        state->phase = SOC_PHASE_MATCH_OVER;
        return;
    }

    switch_player(state);
    state->phase = SOC_PHASE_OPEN;
}

static void score_goal(stopwatch_football_state_t *state) {
    state->score[state->current_player]++;
    state->last_result = SOC_RESULT_GOAL;
}

static void resolve_open_digit(stopwatch_football_state_t *state, uint8_t digit) {
    switch (digit) {
        case 0:
        case 9:
            score_goal(state);
            after_possession(state, true);
            break;
        case 1:
            state->phase = SOC_PHASE_PENALTY;
            state->last_result = SOC_RESULT_PENALTY;
            break;
        case 2:
            state->phase = SOC_PHASE_CORNER;
            state->last_result = SOC_RESULT_CORNER;
            break;
        case 3:
            state->phase = SOC_PHASE_FREE_KICK;
            state->last_result = SOC_RESULT_FREE_KICK;
            break;
        case 4:
            state->last_result = SOC_RESULT_MISS;
            after_possession(state, true);
            break;
        case 5:
            state->last_result = SOC_RESULT_SAVE;
            after_possession(state, true);
            break;
        case 6:
            state->last_result = SOC_RESULT_YELLOW;
            after_possession(state, true);
            break;
        case 7:
            state->last_result = SOC_RESULT_RED;
            if (state->turns[state->current_player] < STOPWATCH_FOOTBALL_TURNS_PER_HALF) {
                state->turns[state->current_player]++;
            }
            switch_player(state);
            state->phase = SOC_PHASE_RED_PK;
            break;
        case 8:
            state->last_result = SOC_RESULT_OFFSIDE;
            after_possession(state, true);
            break;
        default:
            break;
    }
}

static void check_half_or_match_end(stopwatch_football_state_t *state) {
    if (!half_complete(state)) {
        return;
    }
    if (state->half == 1) {
        state->half = 2;
        state->turns[0] = 0;
        state->turns[1] = 0;
        state->current_player = 0;
        state->phase = SOC_PHASE_OPEN;
        return;
    }
    if (state->score[0] == state->score[1]) {
        state->phase = SOC_PHASE_SHOOTOUT;
        state->pk_made[0] = 0;
        state->pk_made[1] = 0;
        state->pk_taken[0] = 0;
        state->pk_taken[1] = 0;
        state->current_player = 0;
        state->last_result = SOC_RESULT_NONE;
        return;
    }
    state->phase = SOC_PHASE_MATCH_OVER;
}

static void resolve_set_piece(stopwatch_football_state_t *state, uint8_t digit) {
    bool goal = false;
    bool red_pk = (state->phase == SOC_PHASE_RED_PK);

    if (state->phase == SOC_PHASE_PENALTY || red_pk) {
        goal = (digit % 2) == 0;
        state->last_result = goal ? SOC_RESULT_PK_GOAL : SOC_RESULT_PK_MISS;
    } else if (state->phase == SOC_PHASE_CORNER) {
        goal = (digit >= 1 && digit <= 3);
        state->last_result = goal ? SOC_RESULT_GOAL : SOC_RESULT_CLEARED;
    } else if (state->phase == SOC_PHASE_FREE_KICK) {
        goal = (digit == 1 || digit == 2);
        state->last_result = goal ? SOC_RESULT_GOAL : SOC_RESULT_BLOCKED;
    }

    if (goal) {
        state->score[state->current_player]++;
    }

    if (red_pk) {
        if (state->turns[state->current_player] < STOPWATCH_FOOTBALL_TURNS_PER_HALF) {
            state->turns[state->current_player]++;
        }
        switch_player(state);
        state->phase = SOC_PHASE_OPEN;
        check_half_or_match_end(state);
        return;
    }

    after_possession(state, true);
}

static void resolve_shootout(stopwatch_football_state_t *state, uint8_t digit) {
    bool goal = (digit % 2) == 0;
    uint8_t p = state->current_player;
    state->pk_taken[p]++;
    if (goal) {
        state->pk_made[p]++;
        state->score[p]++;
        state->last_result = SOC_RESULT_PK_GOAL;
    } else {
        state->last_result = SOC_RESULT_PK_MISS;
    }
    after_possession(state, false);
}

static void on_timer_stopped(stopwatch_football_state_t *state) {
    uint32_t hundredths = swg_timer_elapsed_hundredths(&state->timer);
    uint8_t digit = swg_last_digit(hundredths);

    if (state->phase == SOC_PHASE_SHOOTOUT) {
        resolve_shootout(state, digit);
    } else if (state->phase == SOC_PHASE_OPEN) {
        resolve_open_digit(state, digit);
    } else if (state->phase == SOC_PHASE_PENALTY
            || state->phase == SOC_PHASE_CORNER
            || state->phase == SOC_PHASE_FREE_KICK
            || state->phase == SOC_PHASE_RED_PK) {
        resolve_set_piece(state, digit);
    }

    if (movement_button_should_sound()) {
        watch_buzzer_play_note_with_volume(BUZZER_NOTE_C8, 40, movement_button_volume());
    }
}

static void update_display(stopwatch_football_state_t *state) {
    char buf[8];

    watch_clear_display();
    watch_set_colon();

    if (state->phase == SOC_PHASE_MATCH_OVER) {
        watch_display_text(WATCH_POSITION_TOP_LEFT, "WN");
        if (state->score[0] == state->score[1]) {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "FT");
        } else if (state->score[0] > state->score[1]) {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "P1");
        } else {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "P2");
        }
    } else if (state->phase == SOC_PHASE_SHOOTOUT) {
        sprintf(buf, "P%u", (unsigned)(state->current_player + 1));
        watch_display_text(WATCH_POSITION_TOP_LEFT, buf);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "PK");
    } else {
        sprintf(buf, "P%u", (unsigned)(state->current_player + 1));
        watch_display_text(WATCH_POSITION_TOP_LEFT, buf);
        sprintf(buf, "H%u", (unsigned)state->half);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }

    if (state->timer.running) {
        swg_format_hundredths(swg_timer_elapsed_hundredths(&state->timer), buf);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
        return;
    }

    sprintf(buf, "%02u", (unsigned)state->score[0]);
    watch_display_text(WATCH_POSITION_HOURS, buf);
    sprintf(buf, "%02u", (unsigned)state->score[1]);
    watch_display_text(WATCH_POSITION_MINUTES, buf);
    watch_display_text(WATCH_POSITION_SECONDS, result_code(state->last_result));

    if (state->phase == SOC_PHASE_MATCH_OVER) {
        watch_set_indicator(WATCH_INDICATOR_LAP);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_LAP);
    }
}

void stopwatch_football_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(stopwatch_football_state_t));
        memset(*context_ptr, 0, sizeof(stopwatch_football_state_t));
        stopwatch_football_state_t *state = *context_ptr;
        state->half = 1;
        state->phase = SOC_PHASE_OPEN;
    }
}

void stopwatch_football_face_activate(void *context) {
    stopwatch_football_state_t *state = (stopwatch_football_state_t *)context;
    movement_request_tick_frequency(1);
    update_display(state);
}

bool stopwatch_football_face_loop(movement_event_t event, void *context) {
    stopwatch_football_state_t *state = (stopwatch_football_state_t *)context;

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
            if (state->phase == SOC_PHASE_MATCH_OVER) {
                break;
            }
            if (state->timer.running) {
                swg_timer_stop(&state->timer);
                movement_request_tick_frequency(1);
                on_timer_stopped(state);
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

void stopwatch_football_face_resign(void *context) {
    stopwatch_football_state_t *state = (stopwatch_football_state_t *)context;
    if (state->timer.running) {
        swg_timer_stop(&state->timer);
    }
    movement_request_tick_frequency(1);
}
