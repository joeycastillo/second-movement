/*
 * MIT License
 *
 * Copyright (c) 2025 David Volovskiy
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
#endif

#include <stdlib.h>
#include <string.h>
#include "blackjack_face.h"
#include "watch_common_display.h"

#define ACE    14
#define KING   13
#define QUEEN  12
#define JACK   11

#define MIN_CARD_VALUE 2
#define MAX_CARD_VALUE ACE
#define CARD_RANK_COUNT (MAX_CARD_VALUE - MIN_CARD_VALUE + 1)
#define CARD_SUIT_COUNT 4
#define DECK_SIZE (CARD_SUIT_COUNT * CARD_RANK_COUNT)

#define BLACKJACK_MAX_HAND_SIZE 11 // 4*1 + 4*2 + 3*3 = 21; 11 cards total
#define MAX_PLAYER_CARDS_DISPLAY 4
#define BOARD_DISPLAY_START 4

typedef struct {
    uint8_t score;
    uint8_t idx_hand;
    int8_t high_aces_in_hand;
    uint8_t hand[BLACKJACK_MAX_HAND_SIZE];
} hand_info_t;

typedef enum {
    BJ_TITLE_SCREEN,
    BJ_PLAYING,
    BJ_DEALER_PLAYING,
    BJ_BUST,
    BJ_RESULT,
    BJ_WIN_RATIO,
} game_state_t;

typedef enum {
    A, B, C, D, E, F, G
} segment_t;

static game_state_t game_state;
static uint8_t deck[DECK_SIZE] = {0};
static uint8_t current_card = 0;
static bool add_to_games_played = false;
static bool add_to_games_won = false;
hand_info_t player;
hand_info_t dealer;

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
    stack_deck();
    shuffle_deck();
}

static uint8_t get_next_card(void) {
    if (current_card >= DECK_SIZE)
        reset_deck();
    return deck[current_card++];
}

static uint8_t get_card_value(uint8_t card) {
    switch (card)
    {
    case ACE:
        return 11;
    case KING:
    case QUEEN:
    case JACK:
        return 10;
    default:
        return card;
    }
}

static void modify_score_from_aces(hand_info_t *hand_info) {
    while (hand_info->score > 21 && hand_info->high_aces_in_hand > 0) {
        hand_info->score -= 10;
        hand_info->high_aces_in_hand--;
    }
}

static void reset_hands(void) {
    memset(&player, 0, sizeof(player));
    memset(&dealer, 0, sizeof(dealer));
    reset_deck();
}

static void give_card(hand_info_t *hand_info) {
    uint8_t card = get_next_card();
    if (card == ACE) hand_info->high_aces_in_hand++;
    hand_info->hand[hand_info->idx_hand++] = card;
    uint8_t card_value = get_card_value(card);
    hand_info->score += card_value;
    modify_score_from_aces(hand_info);
}

static void set_segment_at_position(segment_t segment, uint8_t position) {
    digit_mapping_t segmap;
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        segmap = Custom_LCD_Display_Mapping[position];
    } else {
        segmap = Classic_LCD_Display_Mapping[position];
    }
    const uint8_t com_pin = segmap.segment[segment].address.com;
    const uint8_t seg = segmap.segment[segment].address.seg;
    watch_set_pixel(com_pin, seg);
}

static void display_card_at_position(uint8_t card, uint8_t display_position) {
    switch (card) {
        case KING:
            watch_display_character(' ', display_position);
            set_segment_at_position(A, display_position);
            set_segment_at_position(D, display_position);
            set_segment_at_position(G, display_position);
            break;
        case QUEEN:
            watch_display_character(' ', display_position);
            set_segment_at_position(A, display_position);
            set_segment_at_position(D, display_position);
            break;
        case JACK:
            watch_display_character('-', display_position);
            break;
        case ACE:
            watch_display_character(watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM ? 'A' : 'a', display_position);
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
    uint8_t card;
    if (player.idx_hand <= MAX_PLAYER_CARDS_DISPLAY) {
        card = player.hand[player.idx_hand - 1];
        display_card_at_position(card, BOARD_DISPLAY_START + player.idx_hand - 1);
    } else {
        for (uint8_t i=0; i<MAX_PLAYER_CARDS_DISPLAY; i++) {
            card = player.hand[player.idx_hand - MAX_PLAYER_CARDS_DISPLAY + i];
            display_card_at_position(card, BOARD_DISPLAY_START + i);
        }
    }
}

static void display_dealer_hand(void) {
    uint8_t card = dealer.hand[dealer.idx_hand - 1];
    display_card_at_position(card, 0);
}

static void display_score(uint8_t score, watch_position_t pos) {
    char buf[4];
    sprintf(buf, "%2d", score);
    watch_display_text(pos, buf);
}

static void display_win(void) {
    game_state = BJ_RESULT;
    add_to_games_played = true;
    add_to_games_won = true;
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "WlN ", " WIN");
    display_score(player.score, WATCH_POSITION_SECONDS);
    display_score(dealer.score, WATCH_POSITION_TOP_RIGHT);
}

static void display_lose(void) {
    game_state = BJ_RESULT;
    add_to_games_played = true;
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "LOSE", "lOSE");
    display_score(player.score, WATCH_POSITION_SECONDS);
    display_score(dealer.score, WATCH_POSITION_TOP_RIGHT);
}

static void display_tie(void) {
    game_state = BJ_RESULT;
    // Don't record ties to the win ratio
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "TlE ", " TIE");
    display_score(player.score, WATCH_POSITION_SECONDS);
}

static void display_bust(void) {
    game_state = BJ_RESULT;
    add_to_games_played = true;
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "8UST", " BUST");
}

static void display_title(void) {
    game_state = BJ_TITLE_SCREEN;
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "BLACK ", "21  ");
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, " JACK ", "BLaKJK");
}

static void display_win_ratio(blackjack_face_state_t *state) {
    char buf[7];
    game_state = BJ_WIN_RATIO;
    uint8_t win_ratio = 0;
    if (state->games_played > 0) {  // Avoid dividing by zero
        win_ratio = (uint8_t)(100 * state->games_won) / state->games_played;
    }
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "WINS  ", "WR  ");
    sprintf(buf, "%3dPct", win_ratio);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static void begin_playing(bool tap_control_on) {
    watch_clear_display();
    if (tap_control_on) {
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    }
    game_state = BJ_PLAYING;
    reset_hands();
    // Give player their first 2 cards
    give_card(&player);
    display_player_hand();
    give_card(&player);
    display_player_hand();
    display_score(player.score, WATCH_POSITION_SECONDS);
    give_card(&dealer);
    display_dealer_hand();
    display_score(dealer.score, WATCH_POSITION_TOP_RIGHT);
}

static void perform_hit(void) {
    if (player.score == 21) {
        return; // Assume hitting on 21 is a mistake and ignore
    }
    give_card(&player);
    if (player.score > 21) {
        game_state = BJ_BUST;
    }
    display_player_hand();
    display_score(player.score, WATCH_POSITION_SECONDS);
}

static void perform_stand(void) {
    game_state = BJ_DEALER_PLAYING;
    watch_display_text(WATCH_POSITION_BOTTOM, "Stnd");
    display_score(player.score, WATCH_POSITION_SECONDS);
}

static void dealer_performs_hits(void) {
    give_card(&dealer);
    display_dealer_hand();
    if (dealer.score > 21) {
        display_win();
    } else if (dealer.score > player.score) {
        display_lose();
    } else {
        display_dealer_hand();
        display_score(dealer.score, WATCH_POSITION_TOP_RIGHT);
    } 
}

static void see_if_dealer_hits(void) {
    if (dealer.score > 16) {
        if (dealer.score > player.score) {
            display_lose();
        } else if (dealer.score < player.score) {
            display_win();
        } else {
            display_tie();
        }
    } else {
        dealer_performs_hits();
    }
}

static void add_to_game_scores(blackjack_face_state_t *state) {
    if (add_to_games_played) {
        add_to_games_played = false;
        state->games_played++;
        if (state->games_played == 0) {
            // Overflow
            state->games_won = 0;
        }
    }
    if (add_to_games_won) {
        add_to_games_won = false;
        state->games_won++;
        if (state->games_won == 0) {
            // Overflow
            state->games_played = 0;
        }
    }
}

static void handle_button_presses(bool tap_control_on, bool hit) {
    switch (game_state)
    {
    case BJ_TITLE_SCREEN:
        begin_playing(tap_control_on);
        break;
    case BJ_PLAYING:
        if (hit) {
            perform_hit();
        } else {
            perform_stand();
        }
        break;
    case BJ_DEALER_PLAYING:
        see_if_dealer_hits();
        break;
    case BJ_BUST:
        display_bust();
        break;
    case BJ_RESULT:
    case BJ_WIN_RATIO:
        display_title();
        break;
    }
}

static void toggle_tap_control(blackjack_face_state_t *state) {
    if (state->tap_control_on) {
        movement_disable_tap_detection_if_available();
        state->tap_control_on = false;
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    } else {
        bool tap_could_enable = movement_enable_tap_detection_if_available();
        if (tap_could_enable) {
            state->tap_control_on = true;
            watch_set_indicator(WATCH_INDICATOR_SIGNAL);
        }
    }
}

void blackjack_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(blackjack_face_state_t));
        memset(*context_ptr, 0, sizeof(blackjack_face_state_t));
        blackjack_face_state_t *state = (blackjack_face_state_t *)*context_ptr;
        state->tap_control_on = false;
    }
}

void blackjack_face_activate(void *context) {
    blackjack_face_state_t *state = (blackjack_face_state_t *) context;
    (void) state;
    display_title();
}

bool blackjack_face_loop(movement_event_t event, void *context) {
    blackjack_face_state_t *state = (blackjack_face_state_t *) context;
    if (game_state == BJ_RESULT) {
        add_to_game_scores(state);
    }
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            if (state->tap_control_on) {
                bool tap_could_enable = movement_enable_tap_detection_if_available();
                if (tap_could_enable) {
                    watch_set_indicator(WATCH_INDICATOR_SIGNAL);
                } else {
                    state->tap_control_on = false;
                }
            }
            break;
        case EVENT_TICK:
            if (game_state == BJ_DEALER_PLAYING) {
                see_if_dealer_hits();
            }
            else if (game_state == BJ_BUST) {
                display_bust();
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
        case EVENT_DOUBLE_TAP:
            handle_button_presses(state->tap_control_on, false);
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_ALARM_BUTTON_UP:
        case EVENT_SINGLE_TAP:
            handle_button_presses(state->tap_control_on, true);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (game_state == BJ_TITLE_SCREEN) {
                display_win_ratio(state);
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (game_state == BJ_TITLE_SCREEN) {
                toggle_tap_control(state);
            }
            break;
        case EVENT_TIMEOUT:
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void blackjack_face_resign(void *context) {
    (void) context;
}
