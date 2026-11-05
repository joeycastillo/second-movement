#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Include the implementation so fixtures can arrange otherwise rare cave layouts.
// All UI scenarios below drive the public Movement callbacks.
#include "../../watch-faces/complication/wumpus_face.c"

static char screen[11];
static char title[4];
static bool colon;
static uint8_t led[3];
static uint8_t tick_frequency;
static unsigned pixel_clears;
static int next_face;
static watch_lcd_type_t lcd_type;
static uint32_t random_values[32];
static size_t random_length, random_position, random_calls;
static uint32_t random_seed = 1;

uint32_t arc4random_uniform(uint32_t upper_bound) {
    assert(upper_bound > 0);
    random_calls++;
    if (random_position < random_length) return random_values[random_position++] % upper_bound;
    random_seed = random_seed * 1664525u + 1013904223u;
    return (random_seed >> 8) % upper_bound;
}

static void set_random(const uint32_t *values, size_t length) {
    assert(length <= sizeof(random_values) / sizeof(*random_values));
    memcpy(random_values, values, length * sizeof(*values));
    random_length = length;
    random_position = random_calls = 0;
}
#define RANDOM(...) do { \
    const uint32_t values[] = { __VA_ARGS__ }; \
    set_random(values, sizeof(values) / sizeof(*values)); \
} while (0)

void watch_display_text(watch_position_t location, const char *string) {
    static const uint8_t positions[] = { 0, 0, 0, 2, 4, 4, 6, 8 };
    size_t position = positions[location];
    size_t length = strlen(string);
    if (location == WATCH_POSITION_TOP_LEFT) {
        snprintf(title, sizeof(title), "%s", string);
        if (length > 2) length = 2; // The custom LCD's third letter has its own position.
    }
    // Like Second Movement, partial strings leave the remaining digits intact.
    assert(position + length <= 10);
    memcpy(screen + position, string, length);
}

void watch_display_text_with_fallback(watch_position_t location, const char *string, const char *fallback) {
    watch_display_text(location, lcd_type == WATCH_LCD_TYPE_CUSTOM ? string : fallback);
}
void watch_clear_pixel(uint8_t com, uint8_t seg) { (void)com; (void)seg; pixel_clears++; }
void watch_set_colon(void) { colon = true; }
void watch_clear_colon(void) { colon = false; }
watch_lcd_type_t watch_get_lcd_type(void) { return lcd_type; }
void movement_force_led_on(uint8_t red, uint8_t green, uint8_t blue) {
    led[0] = red; led[1] = green; led[2] = blue;
}
void movement_force_led_off(void) { memset(led, 0, sizeof(led)); }
void movement_request_tick_frequency(uint8_t frequency) { tick_frequency = frequency; }
void movement_move_to_face(uint8_t index) { next_face = index; }
bool movement_default_loop_handler(movement_event_t event) {
    if (event.event_type == EVENT_MODE_BUTTON_UP) next_face = 1;
    if (event.event_type == EVENT_MODE_LONG_PRESS) next_face = 0;
    return true;
}

static void send_event(void *context, movement_event_type_t type) {
    assert(wumpus_face_loop((movement_event_t){ .event_type = type }, context));
}

static void reset_boundary(void) {
    memset(screen, ' ', 10);
    screen[10] = '\0';
    memset(title, 0, sizeof(title));
    colon = false;
    movement_force_led_off();
    pixel_clears = 0;
    next_face = -1;
    tick_frequency = 1;
    lcd_type = WATCH_LCD_TYPE_CLASSIC;
    random_calls = random_length = random_position = 0;
}

static wumpus_game_state_t fixture(void) {
    wumpus_game_state_t state = {
        .player_room = 0,
        .selected_room_n = -1,
        .current_action = wumpus_face_shoot,
        .arrows = WUMPUS_NUM_ARROWS,
        .blink_on = true,
    };
    state.hazards[19] = wumpus_hazard_wumpus;
    return state;
}

static void start_shot(wumpus_game_state_t *state, uint8_t length) {
    assert(state->current_action == wumpus_face_shoot);
    send_event(state, EVENT_ALARM_BUTTON_UP);
    for (uint8_t i = 1; i < length; i++) send_event(state, EVENT_LIGHT_BUTTON_UP);
    send_event(state, EVENT_ALARM_BUTTON_UP);
    assert(state->current_action == wumpus_face_shoot_rooms);
}

