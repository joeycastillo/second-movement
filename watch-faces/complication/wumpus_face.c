/*
 * MIT License
 *
 * Copyright (c) 2024 Evgeny Stepanischev
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
#include "wumpus_face.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define WUMPUS_FACE_ROWS 20
#define WUMPUS_FACE_COLS 3
#define WUMPUS_CLEAR_ROOM UINT8_MAX
#define WUMPUS_NUM_PITS 2
#define WUMPUS_NUM_BATS 2
#define WUMPUS_NUM_ARROWS 5
#define WUMPUS_MAX_SHOT_LENGTH 5
#define WUMPUS_MOVE_PROB 75 // Percent chance to move after a completed turn.
#define WUMPUS_TICK_FREQUENCY 4
#define WUMPUS_RESULT_TICKS (3 * WUMPUS_TICK_FREQUENCY)

typedef enum {
    wumpus_hazard_none = 0,
    wumpus_hazard_wumpus = 1 << 0,
    wumpus_hazard_bat = 1 << 1,
    wumpus_hazard_pitfall = 1 << 2,
    wumpus_hazard_arrow = 1 << 3,
} wumpus_hazard_type_t;

typedef enum {
    wumpus_face_shoot,
    wumpus_face_shoot_n,
    wumpus_face_shoot_rooms,
    wumpus_face_go,
    wumpus_face_choosing_room,
    wumpus_face_died,
    wumpus_face_won,
} wumpus_current_action_t;

typedef struct {
    uint8_t player_room;
    int8_t selected_room_n;
    wumpus_current_action_t current_action;
    uint8_t arrows;
    // Bit flags let the Wumpus share a room with a stationary hazard.
    uint8_t hazards[WUMPUS_FACE_ROWS];
    uint8_t hazard_point;
    wumpus_hazard_type_t death_cause;
    bool blink_on;
    uint8_t shots_path_len;
    uint8_t shots_picked;
    uint8_t shots_room;
    uint8_t shots_path[WUMPUS_MAX_SHOT_LENGTH];
    uint8_t result_ticks;
} wumpus_game_state_t;

static const uint8_t cave_map[WUMPUS_FACE_ROWS][WUMPUS_FACE_COLS] = {
    {1, 4, 7},
    {0, 2, 9},
    {1, 3, 11},
    {2, 4, 13},
    {0, 3, 5},
    {4, 6, 14},
    {5, 7, 16},
    {0, 6, 8},
    {7, 9, 17},
    {1, 8, 10},
    {9, 11, 18},
    {2, 10, 12},
    {11, 13, 19},
    {3, 12, 14},
    {5, 13, 15},
    {14, 16, 19},
    {6, 15, 17},
    {8, 16, 18},
    {10, 17, 19},
    {12, 15, 18}
};

static uint8_t _get_rand_num(uint8_t num_values) {
#ifdef __EMSCRIPTEN__
    // No global srand(), and no reseeding when returning to the game.
    return (uint8_t)(emscripten_random() * num_values);
#else
    return arc4random_uniform(num_values);
#endif
}

static bool _game_over(const wumpus_game_state_t *state) {
    return state->current_action == wumpus_face_died || state->current_action == wumpus_face_won;
}

static void _display_room(uint8_t room) {
    char room_str[4] = "  ";
    if (room < WUMPUS_FACE_ROWS) {
        snprintf(room_str, sizeof(room_str), "%2u", (unsigned)(room + 1));
    }
    watch_display_text(WATCH_POSITION_TOP_RIGHT, room_str);
}

static void _display_hazard(wumpus_hazard_type_t hazard) {
    switch (hazard) {
        case wumpus_hazard_wumpus:
            if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
                watch_display_text(WATCH_POSITION_SECONDS, "W ");
            } else {
                // The original LCD needs two digits to draw a wide W.
                watch_display_text(WATCH_POSITION_SECONDS, "UU");
                watch_clear_pixel(1, 4);
                watch_clear_pixel(0, 5);
            }
            break;
        case wumpus_hazard_bat:
            watch_display_text(WATCH_POSITION_SECONDS, "Bt");
            break;
        case wumpus_hazard_pitfall:
            watch_display_text(WATCH_POSITION_SECONDS, "Pt");
            break;
        case wumpus_hazard_arrow:
            watch_display_text(WATCH_POSITION_SECONDS, "Ar");
            break;
        case wumpus_hazard_none:
            watch_display_text(WATCH_POSITION_SECONDS, "  ");
            break;
    }
}

static void _display_hazards(wumpus_game_state_t *state) {
    uint8_t nearby = 0;
    for (size_t i = 0; i < WUMPUS_FACE_COLS; i++) {
        nearby |= state->hazards[cave_map[state->player_room][i]];
    }

    wumpus_hazard_type_t hazards[3];
    uint8_t count = 0;
    if (nearby & wumpus_hazard_wumpus) hazards[count++] = wumpus_hazard_wumpus;
    if (nearby & wumpus_hazard_bat) hazards[count++] = wumpus_hazard_bat;
    if (nearby & wumpus_hazard_pitfall) hazards[count++] = wumpus_hazard_pitfall;

    if (count) {
        // The number of warnings can shrink after moving or shooting a bat.
        state->hazard_point %= count;
        _display_hazard(hazards[state->hazard_point]);
        state->hazard_point = (state->hazard_point + 1) % count;
    } else {
        state->hazard_point = 0;
        _display_hazard(wumpus_hazard_none);
    }
}

static void _display(wumpus_game_state_t *state) {
    char buf[8];
    _display_room(state->player_room);
    watch_clear_colon();

    switch (state->current_action) {
        case wumpus_face_shoot:
            watch_display_text(WATCH_POSITION_BOTTOM, state->blink_on ? "SHOT" : "    ");
            break;
        case wumpus_face_go:
            watch_display_text(WATCH_POSITION_BOTTOM, state->blink_on ? "GO  " : "    ");
            break;
        case wumpus_face_shoot_n:
            snprintf(buf, sizeof(buf), "rn%-2u", (unsigned)state->shots_path_len);
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
            watch_set_colon();
            if (!state->blink_on) watch_display_text(WATCH_POSITION_MINUTES, "  ");
            break;
        case wumpus_face_shoot_rooms:
            snprintf(buf, sizeof(buf), "r%u%-2u", (unsigned)(state->shots_picked + 1), (unsigned)(state->shots_room + 1));
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
            watch_set_colon();
            if (!state->blink_on) watch_display_text(WATCH_POSITION_MINUTES, "  ");
            break;
        case wumpus_face_choosing_room:
            watch_display_text(WATCH_POSITION_BOTTOM, "GO  ");
            _display_room(state->blink_on ? (state->selected_room_n < 0 ? state->player_room :
                cave_map[state->player_room][state->selected_room_n]) : WUMPUS_CLEAR_ROOM);
            break;
        case wumpus_face_died:
            watch_display_text(WATCH_POSITION_BOTTOM, "DIED");
            _display_hazard(state->death_cause);
            return;
        case wumpus_face_won:
            watch_display_text(WATCH_POSITION_BOTTOM, "Great ");
            return;
    }
    _display_hazards(state);
}

static uint8_t _generate_unique(const wumpus_game_state_t *state) {
    uint8_t room;
    do {
        room = _get_rand_num(WUMPUS_FACE_ROWS);
    } while (room == state->player_room || state->hazards[room] != wumpus_hazard_none);
    return room;
}

static void _init_game(wumpus_game_state_t *state) {
    memset(state, 0, sizeof(*state));
    state->current_action = wumpus_face_shoot;
    state->player_room = _get_rand_num(WUMPUS_FACE_ROWS);
    state->selected_room_n = -1;
    state->blink_on = true;
    state->arrows = WUMPUS_NUM_ARROWS;

    for (size_t i = 0; i < WUMPUS_NUM_PITS; i++) {
        state->hazards[_generate_unique(state)] = wumpus_hazard_pitfall;
    }
    for (size_t i = 0; i < WUMPUS_NUM_BATS; i++) {
        state->hazards[_generate_unique(state)] = wumpus_hazard_bat;
    }
    state->hazards[_generate_unique(state)] = wumpus_hazard_wumpus;
}

static void _result_led(const wumpus_game_state_t *state) {
    if (state->current_action == wumpus_face_won) {
        movement_force_led_on(0, 255, 0);
    } else {
        movement_force_led_on(255, 0, 0);
    }
}

static void _finish_game(wumpus_game_state_t *state, wumpus_current_action_t result, wumpus_hazard_type_t cause) {
    state->current_action = result;
    state->death_cause = cause;
    state->result_ticks = WUMPUS_RESULT_TICKS;
    _result_led(state);
}

static bool _wumpus_move(wumpus_game_state_t *state) {
    if (_get_rand_num(100) >= WUMPUS_MOVE_PROB) return false;

    for (size_t i = 0; i < WUMPUS_FACE_ROWS; i++) {
        if (state->hazards[i] & wumpus_hazard_wumpus) {
            uint8_t room = cave_map[i][_get_rand_num(WUMPUS_FACE_COLS)];
            state->hazards[i] &= ~wumpus_hazard_wumpus;
            state->hazards[room] |= wumpus_hazard_wumpus;
            return true;
        }
    }
    return false;
}

static void _complete_turn(wumpus_game_state_t *state) {
    // Only a completed move or a missed shot gives the Wumpus a turn.
    if (_wumpus_move(state) && (state->hazards[state->player_room] & wumpus_hazard_wumpus)) {
        _finish_game(state, wumpus_face_died, wumpus_hazard_wumpus);
    }
    state->hazard_point = 0;
}

static void _go_to_selected_room(wumpus_game_state_t *state) {
    state->current_action = wumpus_face_go;
    if (state->selected_room_n < 0) return; // Selecting the current room cancels the move.

    state->player_room = cave_map[state->player_room][state->selected_room_n];
    uint8_t hazard = state->hazards[state->player_room];
    if (hazard & wumpus_hazard_wumpus) {
        _finish_game(state, wumpus_face_died, wumpus_hazard_wumpus);
    } else if (hazard & wumpus_hazard_bat) {
        _finish_game(state, wumpus_face_died, wumpus_hazard_bat);
    } else if (hazard & wumpus_hazard_pitfall) {
        _finish_game(state, wumpus_face_died, wumpus_hazard_pitfall);
    } else {
        _complete_turn(state);
    }
}

static bool _rooms_connected(uint8_t from, uint8_t to) {
    for (size_t i = 0; i < WUMPUS_FACE_COLS; i++) {
        if (cave_map[from][i] == to) return true;
    }
    return false;
}

static void _shot(wumpus_game_state_t *state) {
    if (state->arrows == 0) {
        _finish_game(state, wumpus_face_died, wumpus_hazard_arrow);
        return;
    }
    state->arrows--;
    uint8_t room = state->player_room;
    for (size_t i = 0; i < state->shots_path_len; i++) {
        uint8_t next = state->shots_path[i];
        if (!_rooms_connected(room, next)) {
            next = cave_map[room][_get_rand_num(WUMPUS_FACE_COLS)];
        }
        room = next;
        if (room == state->player_room) {
            _finish_game(state, wumpus_face_died, wumpus_hazard_arrow);
            return;
        }
        if (state->hazards[room] & wumpus_hazard_wumpus) {
            _finish_game(state, wumpus_face_won, wumpus_hazard_none);
            return;
        }
        state->hazards[room] &= ~wumpus_hazard_bat;
    }
    if (state->arrows == 0) {
        _finish_game(state, wumpus_face_died, wumpus_hazard_arrow);
    } else {
        state->current_action = wumpus_face_shoot;
        _complete_turn(state);
    }
}

static void _cycle_selection(wumpus_game_state_t *state) {
    switch (state->current_action) {
        case wumpus_face_shoot:
        case wumpus_face_go:
            state->current_action = state->current_action == wumpus_face_go ? wumpus_face_shoot : wumpus_face_go;
            break;
        case wumpus_face_shoot_n:
            state->shots_path_len = state->shots_path_len % WUMPUS_MAX_SHOT_LENGTH + 1;
            break;
        case wumpus_face_shoot_rooms:
            if (state->shots_picked) {
                state->shots_room = (state->shots_room + 1) % WUMPUS_FACE_ROWS;
            } else {
                for (size_t i = 0; i < WUMPUS_FACE_COLS; i++) {
                    if (cave_map[state->player_room][i] == state->shots_room) {
                        state->shots_room = cave_map[state->player_room][(i + 1) % WUMPUS_FACE_COLS];
                        break;
                    }
                }
            }
            break;
        case wumpus_face_choosing_room:
            if (++state->selected_room_n >= WUMPUS_FACE_COLS) state->selected_room_n = -1;
            break;
        case wumpus_face_won:
        case wumpus_face_died:
            break;
    }
}

static void _confirm_selection(wumpus_game_state_t *state) {
    switch (state->current_action) {
        case wumpus_face_go:
            state->selected_room_n = -1;
            state->current_action = wumpus_face_choosing_room;
            break;
        case wumpus_face_shoot:
            state->shots_path_len = 1;
            state->shots_picked = 0;
            state->current_action = wumpus_face_shoot_n;
            break;
        case wumpus_face_shoot_n:
            state->shots_room = cave_map[state->player_room][0];
            state->shots_picked = 0;
            state->current_action = wumpus_face_shoot_rooms;
            break;
        case wumpus_face_choosing_room:
            _go_to_selected_room(state);
            break;
        case wumpus_face_shoot_rooms:
            state->shots_path[state->shots_picked++] = state->shots_room;
            state->shots_room = 0;
            if (state->shots_picked == state->shots_path_len) _shot(state);
            break;
        case wumpus_face_won:
        case wumpus_face_died:
            break;
    }
}

void wumpus_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) {
        wumpus_game_state_t *state = calloc(1, sizeof(*state));
        if (state != NULL) _init_game(state);
        *context_ptr = state;
    }
}

void wumpus_face_activate(void *context) {
    wumpus_game_state_t *state = context;
    movement_request_tick_frequency(state ? WUMPUS_TICK_FREQUENCY : 1);
    if (state != NULL) {
        state->blink_on = true;
        if (_game_over(state)) _result_led(state);
    }
}

bool wumpus_face_loop(movement_event_t event, void *context) {
    wumpus_game_state_t *state = context;
    if (state == NULL) {
        watch_display_text(WATCH_POSITION_BOTTOM, "No RAM");
        return movement_default_loop_handler(event);
    }
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "WUM", "WH");
            _display(state);
            break;
        case EVENT_TICK:
            if (_game_over(state)) {
                if (state->result_ticks && --state->result_ticks == 0) {
                    movement_force_led_off();
                    _init_game(state);
                }
            } else {
                state->blink_on = !state->blink_on;
            }
            _display(state);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            _cycle_selection(state);
            state->blink_on = true;
            _display(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            _confirm_selection(state);
            state->blink_on = true;
            _display(state);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
        case EVENT_LIGHT_LONG_PRESS:
        case EVENT_LIGHT_LONG_UP:
            // LIGHT belongs to the game; do not trigger the default LED behavior.
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            // Keep the saved game without waking peripherals in low energy mode.
            break;
        default:
            return movement_default_loop_handler(event);
    }
    // RTC ticks wake us to update the display and switch the result LED off.
    return true;
}

void wumpus_face_resign(void *context) {
    const wumpus_game_state_t *state = context;
    if (state != NULL && _game_over(state)) movement_force_led_off();
    movement_request_tick_frequency(1);
}
