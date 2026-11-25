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

#define ACE    14
#define KING   13
#define QUEEN  12
#define JACK   11

#define MIN_CARD_VALUE 2
#define MAX_CARD_VALUE 9
#define CARD_RANK_COUNT (MAX_CARD_VALUE - MIN_CARD_VALUE + 1)
#define CARD_SUIT_COUNT 4
#define DECK_SIZE (CARD_SUIT_COUNT * CARD_RANK_COUNT)

#define POKER_HAND_SIZE 5
#define BOARD_DISPLAY_START 4
#define AI_MIN_CARD_KEEP 8
#define BETTING_ROUNDS 1
#define SORT_LOW_TO_HIGH false

typedef enum {
    PO_HIGH_CARD,
    PO_ONE_PAIR,
    PO_TWO_PAIR,
    PO_THREE_OF_A_KIND,
    PO_STRAIGHT,
    //PO_FLUSH, // We have no suits in this version, so this is not possible.
    PO_FULL_HOUSE,
    PO_FOUR_OF_A_KIND,
} hand_ranks_t;

typedef enum {
    PO_HC_FOUR_OF_A_KIND,
    PO_HC_THREE_OF_A_KIND,
    PO_HC_PAIR_1,
    PO_HC_PAIR_2,
    PO_HC_HIGH_CARD,  // Also used for straight.
    PO_HC_COUNT,
} highest_card_t;

typedef struct {
    hand_ranks_t hand_rank;
    bool hand_rank_cards[POKER_HAND_SIZE];  // map of the cards that were used in getting the rank
    bool cards_to_swap[POKER_HAND_SIZE];
    uint8_t hand[POKER_HAND_SIZE];  // Hand will be sorted
    uint8_t highest_card[PO_HC_COUNT];  // The card that is in the highest rank, in order: [from 3K, from pair 1, from pair 2, from high card]
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
static bool displaying_opponent;
static uint8_t betting_round;
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
    watch_display_character('-', 9); // - on the final position means we want to submit our hand.
    opponent.hand_rank = PO_HIGH_CARD;
    reset_deck();
}

static inline void reset_hand_rank_info(hand_info_t *hand_info) {
    for (uint8_t i = 0; i < PO_HC_COUNT; i++) {
        hand_info->highest_card[i] = 0;
    }
    hand_info->hand_rank = PO_HIGH_CARD;
    for (uint8_t i = 0; i < POKER_HAND_SIZE; i++) {
        hand_info->hand_rank_cards[i] = false;
    }
}

static inline void set_hand_rank_cards(hand_info_t *hand_info, uint8_t card) {
    for (uint8_t i = 0; i < POKER_HAND_SIZE; i++) {
        if (hand_info->hand[i] == card) {
            hand_info->hand_rank_cards[i] = true;
        }
    }
}

static inline void set_high_card(hand_info_t *hand_info, bool use_non_ranked_cards_only) {
#if SORT_LOW_TO_HIGH
    for (uint8_t i = POKER_HAND_SIZE; i-- > 0;)
#else
    for (uint8_t i = 0; i < POKER_HAND_SIZE; i++)
#endif
    {
        if (!use_non_ranked_cards_only || !hand_info->hand_rank_cards[i]) {
            hand_info->highest_card[PO_HC_HIGH_CARD] = hand_info->hand[i];
            return;
        }
    }
}