static void pick_shot_room(wumpus_game_state_t *state, uint8_t room) {
    unsigned limit = WUMPUS_FACE_ROWS;
    while (state->shots_room != room && limit--) send_event(state, EVENT_LIGHT_BUTTON_UP);
    assert(state->shots_room == room);
    send_event(state, EVENT_ALARM_BUTTON_UP);
}

static void test_cave_and_generation(void) {
    for (size_t room = 0; room < WUMPUS_FACE_ROWS; room++) {
        for (size_t i = 0; i < WUMPUS_FACE_COLS; i++) {
            uint8_t neighbor = cave_map[room][i];
            assert(neighbor < WUMPUS_FACE_ROWS && neighbor != room);
            assert(_rooms_connected(neighbor, room));
            for (size_t j = i + 1; j < WUMPUS_FACE_COLS; j++) assert(neighbor != cave_map[room][j]);
        }
    }
    for (unsigned trial = 0; trial < 1000; trial++) {
        void *context = NULL;
        wumpus_face.setup(0, &context);
        wumpus_game_state_t *state = context;
        assert(state != NULL && state->player_room < WUMPUS_FACE_ROWS);
        assert(state->hazards[state->player_room] == wumpus_hazard_none);
        unsigned pits = 0, bats = 0, wumpuses = 0;
        for (size_t i = 0; i < WUMPUS_FACE_ROWS; i++) {
            pits += state->hazards[i] == wumpus_hazard_pitfall;
            bats += state->hazards[i] == wumpus_hazard_bat;
            wumpuses += state->hazards[i] == wumpus_hazard_wumpus;
        }
        assert(pits == 2 && bats == 2 && wumpuses == 1);
        free(context);
    }
}

static void test_context_lifecycle(void) {
    void *first = NULL, *second = NULL;
    wumpus_face.setup(0, &first);
    wumpus_face.setup(1, &second);
    assert(first != NULL && second != NULL && first != second);
    *(wumpus_game_state_t *)first = fixture();
    *(wumpus_game_state_t *)second = fixture();
    wumpus_game_state_t untouched = *(wumpus_game_state_t *)second;
    wumpus_face.activate(first);
    assert(tick_frequency == 4);
    start_shot(first, 3);
    pick_shot_room(first, 1);
    wumpus_game_state_t saved = *(wumpus_game_state_t *)first;
    void *original = first;
    wumpus_face.resign(first);
    assert(tick_frequency == 1);
    wumpus_face.setup(0, &first); // Called again on wake; must not reallocate or reset.
    assert(first == original && memcmp(first, &saved, sizeof(saved)) == 0);
    wumpus_face.activate(first);
    assert(memcmp(first, &saved, sizeof(saved)) == 0);
    assert(memcmp(second, &untouched, sizeof(untouched)) == 0);
    memset(screen, ' ', 10);
    send_event(first, EVENT_ACTIVATE);
    assert(strncmp(screen + 4, "r2", 2) == 0 && colon);
    free(first);
    free(second);
}

static void test_menu_does_not_advance_turns(void) {
    wumpus_game_state_t state = fixture();
    RANDOM(0, 0, 0, 0, 0, 0);
    start_shot(&state, 5);
    pick_shot_room(&state, 1);
    for (unsigned i = 0; i < 40; i++) send_event(&state, EVENT_LIGHT_BUTTON_UP);
    for (unsigned i = 0; i < 8; i++) send_event(&state, EVENT_TICK);
    assert(random_calls == 0 && state.hazards[19] == wumpus_hazard_wumpus && state.arrows == 5);
}

static void test_cancel_move(void) {
    wumpus_game_state_t state = fixture();
    RANDOM(0, 0);
    send_event(&state, EVENT_LIGHT_BUTTON_UP); // GO
    send_event(&state, EVENT_ALARM_BUTTON_UP); // choose a room
    for (unsigned i = 0; i < 4; i++) send_event(&state, EVENT_LIGHT_BUTTON_UP);
    assert(state.selected_room_n == -1);
    send_event(&state, EVENT_ALARM_BUTTON_UP); // current room cancels
    assert(state.player_room == 0 && state.current_action == wumpus_face_go);
    assert(random_calls == 0);
}

