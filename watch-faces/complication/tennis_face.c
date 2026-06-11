/*
 * MIT License
 *
 * Copyright (c) 2026 Alessandro Genova
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
#include "tennis_face.h"

// --- Types ---

typedef enum {
    TENNIS_MODE_MENU,
    TENNIS_MODE_SET,
    TENNIS_MODE_POINTS,
    TENNIS_MODE_CUSTOM_SET,
    TENNIS_MODE_SAVED,
} tennis_mode_t;

typedef enum {
    TENNIS_MENU_SET = 0,
    TENNIS_MENU_POINTS,
    TENNIS_MENU_CUSTOM_SET,
    TENNIS_MENU_SAVED,
    TENNIS_MENU_COUNT,
} tennis_menu_item_t;

typedef enum {
    TENNIS_POINT_0 = 0,
    TENNIS_POINT_15,
    TENNIS_POINT_30,
    TENNIS_POINT_40,
    TENNIS_POINT_AD,
} tennis_point_t;

#define TENNIS_MAX_SAVED_MATCHES 5

typedef enum {
    TENNIS_MATCH_SET,
    TENNIS_MATCH_POINTS,
    TENNIS_MATCH_CUSTOM_SET,
} tennis_match_type_t;

typedef struct {
    tennis_match_type_t type;
    uint8_t p1_points;
    uint8_t p2_points;
    uint8_t p1_games;
    uint8_t p2_games;
    bool p1_serving;
    bool done;
    bool in_tiebreak;
} tennis_match_t;

typedef struct {
    tennis_mode_t mode;
    tennis_menu_item_t menu_selection;

    uint8_t current_match_idx;
    tennis_match_t matches[TENNIS_MAX_SAVED_MATCHES];
    uint8_t matches_head;
    uint8_t matches_count;

    uint8_t saved_view_index;

    bool blink_visible;

    uint8_t tiebreak_trigger_games;
    uint8_t tiebreak_target_points;
} tennis_state_t;

// --- Static data ---

static const char *tennis_point_strings_custom[] = {
    " 0",  // TENNIS_POINT_0
    "15",  // TENNIS_POINT_15
    "30",  // TENNIS_POINT_30
    "40",  // TENNIS_POINT_40
    "Ad",  // TENNIS_POINT_AD
};

static const char *tennis_point_strings_classic[] = {
    " 0",  // TENNIS_POINT_0
    "15",  // TENNIS_POINT_15
    "30",  // TENNIS_POINT_30
    "40",  // TENNIS_POINT_40
    " A",  // TENNIS_POINT_AD
};

static const char **_tennis_point_strings(void) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM)
        return tennis_point_strings_custom;
    return tennis_point_strings_classic;
}

static const char *tennis_menu_labels[] = {
    "SET   ",  // TENNIS_MENU_SET
    "PO1NTS",  // TENNIS_MENU_POINTS
    "CU SET",  // TENNIS_MENU_CUSTOM_SET
    "SAVEd ",  // TENNIS_MENU_SAVED
};

static const char *tennis_menu_labels_classic[] = {
    " SET  ",  // TENNIS_MENU_SET
    " POINT",  // TENNIS_MENU_POINTS
    "CU SET",  // TENNIS_MENU_CUSTOM_SET
    " SAVEd",  // TENNIS_MENU_SAVED
};

static const char *tennis_match_type_labels_custom[] = {"SET", "PTS", "CUS"};
static const char *tennis_match_type_labels_classic[] = {"ST", "PT", "CU"};

// --- Display helpers ---

static void _tennis_display_number(watch_position_t position, uint8_t value, uint8_t max) {
    char buf[3];
    uint8_t v = value > max ? max : value;
    buf[0] = v >= 10 ? ('0' + v / 10) : ' ';
    buf[1] = '0' + (v % 10);
    buf[2] = '\0';
    watch_display_text(position, buf);
}

static tennis_match_t *_tennis_current_match(tennis_state_t *state) {
    return &state->matches[state->current_match_idx];
}

static void _tennis_draw_points_area(tennis_match_t *m) {
    if (m->done) {
        watch_display_text(WATCH_POSITION_BOTTOM, " End");
    } else if (m->in_tiebreak || m->type == TENNIS_MATCH_POINTS) {
        _tennis_display_number(WATCH_POSITION_HOURS, m->p1_points, 99);
        _tennis_display_number(WATCH_POSITION_MINUTES, m->p2_points, 99);
        watch_set_colon();
    } else {
        const char **pt_strings = _tennis_point_strings();
        watch_display_text(WATCH_POSITION_HOURS, pt_strings[m->p1_points]);
        watch_display_text(WATCH_POSITION_MINUTES, pt_strings[m->p2_points]);
        watch_set_colon();
    }
}

// --- Draw functions ---

static void _tennis_draw_menu(tennis_state_t *state) {
    watch_clear_display();
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "TNS", "TE");
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, tennis_menu_labels[state->menu_selection], tennis_menu_labels_classic[state->menu_selection]);
}

static void _tennis_draw_set(tennis_state_t *state) {
    char buf[4];
    tennis_match_t *m = _tennis_current_match(state);

    watch_clear_display();
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT,
        m->in_tiebreak ? "TBR" : "SET",
        m->in_tiebreak ? "TB" : "ST");

    buf[0] = (m->p1_serving && !m->done) ? '`' : ' ';
    buf[1] = '0' + (m->p1_games % 10);
    buf[2] = '\0';
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);

    _tennis_draw_points_area(m);

    buf[0] = (!m->p1_serving && !m->done) ? '`' : ' ';
    buf[1] = '0' + (m->p2_games % 10);
    buf[2] = '\0';
    watch_display_text(WATCH_POSITION_SECONDS, buf);
}

static void _tennis_draw_points(tennis_state_t *state) {
    char buf[4];
    tennis_match_t *m = _tennis_current_match(state);

    watch_clear_display();
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "PTS", "PT");

    buf[0] = ' ';
    buf[1] = m->p1_serving ? 'o' : ' ';
    buf[2] = '\0';
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);

    _tennis_draw_points_area(m);

    buf[0] = ' ';
    buf[1] = m->p1_serving ? ' ' : 'o';
    buf[2] = '\0';
    watch_display_text(WATCH_POSITION_SECONDS, buf);
}

static void _tennis_draw_custom_set(tennis_state_t *state) {
    char buf[4];
    tennis_match_t *m = _tennis_current_match(state);
    bool two_digit_games = (m->p1_games >= 10 || m->p2_games >= 10);

    watch_clear_display();
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT,
        m->in_tiebreak ? "TBR" : "CUS",
        m->in_tiebreak ? "TB" : "CU");

    if (two_digit_games) {
        _tennis_display_number(WATCH_POSITION_TOP_RIGHT, m->p1_games, 39);
        _tennis_display_number(WATCH_POSITION_SECONDS, m->p2_games, 39);
    } else {
        buf[0] = (m->p1_serving && !m->done) ? '`' : ' ';
        buf[1] = '0' + (m->p1_games % 10);
        buf[2] = '\0';
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);

        buf[0] = (!m->p1_serving && !m->done) ? '`' : ' ';
        buf[1] = '0' + (m->p2_games % 10);
        buf[2] = '\0';
        watch_display_text(WATCH_POSITION_SECONDS, buf);
    }

    _tennis_draw_points_area(m);
}

static void _tennis_draw_match_readonly(tennis_match_t *m, const char *top_left_label, const char *top_left_fallback) {
    char buf[4];

    watch_clear_display();
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, top_left_label, top_left_fallback);

    if (m->type != TENNIS_MATCH_POINTS) {
        bool two_digit = (m->p1_games >= 10 || m->p2_games >= 10);
        if (two_digit) {
            _tennis_display_number(WATCH_POSITION_TOP_RIGHT, m->p1_games, 39);
            _tennis_display_number(WATCH_POSITION_SECONDS, m->p2_games, 39);
        } else {
            buf[0] = ' ';
            buf[1] = '0' + (m->p1_games % 10);
            buf[2] = '\0';
            watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
            buf[1] = '0' + (m->p2_games % 10);
            watch_display_text(WATCH_POSITION_SECONDS, buf);
        }
    }

    _tennis_draw_points_area(m);
}

static void _tennis_draw_saved(tennis_state_t *state) {
    if (state->matches_count == 0) return;

    uint8_t buf_idx = (state->matches_head - 1 - state->saved_view_index + TENNIS_MAX_SAVED_MATCHES) % TENNIS_MAX_SAVED_MATCHES;
    tennis_match_t *m = &state->matches[buf_idx];

    char idx_buf[3];
    idx_buf[0] = '0' + (state->saved_view_index + 1);
    idx_buf[1] = ' ';
    idx_buf[2] = '\0';

    if (state->blink_visible) {
        _tennis_draw_match_readonly(m, tennis_match_type_labels_custom[m->type], tennis_match_type_labels_classic[m->type]);
    } else {
        _tennis_draw_match_readonly(m, idx_buf, idx_buf);
    }
}

static void _tennis_update_display(tennis_state_t *state) {
    switch (state->mode) {
        case TENNIS_MODE_MENU:
            _tennis_draw_menu(state);
            break;
        case TENNIS_MODE_SET:
            _tennis_draw_set(state);
            break;
        case TENNIS_MODE_POINTS:
            _tennis_draw_points(state);
            break;
        case TENNIS_MODE_CUSTOM_SET:
            _tennis_draw_custom_set(state);
            break;
        case TENNIS_MODE_SAVED:
            _tennis_draw_saved(state);
            break;
        default:
            break;
    }
}

// --- Match management ---

static void _tennis_reset_match(tennis_match_t *match, tennis_match_type_t type) {
    memset(match, 0, sizeof(tennis_match_t));
    match->type = type;
    match->p1_serving = true;
}

static void _tennis_start_new_match(tennis_state_t *state, tennis_match_type_t type) {
    state->current_match_idx = state->matches_head;
    state->matches_head = (state->matches_head + 1) % TENNIS_MAX_SAVED_MATCHES;
    if (state->matches_count < TENNIS_MAX_SAVED_MATCHES) {
        state->matches_count++;
    }
    _tennis_reset_match(&state->matches[state->current_match_idx], type);
}

// --- Scoring logic ---

static void _tennis_check_set(tennis_state_t *state, tennis_match_t *match) {
    if (match->type != TENNIS_MATCH_SET) return;

    if ((match->p1_games >= 6 || match->p2_games >= 6) &&
        abs(match->p1_games - match->p2_games) >= 2) {
        match->done = true;
    } else if (match->p1_games == state->tiebreak_trigger_games &&
               match->p2_games == state->tiebreak_trigger_games) {
        match->in_tiebreak = true;
        match->p1_points = 0;
        match->p2_points = 0;
    }
}

static void _tennis_win_game(tennis_state_t *state, tennis_match_t *match, uint8_t player) {
    if (player == 1) {
        match->p1_games++;
    } else {
        match->p2_games++;
    }
    match->p1_points = 0;
    match->p2_points = 0;
    match->p1_serving = !match->p1_serving;
    _tennis_check_set(state, match);
}

static void _tennis_award_tb_point(tennis_match_t *match, uint8_t player) {
    if (player == 1) {
        match->p1_points++;
    } else {
        match->p2_points++;
    }

    uint16_t total = match->p1_points + match->p2_points;
    if (total % 2 == 1) {
        match->p1_serving = !match->p1_serving;
    }
}

static void _tennis_award_set_tb_point(tennis_state_t *state, tennis_match_t *match, uint8_t player) {
    _tennis_award_tb_point(match, player);

    // Check if tiebreak is won (only in regular Set mode)
    if (match->type == TENNIS_MATCH_SET &&
        (match->p1_points >= state->tiebreak_target_points ||
         match->p2_points >= state->tiebreak_target_points) &&
        abs(match->p1_points - match->p2_points) >= 2) {
        if (match->p1_points > match->p2_points) {
            match->p1_games++;
        } else {
            match->p2_games++;
        }
        match->in_tiebreak = false;
        match->done = true;
    }
}

static void _tennis_award_point(tennis_state_t *state, tennis_match_t *match, uint8_t player) {
    if (match->done) return;

    if (match->in_tiebreak) {
        _tennis_award_set_tb_point(state, match, player);
        return;
    }

    uint8_t *scorer_pts = (player == 1) ? &match->p1_points : &match->p2_points;
    uint8_t *opponent_pts = (player == 1) ? &match->p2_points : &match->p1_points;

    switch (*scorer_pts) {
        case TENNIS_POINT_0:
            *scorer_pts = TENNIS_POINT_15;
            break;
        case TENNIS_POINT_15:
            *scorer_pts = TENNIS_POINT_30;
            break;
        case TENNIS_POINT_30:
            *scorer_pts = TENNIS_POINT_40;
            break;
        case TENNIS_POINT_40:
            if (*opponent_pts < TENNIS_POINT_40) {
                _tennis_win_game(state, match, player);
            } else if (*opponent_pts == TENNIS_POINT_40) {
                *scorer_pts = TENNIS_POINT_AD;
            } else if (*opponent_pts == TENNIS_POINT_AD) {
                *opponent_pts = TENNIS_POINT_40;
            }
            break;
        case TENNIS_POINT_AD:
            _tennis_win_game(state, match, player);
            break;
    }
}

// --- Common scoring mode event handler ---

static bool _tennis_scoring_mode_common_handler(tennis_state_t *state, tennis_match_t *match, movement_event_t event) {
    switch (event.event_type) {
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_LONG_PRESS:
            state->mode = TENNIS_MODE_MENU;
            _tennis_update_display(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            match->p1_serving = !match->p1_serving;
            _tennis_update_display(state);
            break;
        case EVENT_TIMEOUT:
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            _tennis_update_display(state);
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

// --- Face callbacks ---

void tennis_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(tennis_state_t));
        memset(*context_ptr, 0, sizeof(tennis_state_t));
        tennis_state_t *state = (tennis_state_t *)*context_ptr;
        state->tiebreak_trigger_games = 6;
        state->tiebreak_target_points = 7;
    }
}

void tennis_face_activate(void *context) {
    tennis_state_t *state = (tennis_state_t *)context;
    _tennis_update_display(state);
}

bool tennis_face_loop(movement_event_t event, void *context) {
    tennis_state_t *state = (tennis_state_t *)context;
    tennis_match_t *match = _tennis_current_match(state);

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _tennis_update_display(state);
            return true;
        default:
            break;
    }

    switch (state->mode) {
        case TENNIS_MODE_MENU:
            switch (event.event_type) {
                case EVENT_LIGHT_BUTTON_DOWN:
                    break;
                case EVENT_LIGHT_BUTTON_UP:
                    state->menu_selection = (state->menu_selection + 1) % TENNIS_MENU_COUNT;
                    _tennis_update_display(state);
                    break;
                case EVENT_ALARM_BUTTON_UP:
                    switch (state->menu_selection) {
                        case TENNIS_MENU_SET:
                            _tennis_start_new_match(state, TENNIS_MATCH_SET);
                            state->mode = TENNIS_MODE_SET;
                            break;
                        case TENNIS_MENU_POINTS:
                            _tennis_start_new_match(state, TENNIS_MATCH_POINTS);
                            state->mode = TENNIS_MODE_POINTS;
                            break;
                        case TENNIS_MENU_CUSTOM_SET:
                            _tennis_start_new_match(state, TENNIS_MATCH_CUSTOM_SET);
                            state->mode = TENNIS_MODE_CUSTOM_SET;
                            break;
                        case TENNIS_MENU_SAVED:
                            if (state->matches_count > 0) {
                                state->saved_view_index = 0;
                                state->blink_visible = true;
                                state->mode = TENNIS_MODE_SAVED;
                            }
                            break;
                        default:
                            break;
                    }
                    _tennis_update_display(state);
                    break;
                case EVENT_TIMEOUT:
                    movement_move_to_face(0);
                    break;
                default:
                    return movement_default_loop_handler(event);
            }
            break;

        case TENNIS_MODE_SET:
            switch (event.event_type) {
                case EVENT_LIGHT_BUTTON_UP:
                    if (!match->done) {
                        _tennis_award_point(state, match, 1);
                        _tennis_update_display(state);
                    }
                    break;
                case EVENT_ALARM_BUTTON_UP:
                    if (!match->done) {
                        _tennis_award_point(state, match, 2);
                        _tennis_update_display(state);
                    }
                    break;
                default:
                    return _tennis_scoring_mode_common_handler(state, match, event);
            }
            break;

        case TENNIS_MODE_POINTS:
            switch (event.event_type) {
                case EVENT_LIGHT_BUTTON_UP:
                    _tennis_award_tb_point(match, 1);
                    _tennis_update_display(state);
                    break;
                case EVENT_ALARM_BUTTON_UP:
                    _tennis_award_tb_point(match, 2);
                    _tennis_update_display(state);
                    break;
                default:
                    return _tennis_scoring_mode_common_handler(state, match, event);
            }
            break;

        case TENNIS_MODE_CUSTOM_SET:
            switch (event.event_type) {
                case EVENT_LIGHT_BUTTON_UP:
                    _tennis_award_point(state, match, 1);
                    _tennis_update_display(state);
                    break;
                case EVENT_ALARM_BUTTON_UP:
                    _tennis_award_point(state, match, 2);
                    _tennis_update_display(state);
                    break;
                case EVENT_TICK:
                    if (!match->done && (match->p1_games >= 10 || match->p2_games >= 10)) {
                        state->blink_visible = !state->blink_visible;
                        if (match->p1_serving) {
                            if (state->blink_visible) {
                                _tennis_display_number(WATCH_POSITION_TOP_RIGHT, match->p1_games, 39);
                            } else {
                                watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
                            }
                        } else {
                            if (state->blink_visible) {
                                _tennis_display_number(WATCH_POSITION_SECONDS, match->p2_games, 39);
                            } else {
                                watch_display_text(WATCH_POSITION_SECONDS, "  ");
                            }
                        }
                    }
                    break;
                case EVENT_ALARM_REALLY_LONG_PRESS:
                    if (match->in_tiebreak) {
                        if (match->p1_points != match->p2_points) {
                            match->p1_serving = !match->p1_serving;
                            if (match->p1_points > match->p2_points) {
                                match->p1_games++;
                            } else {
                                match->p2_games++;
                            }
                            match->in_tiebreak = false;
                            match->done = true;
                        }
                    } else if (match->p1_points == 0 &&
                               match->p2_points == 0 &&
                               match->p1_games == match->p2_games) {
                        match->p1_serving = !match->p1_serving;
                        match->in_tiebreak = true;
                        match->p1_points = 0;
                        match->p2_points = 0;
                    }
                    _tennis_update_display(state);
                    break;
                default:
                    return _tennis_scoring_mode_common_handler(state, match, event);
            }
            break;

        case TENNIS_MODE_SAVED:
            switch (event.event_type) {
                case EVENT_LIGHT_BUTTON_DOWN:
                    break;
                case EVENT_LIGHT_BUTTON_UP:
                    state->saved_view_index = (state->saved_view_index + 1) % state->matches_count;
                    state->blink_visible = true;
                    _tennis_update_display(state);
                    break;
                case EVENT_ALARM_BUTTON_UP:
                {
                    uint8_t buf_idx = (state->matches_head - 1 - state->saved_view_index + TENNIS_MAX_SAVED_MATCHES) % TENNIS_MAX_SAVED_MATCHES;
                    tennis_match_t *viewed = &state->matches[buf_idx];
                    if (!viewed->done) {
                        state->current_match_idx = buf_idx;
                        switch (viewed->type) {
                            case TENNIS_MATCH_SET:
                                state->mode = TENNIS_MODE_SET;
                                break;
                            case TENNIS_MATCH_POINTS:
                                state->mode = TENNIS_MODE_POINTS;
                                break;
                            case TENNIS_MATCH_CUSTOM_SET:
                                state->mode = TENNIS_MODE_CUSTOM_SET;
                                break;
                        }
                        _tennis_update_display(state);
                    }
                }
                    break;
                case EVENT_TICK:
                    state->blink_visible = !state->blink_visible;
                    _tennis_update_display(state);
                    break;
                case EVENT_LIGHT_LONG_PRESS:
                    state->mode = TENNIS_MODE_MENU;
                    _tennis_update_display(state);
                    break;
                case EVENT_TIMEOUT:
                    movement_move_to_face(0);
                    break;
                case EVENT_LOW_ENERGY_UPDATE:
                    _tennis_update_display(state);
                    break;
                default:
                    return movement_default_loop_handler(event);
            }
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void tennis_face_resign(void *context) {
    (void) context;
}
