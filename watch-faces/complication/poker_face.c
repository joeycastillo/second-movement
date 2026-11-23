/*
 * MIT License
 *
 * Copyright (c) 2026 David Volovskiy
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

// Emulator only: need time() to seed the random number generator.
#if __EMSCRIPTEN__
#include <time.h>
#include <emscripten.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "poker_face.h"
#include "watch_common_display.h"

#define MIN_CARD_VALUE 2
#define MAX_CARD_VALUE 9
#define CARD_RANK_COUNT (MAX_CARD_VALUE - MIN_CARD_VALUE + 1)
#define CARD_SUIT_COUNT 4
#define DECK_SIZE (CARD_SUIT_COUNT * CARD_RANK_COUNT)

#define POKER_HAND_SIZE 5
#define BOARD_DISPLAY_START 4
#define AI_MIN_CARD_KEEP 8 

typedef enum {
    PO_HIGH_CARD,
    PO_ONE_PAIR,
    PO_TWO_PAIR,
    PO_THREE_OF_A_KIND,
    PO_STRAIGHT,
    //PO_FLUSH, // We have no suits in this version, so this is not possible.
    PO_FLUSH,
    PO_FULL_HOUSE,
    PO_FOUR_OF_A_KIND,
} hand_ranks_t;

typedef struct {
    hand_ranks_t hand_rank;
    bool hand_rank_cards[POKER_HAND_SIZE];  // map of the cards that were used in getting the rank
    bool cards_to_swap[POKER_HAND_SIZE];
    uint8_t hand[POKER_HAND_SIZE];  // Hand will be sorted largest card to smallest
    uint8_t selected_pos;  // 0 = select left most, POKER_HAND_SIZE = submit hand
} hand_info_t;

typedef enum {
    PO_TITLE_SCREEN,
    PO_PLAYING,
    PO_RESULT,
    PO_WIN_RATIO,
} game_state_t;

typedef enum {
    A, B, C, D, E, F, G
} segment_t;

typedef enum {
    PO_LEFT,
    PO_RIGHT,
    PO_SELECT,
} button_options_t;

static game_state_t game_state;
static uint8_t deck[DECK_SIZE] = {0};
static uint8_t current_card = 0;
static poker_face_state_t *g_state = NULL;
static hand_info_t player;
static hand_info_t opponent;

static uint8_t generate_random_number(uint8_t num_values) {
    // Emulator: use rand. Hardware: use arc4random.
#if __EMSCRIPTEN__
    return rand() % num_values;
#else
    return arc4random_uniform(num_values);
#endif
}

static void stack_deck(void) {
    for (size_t i = 0; i < CARD_RANK_COUNT; i++) {
        for (size_t j = 0; j < CARD_SUIT_COUNT; j++)
            deck[(i * CARD_SUIT_COUNT) + j] = MIN_CARD_VALUE + i;
    }
}

static void shuffle_deck(void) {
    // Randomize shuffle with Fisher Yates
    size_t i;
    size_t j;
    uint8_t tmp;

    for (i = DECK_SIZE - 1; i > 0; i--) {
        j = generate_random_number(0xFF) % (i + 1);
        tmp = deck[j];
        deck[j] = deck[i];
        deck[i] = tmp;
    }
}

static void reset_deck(void) {
    current_card = 0;
    shuffle_deck();
}

static uint8_t get_next_card(void) {
    if (current_card >= DECK_SIZE)
        reset_deck();
    return deck[current_card++];
}

static void reset_hands(void) {
    memset(&player, 0, sizeof(player));
    memset(&opponent, 0, sizeof(opponent));
    player.hand_rank = PO_HIGH_CARD;
    player.selected_pos = POKER_HAND_SIZE;
    watch_display_character('o', 9); // o on the final position means we want to submit our hand.
    opponent.hand_rank = PO_HIGH_CARD;
    reset_deck();
}

static inline void set_hand_rank_cards(hand_info_t *hand_info, uint8_t card) {
    for (uint8_t i = 0; i < POKER_HAND_SIZE; i++) {
        if (hand_info->hand[i] == card) {
            hand_info->hand_rank_cards[i] = true;
        }
    }
}

static void get_hand_rank(hand_info_t *hand_info) {
    // We first check if it's a straight
    bool is_straight = true;
    uint8_t duplicate_counts[MAX_CARD_VALUE] = {0};
    duplicate_counts[hand_info->hand[0]]++;
    for (uint8_t i = 1; i < POKER_HAND_SIZE; i++) {
        duplicate_counts[hand_info->hand[i]]++;
        if (hand_info->hand[i] != hand_info->hand[i-1] + 1) {
            is_straight = false;
        }
    }
    if (is_straight) {
        hand_info->hand_rank = PO_STRAIGHT;
        for (uint8_t i = 0; i < POKER_HAND_SIZE; i++) {
            hand_info->hand_rank_cards[i] = true;
        }
        return;
    }
    for (uint8_t i = 1; i < MAX_CARD_VALUE; i++) {
        if (duplicate_counts[i] == 4) {
            hand_info->hand_rank = PO_FOUR_OF_A_KIND;
            set_hand_rank_cards(hand_info, i);
            return;  
        } else if (duplicate_counts[i] == 3) {
            set_hand_rank_cards(hand_info, i);
            if (hand_info->hand_rank == PO_ONE_PAIR) {
                hand_info->hand_rank = PO_FULL_HOUSE;
                return;
            }
            hand_info->hand_rank = PO_THREE_OF_A_KIND;
        } else if (duplicate_counts[i] == 2) {
            set_hand_rank_cards(hand_info, i);
            if (hand_info->hand_rank == PO_THREE_OF_A_KIND) {
                hand_info->hand_rank = PO_FULL_HOUSE;
                return;
            }
            if (hand_info->hand_rank == PO_ONE_PAIR) {
                hand_info->hand_rank = PO_TWO_PAIR;
                return;
            }
            hand_info->hand_rank = PO_ONE_PAIR;
        }
    }
    if (hand_info->hand_rank == PO_HIGH_CARD) {
        hand_info->hand_rank_cards[0] = true;  // Cards already sorted largest to smallest
    }
}

static inline void sort_cards(uint8_t cards[POKER_HAND_SIZE]) {
    for (uint8_t i = 1; i < POKER_HAND_SIZE; i++) {
        uint8_t key = cards[i];
        uint8_t j;
        for (j = i; j > 0; j--) {
            if (cards[j-1] >= key)
                break;
            cards[j] = cards[j-1];
        }
        cards[j] = key;
    }
}

static void give_hand(hand_info_t *hand_info) {
    for (uint8_t i=0; i < POKER_HAND_SIZE; i++) {
        hand_info->hand[i] = get_next_card();
    }
    sort_cards(hand_info->hand);
    get_hand_rank(hand_info);
}

static void set_segment_at_position(segment_t segment, uint8_t position) {
    digit_mapping_t segmap;
    watch_lcd_type_t lcd_type = watch_get_lcd_type();
    if (lcd_type == WATCH_LCD_TYPE_CUSTOM) {
        segmap = Custom_LCD_Display_Mapping[position];
    } else if (lcd_type == WATCH_LCD_TYPE_GSHOCK) {
        segmap = GShock_LCD_Display_Mapping[position];
    } else {
        segmap = Classic_LCD_Display_Mapping[position];
    }
    const uint8_t com_pin = segmap.segment[segment].address.com;
    const uint8_t seg = segmap.segment[segment].address.seg;
    watch_set_pixel(com_pin, seg);
}

static void display_rank(hand_ranks_t rank) {
    switch (rank) {
        case PO_HIGH_CARD:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, "HC", "HC");
            break;
        case PO_ONE_PAIR:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, "1P", "1P");
            break;
        case PO_TWO_PAIR:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, "2P", "2P");
            break;
        case PO_THREE_OF_A_KIND:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, "3K", " 3");
            break;
        case PO_STRAIGHT:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, " S", " S");
            break;
        case PO_FULL_HOUSE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, "FH", " F");
            break;
        case PO_FOUR_OF_A_KIND:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, "4K", " 4");
            break;
        default:
            break;
    }
}

static void display_opponent_rank(void) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_GSHOCK) {
        watch_display_text(WATCH_POSITION_MONTH_GSHOCK, "OP");
        watch_set_indicator(WATCH_INDICATOR_BOX_COLON_TOP);
        watch_set_indicator(WATCH_INDICATOR_BOX_COLON_BOTTOM);
    }
    display_rank(opponent.hand_rank);
}

static void display_lifted_card_at_position(uint8_t card, uint8_t display_position) {
    switch (card) {
        case 1:
        case 4:
        case 7:
            watch_display_character('`', display_position);
            break;
        case 2:
            watch_display_character(' ', display_position);
            set_segment_at_position(F, display_position);
            set_segment_at_position(G, display_position);
            break;
        case 3:
        case 5:
        case 9:
            watch_display_character(' ', display_position);
            set_segment_at_position(B, display_position);
            set_segment_at_position(G, display_position);
            break;

        case 6:
        case 8:
            watch_display_character(' ', display_position);
            set_segment_at_position(B, display_position);
            set_segment_at_position(F, display_position);
            set_segment_at_position(G, display_position);
            break;
        default: {
            watch_display_character(' ', display_position);
            break;
        }
    }
}

static void display_card_at_position(uint8_t card, uint8_t display_position) {
    switch (card) {
        default: {
            const char display_char = card + '0';
            watch_display_character(display_char, display_position);
            break;
        }
    }
}

static void display_player_hand(bool display_all) {
    for (uint8_t i=0; i<POKER_HAND_SIZE; i++) {
        if (display_all || player.cards_to_swap[i]) {
            display_card_at_position(player.hand[i], BOARD_DISPLAY_START + i);
        }
    }
}

static void add_to_game_scores(bool add_to_wins) {
    g_state->games_played++;
    if (g_state->games_played == 0) {
        // Overflow
        g_state->games_played = 1;
        g_state->games_won = add_to_wins ? 1 : 0;
        return;
    }
    if (add_to_wins) {
        g_state->games_won++;
        if (g_state->games_won == 0) {
            // Overflow
            g_state->games_played = 1;
            g_state->games_won = 1;
        }
    }
}

static void display_title(void) {
    game_state = PO_TITLE_SCREEN;
    watch_display_text(WATCH_POSITION_TOP_LEFT, "PO");
    watch_display_text_with_gshock_and_fallback(WATCH_POSITION_TOP_RIGHT, "    ", "  ", "  ");
    watch_clear_indicator(WATCH_INDICATOR_BOX_COLON_TOP);
    watch_clear_indicator(WATCH_INDICATOR_BOX_COLON_BOTTOM);
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "POKER ", " POKER");
}

static void display_win_ratio(poker_face_state_t *state) {
    char buf[7];
    game_state = PO_WIN_RATIO;
    uint8_t win_ratio = 0;
    if (state->games_played > 0) {  // Avoid dividing by zero
        win_ratio = (uint8_t)((100 * state->games_won) / state->games_played);
    }
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "PO");
    watch_display_text_with_gshock_and_fallback(WATCH_POSITION_TOP, "WR   ", "WINS  ", "WR");
    sprintf(buf, "%3dPct", win_ratio);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static void begin_playing(void) {
    watch_clear_display();
    game_state = PO_PLAYING;
    reset_hands();
    // Give player their first 2 cards
    give_hand(&player);
    give_hand(&opponent);
    display_player_hand(true);
    display_rank(player.hand_rank);
}

static void opponent_decisions(hand_info_t *hand_info) {
    switch (hand_info->hand_rank)
    {
    case PO_FOUR_OF_A_KIND:
    case PO_FULL_HOUSE:
    case PO_STRAIGHT:
        break;
    case PO_THREE_OF_A_KIND:
        for (uint8_t i=0; i<POKER_HAND_SIZE; i++) {
            if (!hand_info->hand_rank_cards[i]) {
                hand_info->cards_to_swap[i] = true;
            }
        }
        break;
    default:
        for (uint8_t i=0; i<POKER_HAND_SIZE; i++) {
            if (!hand_info->hand_rank_cards[i] && hand_info->hand[i] < AI_MIN_CARD_KEEP) {
                hand_info->cards_to_swap[i] = true;
            }
        }
        break;
    }
}

static void swap_cards(hand_info_t *hand_info) {
    bool cards_swapped = false;
    for (uint8_t i=0; i<POKER_HAND_SIZE; i++) {
        if (hand_info->cards_to_swap[i]) {
            hand_info->hand[i] = get_next_card();
            cards_swapped = true;
        }
    }
    if (cards_swapped) {
        sort_cards(hand_info->hand);
        get_hand_rank(hand_info);
    }
}

static inline void display_win(void) {
    add_to_game_scores(true);
    watch_display_text(WATCH_POSITION_TOP_LEFT, "WI");
    display_opponent_rank();
}

static inline void display_lose(void) {
    add_to_game_scores(false);
    watch_display_text(WATCH_POSITION_TOP_LEFT, "LO");
    display_opponent_rank();
}

static inline void display_tie(void) {
    // Don't record ties to the win ratio
    watch_display_text(WATCH_POSITION_TOP_LEFT, "TI");
    display_opponent_rank();
}

static inline uint8_t get_top_rank_card(const hand_info_t *hand_info) {
    for (uint8_t i = 0; i < POKER_HAND_SIZE; i++) {
        if (hand_info->hand_rank_cards[i])
            return hand_info->hand[i];
    }
    return 0;
}

static void get_result(void) {
    game_state = PO_RESULT;
    if (player.hand_rank == opponent.hand_rank) {
        // If both players have the same rank, take the highest card that makes the rank of either.
        // This has a bug, where if there's a 3 of a kind that's 2s for one and 3s for the other and they both have a pair of 4s,
        // the 4s are the highest cards. So this is imperfect, but good enough since we won't be sharing the opponent's cards.
        uint8_t top_card_player = get_top_rank_card(&player);
        uint8_t top_card_opponent = get_top_rank_card(&opponent);
        if (top_card_player == top_card_opponent) {
            display_tie();
        } else if (top_card_player > top_card_opponent) {
            display_win();
        } else {
            display_lose();
        }
    } else if (player.hand_rank > opponent.hand_rank) {
        display_win();
    } else {
        display_lose();
    }
}

static inline void display_player_card_or_lifted(void) {
    if (player.cards_to_swap[player.selected_pos]) {
        display_lifted_card_at_position(player.hand[player.selected_pos], BOARD_DISPLAY_START + player.selected_pos);
    } else {
        display_card_at_position(player.hand[player.selected_pos], BOARD_DISPLAY_START + player.selected_pos);
    }
}

static inline void underscore_selected(bool ignore_if_lifted) {
    if (ignore_if_lifted || player.cards_to_swap[player.selected_pos]) {
        set_segment_at_position(D, BOARD_DISPLAY_START + player.selected_pos);
    }
}

static void blink_selected_card(uint8_t subsecond) {
#if __EMSCRIPTEN__
    EM_ASM({
    console.log('selected_pos:' + $0);
    }, player.selected_pos);
#endif
    if (player.selected_pos == POKER_HAND_SIZE) return;
    if (subsecond % 2) {
        watch_display_character(' ', BOARD_DISPLAY_START + player.selected_pos);
        underscore_selected(true);
    } else {
        display_player_card_or_lifted();
        underscore_selected(false);
    }
}

static void move_left(void) {
    display_player_card_or_lifted(); // In case blink_selected_card left that card blank
    if (player.selected_pos == 0) {
        player.selected_pos = POKER_HAND_SIZE;
        watch_display_character('o', 9);
    } else {
        if (player.selected_pos == POKER_HAND_SIZE) {
            watch_display_character(' ', 9);
        }
        player.selected_pos--;
        underscore_selected(false);
    }
}

static void move_right(void) {
    display_player_card_or_lifted(); // In case blink_selected_card left that card blank
    if (player.selected_pos == POKER_HAND_SIZE) {
        player.selected_pos = 0;
        watch_display_character(' ', 9);
    } else {
        player.selected_pos++;
        if (player.selected_pos == POKER_HAND_SIZE) {
            watch_display_character('o', 9);
            return;
        }
        underscore_selected(false);
    }
}

static void make_decision(void) {
    if (player.selected_pos == POKER_HAND_SIZE) {
        swap_cards(&player);
        display_player_hand(false);
        display_rank(player.hand_rank);
        opponent_decisions(&opponent);
        get_result();
    } else {
        player.cards_to_swap[player.selected_pos] = !player.cards_to_swap[player.selected_pos];
        display_player_card_or_lifted();
    }
}

static void handle_button_presses(button_options_t option) {
    switch (game_state)
    {
    case PO_TITLE_SCREEN:
        begin_playing();
        break;
    case PO_PLAYING:
        switch (option)
        {
        case PO_LEFT:
            move_left();
            break;
        case PO_RIGHT:
            move_right();
            break;
        case PO_SELECT:
        default:
            make_decision();
            break;
        }
        break;
    case PO_RESULT:
    case PO_WIN_RATIO:
        display_title();
        break;
    }
}

void poker_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(poker_face_state_t));
        memset(*context_ptr, 0, sizeof(poker_face_state_t));
    }
    g_state = (poker_face_state_t *)*context_ptr;
}

void poker_face_activate(void *context) {
    poker_face_state_t *state = (poker_face_state_t *) context;
    (void) state;
    display_title();
    stack_deck();
}

static bool toggle_flash = false;

bool poker_face_loop(movement_event_t event, void *context) {
    poker_face_state_t *state = (poker_face_state_t *) context;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            break;
        case EVENT_TICK:
            if (game_state == PO_PLAYING) {
                toggle_flash = !toggle_flash;
                blink_selected_card(toggle_flash);
            }
            break;
#ifdef FORCE_GSHOCK_LCD_TYPE
        case EVENT_START_BUTTON_UP:
            handle_button_presses(PO_LEFT);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            handle_button_presses(PO_RIGHT);
            break;
        case EVENT_ALARM_BUTTON_UP:
            handle_button_presses(PO_SELECT);
            break;
#else
        case EVENT_MODE_LONG_PRESS:
            movement_move_to_next_face();
            break;
        case EVENT_MODE_BUTTON_UP:
            if (game_state != PO_PLAYING) {
                movement_move_to_next_face();
            } else {
                handle_button_presses(PO_LEFT);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            handle_button_presses(PO_RIGHT);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            handle_button_presses(PO_SELECT);
            break;
#endif
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (game_state == PO_TITLE_SCREEN) {
                display_win_ratio(state);
            } else {
                movement_illuminate_led();
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (game_state == PO_WIN_RATIO) {
                // Reset the win-lose ratio
                state->games_won = 0;
                state->games_played = 0;
                watch_display_text(WATCH_POSITION_BOTTOM, "  0Pct");
            }
            break;
        case EVENT_TIMEOUT:
        case EVENT_LOW_ENERGY_UPDATE:
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void poker_face_resign(void *context) {
    (void) context;
}