static void get_hand_rank(hand_info_t *hand_info) {
    // We first check if it's a straight
    bool is_straight = true;
    reset_hand_rank_info(hand_info);
    uint8_t duplicate_counts[MAX_CARD_VALUE + 1] = {0};
    duplicate_counts[hand_info->hand[0]]++;
    for (uint8_t i = 1; i < POKER_HAND_SIZE; i++) {
        duplicate_counts[hand_info->hand[i]]++;
#if SORT_LOW_TO_HIGH
        if (hand_info->hand[i] != hand_info->hand[i-1] + 1)
#else
        if (hand_info->hand[i] != hand_info->hand[i-1] - 1)
#endif
        {
            is_straight = false;
        }
    }
    if (is_straight) {
        hand_info->hand_rank = PO_STRAIGHT;
        for (uint8_t i = 0; i < POKER_HAND_SIZE; i++) {
            hand_info->hand_rank_cards[i] = true;
        }
        set_high_card(hand_info, false);
        return;
    }
    for (uint8_t i = MIN_CARD_VALUE; i <= MAX_CARD_VALUE; i++) {
        if (duplicate_counts[i] == 4) {
            hand_info->hand_rank = PO_FOUR_OF_A_KIND;
            set_hand_rank_cards(hand_info, i);
            hand_info->highest_card[PO_HC_FOUR_OF_A_KIND] = i;
            set_high_card(hand_info, true);
            return;  
        } else if (duplicate_counts[i] == 3) {
            set_hand_rank_cards(hand_info, i);
            hand_info->highest_card[PO_HC_THREE_OF_A_KIND] = i;
            if (hand_info->hand_rank == PO_ONE_PAIR) {
                hand_info->hand_rank = PO_FULL_HOUSE;
                return;
            }
            hand_info->hand_rank = PO_THREE_OF_A_KIND;
        } else if (duplicate_counts[i] == 2) {
            set_hand_rank_cards(hand_info, i);
            if (hand_info->hand_rank == PO_THREE_OF_A_KIND) {
                hand_info->hand_rank = PO_FULL_HOUSE;
                hand_info->highest_card[PO_HC_PAIR_1] = i;
                return;
            } else if (hand_info->hand_rank == PO_ONE_PAIR) {
                hand_info->hand_rank = PO_TWO_PAIR;
                if (i > hand_info->highest_card[PO_HC_PAIR_1]) {
                    hand_info->highest_card[PO_HC_PAIR_2] = hand_info->highest_card[PO_HC_PAIR_1];
                    hand_info->highest_card[PO_HC_PAIR_1] = i;
                } else {
                    hand_info->highest_card[PO_HC_PAIR_2] = i;
                }
                set_high_card(hand_info, true);
                return;
            } else {
                hand_info->highest_card[PO_HC_PAIR_1] = i;
                hand_info->hand_rank = PO_ONE_PAIR;
            }
        }
    }
    if (hand_info->hand_rank == PO_HIGH_CARD) {
#if SORT_LOW_TO_HIGH
        hand_info->hand_rank_cards[POKER_HAND_SIZE - 1] = true;
#else
        hand_info->hand_rank_cards[0] = true;
#endif
        set_high_card(hand_info, false);
    } else {
        set_high_card(hand_info, true);

    }
}

