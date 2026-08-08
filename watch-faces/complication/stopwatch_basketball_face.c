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
#include "stopwatch_basketball_face.h"

static const char *result_code(swbb_result_t result) {
    switch (result) {
        case SWBB_RESULT_2PT: return "2P";
        case SWBB_RESULT_3PT: return "3P";
        case SWBB_RESULT_MISS: return "MS";
        case SWBB_RESULT_FOUL: return "FL";
        case SWBB_RESULT_TURNOVER: return "TO";
        case SWBB_RESULT_FT_MAKE: return "F1";
        case SWBB_RESULT_FT_MISS: return "F0";
        default: return "  ";
    }
}

static void reset_game(stopwatch_basketball_state_t *state) {
    memset(state, 0, sizeof(*state));
    swg_timer_reset(&state->timer);
    state->quarter = 1;
    state->phase = SWBB_PHASE_OPEN;
    if (movement_button_should_sound()) {
        watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
    }
}

static void end_possession(stopwatch_basketball_state_t *state) {
    if (state->possessions[state->current_player] < SWBB_POSSESSIONS_PER_QUARTER) {
        state->possessions[state->current_player]++;
    }

    if (state->possessions[0] >= SWBB_POSSESSIONS_PER_QUARTER
            && state->possessions[1] >= SWBB_POSSESSIONS_PER_QUARTER) {
        if (state->quarter < SWBB_QUARTERS) {
            state->quarter++;
            state->possessions[0] = 0;
            state->possessions[1] = 0;
            state->current_player = 0;
            state->phase = SWBB_PHASE_OPEN;
            return;
        }
        state->phase = SWBB_PHASE_OVER;
        return;
    }

    state->current_player ^= 1;
    state->phase = SWBB_PHASE_OPEN;
}

static void resolve_open(stopwatch_basketball_state_t *state, uint8_t digit) {
    if (digit <= 2) {
        state->score[state->current_player] += 2;
        state->last_result = SWBB_RESULT_2PT;
        end_possession(state);
    } else if (digit == 3) {
        state->score[state->current_player] += 3;
        state->last_result = SWBB_RESULT_3PT;
        end_possession(state);
    } else if (digit <= 5) {
        state->last_result = SWBB_RESULT_MISS;
        end_possession(state);
    } else if (digit <= 7) {
        state->last_result = SWBB_RESULT_FOUL;
        state->phase = SWBB_PHASE_FOUL_1;
    } else {
        state->last_result = SWBB_RESULT_TURNOVER;
        end_possession(state);
    }
}

static void resolve_foul_shot(stopwatch_basketball_state_t *state, uint8_t digit) {
    if ((digit % 2) == 0) {
        state->score[state->current_player] += 1;
        state->last_result = SWBB_RESULT_FT_MAKE;
    } else {
        state->last_result = SWBB_RESULT_FT_MISS;
    }

    if (state->phase == SWBB_PHASE_FOUL_1) {
        state->phase = SWBB_PHASE_FOUL_2;
    } else {
        end_possession(state);
    }
}

static void on_timer_stopped(stopwatch_basketball_state_t *state) {
    uint8_t digit = swg_last_digit(swg_timer_elapsed_hundredths(&state->timer));
    if (state->phase == SWBB_PHASE_OPEN) {
        resolve_open(state, digit);
    } else if (state->phase == SWBB_PHASE_FOUL_1 || state->phase == SWBB_PHASE_FOUL_2) {
        resolve_foul_shot(state, digit);
    }
    if (movement_button_should_sound()) {
        watch_buzzer_play_note_with_volume(BUZZER_NOTE_C8, 40, movement_button_volume());
    }
}

static void update_display(stopwatch_basketball_state_t *state) {
    char buf[8];

    watch_clear_display();
    watch_set_colon();

    if (state->phase == SWBB_PHASE_OVER) {
        watch_display_text(WATCH_POSITION_TOP_LEFT, "WN");
        if (state->score[0] == state->score[1]) {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "TI");
        } else if (state->score[0] > state->score[1]) {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "P1");
        } else {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "P2");
        }
        watch_set_indicator(WATCH_INDICATOR_LAP);
    } else {
        sprintf(buf, "P%u", (unsigned)(state->current_player + 1));
        watch_display_text(WATCH_POSITION_TOP_LEFT, buf);
        sprintf(buf, "Q%u", (unsigned)state->quarter);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
        watch_clear_indicator(WATCH_INDICATOR_LAP);
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
    if (state->phase == SWBB_PHASE_FOUL_1) {
        watch_display_text(WATCH_POSITION_SECONDS, "F1");
    } else if (state->phase == SWBB_PHASE_FOUL_2) {
        watch_display_text(WATCH_POSITION_SECONDS, "F2");
    } else {
        watch_display_text(WATCH_POSITION_SECONDS, result_code(state->last_result));
    }
}

void stopwatch_basketball_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(stopwatch_basketball_state_t));
        memset(*context_ptr, 0, sizeof(stopwatch_basketball_state_t));
        stopwatch_basketball_state_t *state = *context_ptr;
        state->quarter = 1;
        state->phase = SWBB_PHASE_OPEN;
    }
}

void stopwatch_basketball_face_activate(void *context) {
    stopwatch_basketball_state_t *state = (stopwatch_basketball_state_t *)context;
    movement_request_tick_frequency(1);
    update_display(state);
}

bool stopwatch_basketball_face_loop(movement_event_t event, void *context) {
    stopwatch_basketball_state_t *state = (stopwatch_basketball_state_t *)context;

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
                reset_game(state);
                movement_request_tick_frequency(1);
                update_display(state);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->phase == SWBB_PHASE_OVER) {
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
            reset_game(state);
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

void stopwatch_basketball_face_resign(void *context) {
    stopwatch_basketball_state_t *state = (stopwatch_basketball_state_t *)context;
    if (state->timer.running) {
        swg_timer_stop(&state->timer);
    }
    movement_request_tick_frequency(1);
}