static void test_move_advances_one_turn(void) {
    wumpus_game_state_t state = fixture();
    RANDOM(0, 0); // Wumpus: 19 -> 12.
    send_event(&state, EVENT_LIGHT_BUTTON_UP);
    send_event(&state, EVENT_ALARM_BUTTON_UP);
    send_event(&state, EVENT_LIGHT_BUTTON_UP);
    send_event(&state, EVENT_ALARM_BUTTON_UP);
    assert(state.player_room == 1 && state.current_action == wumpus_face_go);
    assert(state.hazards[12] == wumpus_hazard_wumpus && random_calls == 2);
    assert(strncmp(screen + 2, " 2", 2) == 0);
}

static void test_wumpus_probability_and_stationary_hazards(void) {
    wumpus_game_state_t state = fixture();
    memset(state.hazards, 0, sizeof(state.hazards));
    state.hazards[1] = wumpus_hazard_wumpus | wumpus_hazard_bat;
    state.hazards[0] = wumpus_hazard_pitfall;
    RANDOM(74, 0, 0, 0);
    assert(_wumpus_move(&state)); // 1 -> 0; neither bat nor pit may disappear.
    assert(state.hazards[1] == wumpus_hazard_bat);
    assert(state.hazards[0] == (wumpus_hazard_pitfall | wumpus_hazard_wumpus));
    assert(_wumpus_move(&state)); // 0 -> 1.
    assert(state.hazards[0] == wumpus_hazard_pitfall);
    assert(state.hazards[1] == (wumpus_hazard_bat | wumpus_hazard_wumpus));
    RANDOM(75, 99);
    assert(!_wumpus_move(&state));
    assert(!_wumpus_move(&state));
    assert(random_calls == 2);
}

static void test_entering_hazards(void) {
    const wumpus_hazard_type_t hazards[] = { wumpus_hazard_wumpus, wumpus_hazard_bat, wumpus_hazard_pitfall };
    for (size_t i = 0; i < sizeof(hazards) / sizeof(*hazards); i++) {
        wumpus_game_state_t state = fixture();
        state.hazards[1] = hazards[i];
        state.current_action = wumpus_face_choosing_room;
        state.selected_room_n = 0;
        RANDOM(0, 0);
        send_event(&state, EVENT_ALARM_BUTTON_UP);
        assert(state.current_action == wumpus_face_died && state.death_cause == hazards[i]);
        assert(random_calls == 0); // A dead player must not get another Wumpus move.
        assert(led[0] == 255 && led[1] == 0);
        assert(strncmp(screen + 2, " 2DIED", 6) == 0);
    }
}

static void test_wumpus_can_enter_player_room(void) {
    wumpus_game_state_t state = fixture();
    state.hazards[19] = 0;
    state.hazards[2] = wumpus_hazard_wumpus;
    state.current_action = wumpus_face_choosing_room;
    state.selected_room_n = 0; // Player: 0 -> 1.
    RANDOM(0, 0); // Wumpus: 2 -> 1.
    send_event(&state, EVENT_ALARM_BUTTON_UP);
    assert(state.current_action == wumpus_face_died && state.death_cause == wumpus_hazard_wumpus);
}

static void test_shrinking_and_overlapping_warnings(void) {
    wumpus_game_state_t state = fixture();
    memset(state.hazards, 0, sizeof(state.hazards));
    state.hazards[1] = wumpus_hazard_wumpus | wumpus_hazard_pitfall;
    state.hazards[4] = wumpus_hazard_bat;
    _display_hazards(&state);
    assert(strncmp(screen + 8, "UU", 2) == 0);
    _display_hazards(&state);
    assert(strncmp(screen + 8, "Bt", 2) == 0);
    state.hazards[1] = wumpus_hazard_pitfall;
    state.hazards[4] = 0; // Three warnings shrink to one while cursor is at 2.
    send_event(&state, EVENT_TICK);
    assert(strncmp(screen + 8, "Pt", 2) == 0 && state.hazard_point == 0);
    state.hazards[1] = 0;
    send_event(&state, EVENT_TICK);
    assert(strncmp(screen + 8, "  ", 2) == 0);
}