static inline void sort_cards(uint8_t cards[POKER_HAND_SIZE]) {
    for (uint8_t i = 1; i < POKER_HAND_SIZE; i++) {
        uint8_t key = cards[i];
        uint8_t j;
        for (j = i; j > 0; j--) {
#if SORT_LOW_TO_HIGH
            if (cards[j-1] <= key)
#else
            if (cards[j-1] >= key)
#endif
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
/*
    hand_info->hand[0] = 7;
    hand_info->hand[1] = 7;
    hand_info->hand[2] = 9;
    hand_info->hand[3] = 9;
    hand_info->hand[4] = 9;
*/
    sort_cards(hand_info->hand);
    get_hand_rank(hand_info);
}

static void set_segment_at_position(segment_t segment, uint8_t position) {
    digit_mapping_t segmap;
    watch_lcd_type_t lcd_type = watch_get_lcd_type();
    if (lcd_type == WATCH_LCD_TYPE_CUSTOM) {
        segmap = Custom_LCD_Display_Mapping[position];
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
            watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, "St", " S");
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
    display_rank(opponent.hand_rank);
}

static void display_lifted_card_at_position(uint8_t card, uint8_t display_position) {
    bool indicate_top = false;
    bool is_classic = watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC;
    switch (card) {
        case 4:
            indicate_top = true;
            // fall-through
        case 1:
        case 7:
            watch_display_character('`', display_position);
            break;
        case 2:
            indicate_top = true;
            watch_display_character(' ', display_position);
            set_segment_at_position(F, display_position);
            set_segment_at_position(G, display_position);
            break;
        case 3:
        case 5:
        case 9:
            indicate_top = true;
            // fall-through
        case JACK:
            watch_display_character(' ', display_position);
            set_segment_at_position(B, display_position);
            set_segment_at_position(G, display_position);
            break;
        case 6:
        case 8:
            indicate_top = true;
            watch_display_character(' ', display_position);
            set_segment_at_position(B, display_position);
            set_segment_at_position(F, display_position);
            set_segment_at_position(G, display_position);
            break;
        case KING:
            indicate_top = true;
            set_segment_at_position(B, display_position);
            set_segment_at_position(F, display_position);
            break;
        case QUEEN:
            indicate_top = true;
            watch_display_character(' ', display_position);
            set_segment_at_position(B, display_position);
            break;
        case ACE:
            watch_display_character(' ', display_position);
            if (is_classic) {
                set_segment_at_position(B, display_position);
                set_segment_at_position(F, display_position);
                set_segment_at_position(G, display_position);
            } else {
                indicate_top = true;
                set_segment_at_position(B, display_position);
                set_segment_at_position(F, display_position);
            }
            break;
        default: {
            watch_display_character(' ', display_position);
            break;
        }
    }
    if (indicate_top && !is_classic) {
        set_segment_at_position(A, display_position);
    }
}

static void display_card_at_position(uint8_t card, uint8_t display_position) {
    switch (card) {
        case ACE:
            watch_display_character(watch_get_lcd_type() != WATCH_LCD_TYPE_CLASSIC ? 'A' : 'a', display_position);
            break;
        case KING:
            watch_display_character('K', display_position);
            break;
        case QUEEN:
            watch_display_character('q', display_position);
            break;
        case JACK:
            watch_display_character('J', display_position);
            break;
        case 10:
            watch_display_character('0', display_position);
            break;
        default: {
            const char display_char = card + '0';
            watch_display_character(display_char, display_position);
            break;
        }
    }
}

static void display_player_hand(void) {
    for (uint8_t i=0; i<POKER_HAND_SIZE; i++) {
        display_card_at_position(player.hand[i], BOARD_DISPLAY_START + i);
    }
}

static void display_opponent_hand(void) {
    for (uint8_t i=0; i<POKER_HAND_SIZE; i++) {
        display_card_at_position(opponent.hand[i], BOARD_DISPLAY_START + i);
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
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "POKER ", " POKER");
}

static void display_win_ratio(poker_face_state_t *state) {
    char buf[7];
    game_state = PO_WIN_RATIO;
    uint8_t win_ratio = 0;
    if (state->games_played > 0) {  // Avoid dividing by zero
        win_ratio = (uint8_t)((100 * state->games_won) / state->games_played);
    }
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "WINS  ", "WR");
    sprintf(buf, "%3dPct", win_ratio);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static inline void display_betting_round(void) {
    if (BETTING_ROUNDS <= 1) return;
    watch_display_character(' ', 1);
    watch_display_character(betting_round + '0', 0);
}

static void begin_playing(void) {
    movement_request_tick_frequency(2);
    game_state = PO_PLAYING;
    watch_display_text(WATCH_POSITION_BOTTOM, "      ");
    displaying_opponent = false;
    betting_round = 1;
    display_betting_round();
    reset_hands();
    // Give player their first 2 cards
    give_hand(&player);
    give_hand(&opponent);
    display_player_hand();
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

static bool swap_cards(hand_info_t *hand_info) {
    bool cards_swapped = false;
    for (uint8_t i=0; i<POKER_HAND_SIZE; i++) {
        if (hand_info->cards_to_swap[i]) {
            hand_info->hand[i] = get_next_card();
            hand_info->cards_to_swap[i] = false;
            cards_swapped = true;
        }
    }
    if (cards_swapped) {
        sort_cards(hand_info->hand);
        get_hand_rank(hand_info);
    }
    return cards_swapped;
}

static inline void display_win(void) {
    add_to_game_scores(true);
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "WI", "WI");
    display_opponent_rank();
}

static inline void display_lose(void) {
    add_to_game_scores(false);
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "LO", "LO");
    display_opponent_rank();
}

static inline void display_tie(void) {
    // Don't record ties to the win ratio
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "TI", "TI");
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
    watch_display_character(' ', 9);
    if (player.hand_rank == opponent.hand_rank) {
        bool is_tying = true;
        for (uint8_t i = 0; i < PO_HC_COUNT; i++) {
            if (player.highest_card[i] == opponent.highest_card[i]) {
                continue;
            } else if (player.highest_card[i] > opponent.highest_card[i]) {
                is_tying = false;
                display_win();
                break;
            } else {
                is_tying = false;
                display_lose();
                break;
            }
        }
        if (is_tying) {
            display_tie();
        }
    } else if (player.hand_rank > opponent.hand_rank) {
        display_win();
    } else {
        display_lose();
    }
#if __EMSCRIPTEN__
    EM_ASM({ console.log('player hand           : [' + $0 + ', ' + $1 + ', '
            + $2 + ', ' + $3 + ', ' + $4 + '] = ' + $5); }, 
            player.hand[0], player.hand[1], player.hand[2],
            player.hand[3], player.hand[4], player.hand_rank);
    EM_ASM({ console.log('player hand_rank_cards: [' + $0 + ', ' + $1 + ', '
            + $2 + ', ' + $3 + ', ' + $4 + ']'); }, 
            player.hand_rank_cards[0], player.hand_rank_cards[1], player.hand_rank_cards[2],
            player.hand_rank_cards[3], player.hand_rank_cards[4]);
    EM_ASM({ console.log('player highest_card   : [' + $0 + ', ' + $1 + ', '
            + $2 + ', ' + $3 + ', ' + $4 + ']'); }, 
            player.highest_card[PO_HC_FOUR_OF_A_KIND], player.highest_card[PO_HC_THREE_OF_A_KIND],
            player.highest_card[PO_HC_PAIR_1], player.highest_card[PO_HC_PAIR_2],
            player.highest_card[PO_HC_HIGH_CARD]);

    EM_ASM({ console.log('opponent hand           : [' + $0 + ', ' + $1 + ', '
            + $2 + ', ' + $3 + ', ' + $4 + '] = ' + $5); }, 
            opponent.hand[0], opponent.hand[1], opponent.hand[2],
            opponent.hand[3], opponent.hand[4], opponent.hand_rank);
    EM_ASM({ console.log('opponent hand_rank_cards: [' + $0 + ', ' + $1 + ', '
            + $2 + ', ' + $3 + ', ' + $4 + ']'); }, 
            opponent.hand_rank_cards[0], opponent.hand_rank_cards[1], opponent.hand_rank_cards[2],
            opponent.hand_rank_cards[3], opponent.hand_rank_cards[4]);
    EM_ASM({ console.log('opponent highest_card   : [' + $0 + ', ' + $1 + ', '
            + $2 + ', ' + $3 + ', ' + $4 + ']'); }, 
            opponent.highest_card[PO_HC_FOUR_OF_A_KIND], opponent.highest_card[PO_HC_THREE_OF_A_KIND],
            opponent.highest_card[PO_HC_PAIR_1], opponent.highest_card[PO_HC_PAIR_2],
            opponent.highest_card[PO_HC_HIGH_CARD]);
#endif
}

static inline void display_player_card_or_lifted_at_position(uint8_t card_to_display) {
    if (card_to_display == POKER_HAND_SIZE) return;
    if (player.cards_to_swap[card_to_display]) {
        display_lifted_card_at_position(player.hand[card_to_display], BOARD_DISPLAY_START + card_to_display);
    } else {
        display_card_at_position(player.hand[card_to_display], BOARD_DISPLAY_START + card_to_display);
    }
}

static inline void display_player_card_or_lifted(void) {
    display_player_card_or_lifted_at_position(player.selected_pos);
}

static inline void underscore_selected(bool ignore_if_lifted) {
    if (ignore_if_lifted || player.cards_to_swap[player.selected_pos]) {
        set_segment_at_position(watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC ? G : D,
        BOARD_DISPLAY_START + player.selected_pos);
    }
}

static inline void invert_all_swap_card(void) {
    for (uint8_t i=0; i<POKER_HAND_SIZE; i++) {
        player.cards_to_swap[i] = !player.cards_to_swap[i];
        display_player_card_or_lifted_at_position(i);
    }
}
static void blink_selected_card(uint8_t subsecond) {
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
        watch_display_character('-', 9);
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
            watch_display_character('-', 9);
            return;
        }
        underscore_selected(false);
    }
}

static void make_decision(void) {
    if (player.selected_pos == POKER_HAND_SIZE) {
        bool player_swapped = swap_cards(&player);
        if (player_swapped) {
            display_player_hand();
        }
        display_rank(player.hand_rank);
        opponent_decisions(&opponent);
        bool opponent_swapped = swap_cards(&opponent);
        betting_round++;
        if (betting_round > BETTING_ROUNDS) {
            get_result();
        } else if (!player_swapped) {
            // Even though we're good with our cards, the opponent gets to keep swapping until they're out of rounds.
            while (opponent_swapped && betting_round <= BETTING_ROUNDS) {
                opponent_decisions(&opponent);
                opponent_swapped = swap_cards(&opponent);
                betting_round++;
            }
            get_result();
        } else {
            display_betting_round();
        }
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
    displaying_opponent = false;
    display_title();
    stack_deck();
}

bool poker_face_loop(movement_event_t event, void *context) {
    poker_face_state_t *state = (poker_face_state_t *) context;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            break;
        case EVENT_TICK:
            if (game_state == PO_PLAYING) {
                blink_selected_card(event.subsecond);
            }
            break;
        case EVENT_MODE_BUTTON_UP:
            if (game_state == PO_PLAYING) {
                handle_button_presses(PO_LEFT);
            } else {
                return movement_default_loop_handler(event);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            handle_button_presses(PO_RIGHT);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            handle_button_presses(PO_SELECT);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (game_state == PO_TITLE_SCREEN) {
                display_win_ratio(state);
            } else if (game_state == PO_PLAYING) {
                invert_all_swap_card();
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
            } else if (game_state == PO_PLAYING) {
                if (player.selected_pos != POKER_HAND_SIZE) {
                    display_player_card_or_lifted();
                    player.selected_pos = POKER_HAND_SIZE;
                }
                handle_button_presses(PO_SELECT);
            } else if (game_state == PO_TITLE_SCREEN) {
                display_win_ratio(state);
            } else if (game_state == PO_RESULT) {
                if (displaying_opponent) {
                    display_player_hand();
                    display_rank(player.hand_rank);
                } else {
                    display_opponent_hand();
                    display_opponent_rank();
                }
                displaying_opponent = !displaying_opponent;
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