static void test_five_arrows(void) {
    wumpus_game_state_t state = fixture();
    RANDOM(99, 99, 99, 99);
    for (unsigned shot = 0; shot < WUMPUS_NUM_ARROWS; shot++) {
        start_shot(&state, 1);
        pick_shot_room(&state, 1);
        assert(state.arrows == WUMPUS_NUM_ARROWS - shot - 1);
        assert(state.current_action == (shot == 4 ? wumpus_face_died : wumpus_face_shoot));
    }
    assert(random_calls == 4 && state.death_cause == wumpus_hazard_arrow);
    assert(strncmp(screen + 4, "DIEDAr", 6) == 0);
    send_event(&state, EVENT_ALARM_BUTTON_UP);
    assert(state.arrows == 0 && state.current_action == wumpus_face_died);
}

static void test_last_arrow_can_win(void) {
    wumpus_game_state_t state = fixture();
    state.hazards[19] = 0;
    state.hazards[1] = wumpus_hazard_wumpus | wumpus_hazard_bat;
    state.arrows = 1;
    RANDOM(0, 0); // Moving after victory would bring the Wumpus to the player.
    start_shot(&state, 1);
    pick_shot_room(&state, 1);
    assert(state.current_action == wumpus_face_won && state.arrows == 0);
    assert(random_calls == 0 && led[0] == 0 && led[1] == 255);
    assert(strncmp(screen + 4, "Great ", 6) == 0);
}

static void test_arrow_can_hit_player(void) {
    wumpus_game_state_t state = fixture();
    start_shot(&state, 2);
    pick_shot_room(&state, 1);
    pick_shot_room(&state, 0);
    assert(state.current_action == wumpus_face_died && state.death_cause == wumpus_hazard_arrow);
    assert(state.arrows == 4);
}

static void test_arrow_follows_actual_tunnels(void) {
    wumpus_game_state_t state = fixture();
    state.hazards[19] = 0;
    state.hazards[3] = wumpus_hazard_wumpus;
    RANDOM(1); // Invalid 1 -> 19 becomes 1 -> 2, then 2 -> 3 is valid.
    start_shot(&state, 3);
    pick_shot_room(&state, 1);
    pick_shot_room(&state, 19);
    pick_shot_room(&state, 3);
    assert(state.current_action == wumpus_face_won && random_calls == 1);
}

static void test_arrow_first_step_is_checked(void) {
    wumpus_game_state_t state = fixture();
    state.hazards[19] = 0;
    state.hazards[1] = wumpus_hazard_wumpus;
    state.shots_path_len = 1;
    state.shots_path[0] = 19; // Even an invalid first target cannot teleport the arrow.
    RANDOM(0);
    _shot(&state);
    assert(state.current_action == wumpus_face_won && random_calls == 1);
}

static void test_shooting_bats_preserves_pits(void) {
    wumpus_game_state_t state = fixture();
    state.hazards[1] = wumpus_hazard_bat | wumpus_hazard_pitfall;
    RANDOM(99);
    start_shot(&state, 1);
    pick_shot_room(&state, 1);
    assert(state.hazards[1] == wumpus_hazard_pitfall && state.current_action == wumpus_face_shoot);
    send_event(&state, EVENT_LIGHT_BUTTON_UP);
    send_event(&state, EVENT_ALARM_BUTTON_UP);
    send_event(&state, EVENT_LIGHT_BUTTON_UP);
    send_event(&state, EVENT_ALARM_BUTTON_UP);
    assert(state.current_action == wumpus_face_died && state.death_cause == wumpus_hazard_pitfall);
}

static void test_arrow_can_continue_past_bats(void) {
    wumpus_game_state_t state = fixture();
    state.hazards[19] = 0;
    state.hazards[1] = wumpus_hazard_bat;
    state.hazards[2] = wumpus_hazard_wumpus;
    start_shot(&state, 2);
    pick_shot_room(&state, 1);
    pick_shot_room(&state, 2);
    assert(state.hazards[1] == 0 && state.current_action == wumpus_face_won);
}

static void test_result_duration_and_pause(void) {
    wumpus_game_state_t state = fixture();
    _finish_game(&state, wumpus_face_died, wumpus_hazard_arrow);
    for (unsigned i = 0; i < WUMPUS_RESULT_TICKS - 1; i++) {
        send_event(&state, EVENT_TICK);
        assert(state.current_action == wumpus_face_died);
        assert(strncmp(screen + 4, "DIEDAr", 6) == 0);
    }
    wumpus_face_resign(&state);
    assert(led[0] == 0 && tick_frequency == 1);
    wumpus_face_activate(&state);
    assert(state.result_ticks == 1 && led[0] == 255);
    send_event(&state, EVENT_TICK);
    assert(state.current_action == wumpus_face_shoot && state.arrows == 5);
    assert(led[0] == 0 && led[1] == 0 && state.result_ticks == 0);
    assert(strncmp(screen + 4, "SHOT", 4) == 0);
}

static void test_lcd_variants(void) {
    wumpus_game_state_t state = fixture();
    state.hazards[1] = wumpus_hazard_wumpus;
    send_event(&state, EVENT_ACTIVATE);
    assert(strncmp(screen, "WH", 2) == 0 && pixel_clears == 2);
    lcd_type = WATCH_LCD_TYPE_CUSTOM;
    pixel_clears = 0;
    send_event(&state, EVENT_ACTIVATE);
    assert(strcmp(title, "WUM") == 0 && pixel_clears == 0);
    assert(strncmp(screen + 8, "W ", 2) == 0);
}

static void test_navigation_and_sleep(void) {
    wumpus_game_state_t state = fixture();
    start_shot(&state, 2);
    wumpus_game_state_t saved = state;
    send_event(&state, EVENT_MODE_BUTTON_UP);
    assert(next_face == 1);
    send_event(&state, EVENT_TIMEOUT);
    assert(next_face == 0);
    send_event(&state, EVENT_LOW_ENERGY_UPDATE);
    assert(memcmp(&saved, &state, sizeof(state)) == 0);
    send_event(NULL, EVENT_MODE_BUTTON_UP); // Allocation failure still allows leaving.
    assert(next_face == 1);
}

static void test_random_event_sequences(void) {
    void *context = NULL;
    wumpus_face_setup(0, &context);
    const movement_event_type_t events[] = {
        EVENT_LIGHT_BUTTON_UP, EVENT_ALARM_BUTTON_UP, EVENT_TICK, EVENT_TICK,
        EVENT_LIGHT_BUTTON_DOWN, EVENT_ACTIVATE, EVENT_MODE_BUTTON_UP,
    };
    for (unsigned i = 0; i < 50000; i++) {
        movement_event_type_t event = events[arc4random_uniform(sizeof(events) / sizeof(*events))];
        if (event == EVENT_MODE_BUTTON_UP) {
            wumpus_face_resign(context);
            wumpus_face_setup(0, &context);
            wumpus_face_activate(context);
        }
        send_event(context, event);
        wumpus_game_state_t *state = context;
        assert(state->player_room < WUMPUS_FACE_ROWS);
        assert(state->selected_room_n >= -1 && state->selected_room_n < WUMPUS_FACE_COLS);
        assert(state->arrows <= WUMPUS_NUM_ARROWS);
        assert(state->shots_path_len <= WUMPUS_MAX_SHOT_LENGTH);
        assert(state->shots_picked <= state->shots_path_len);
        unsigned pits = 0, bats = 0, wumpuses = 0;
        for (size_t room = 0; room < WUMPUS_FACE_ROWS; room++) {
            pits += !!(state->hazards[room] & wumpus_hazard_pitfall);
            bats += !!(state->hazards[room] & wumpus_hazard_bat);
            wumpuses += !!(state->hazards[room] & wumpus_hazard_wumpus);
        }
        assert(pits == 2 && bats <= 2 && wumpuses == 1);
    }
    free(context);
}

int main(void) {
    unsigned tests = 0;
#define RUN(test) do { reset_boundary(); test(); tests++; puts("PASS " #test); } while (0)
    RUN(test_cave_and_generation);
    RUN(test_context_lifecycle);
    RUN(test_menu_does_not_advance_turns);
    RUN(test_cancel_move);
    RUN(test_move_advances_one_turn);
    RUN(test_wumpus_probability_and_stationary_hazards);
    RUN(test_entering_hazards);
    RUN(test_wumpus_can_enter_player_room);
    RUN(test_shrinking_and_overlapping_warnings);
    RUN(test_five_arrows);
    RUN(test_last_arrow_can_win);
    RUN(test_arrow_can_hit_player);
    RUN(test_arrow_follows_actual_tunnels);
    RUN(test_arrow_first_step_is_checked);
    RUN(test_shooting_bats_preserves_pits);
    RUN(test_arrow_can_continue_past_bats);
    RUN(test_result_duration_and_pause);
    RUN(test_lcd_variants);
    RUN(test_navigation_and_sleep);
    RUN(test_random_event_sequences);
    printf("%u tests passed\n", tests);
    return 0;
}
