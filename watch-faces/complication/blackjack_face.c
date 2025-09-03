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

#define ACE    13
#define KING   12
#define QUEEN  11
#define JACK   10

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
    BJ_RESULT,
} game_state_t;

static game_state_t game_state;
static uint8_t deck[DECK_SIZE] = {0};
static uint8_t current_card = 0;
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

static uint8_t get_next_card(hand_info_t *hand_info) {
    if (current_card >= DECK_SIZE)
        reset_deck();
    uint8_t card = deck[current_card++];
    switch (card)
    {
    case ACE:
        card = 11;
        hand_info->high_aces_in_hand++;
        break;
    case KING:
    case QUEEN:
    case JACK:
        card = 10;
    
    default:
        break;
    }
    return card;
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
    uint8_t card = get_next_card(hand_info);
    hand_info->hand[hand_info->idx_hand++] = card;
    hand_info->score += card;
    modify_score_from_aces(hand_info);
}

static void display_player_hand(void) { 
    uint8_t cards_to_display = player.idx_hand > MAX_PLAYER_CARDS_DISPLAY ? MAX_PLAYER_CARDS_DISPLAY : player.idx_hand;
    for (uint8_t i=cards_to_display; i>0; i--) {
        uint8_t card = player.hand[player.idx_hand-i];
        if (card == 10) card = 0;
        const char display_char = card + '0';
        watch_display_character(display_char, BOARD_DISPLAY_START + cards_to_display - i);
    }
}

static void display_dealer_hand(void) {
    uint8_t card = dealer.hand[dealer.idx_hand - 1];
    if (card == 10) card = 0;
    const char display_char = card + '0';
    watch_display_character(display_char, 0);
}

static void display_player_score(void) {
    char buf[4];
    sprintf(buf, "%2d", player.score);
    watch_display_text(WATCH_POSITION_SECONDS, buf);
}

static void display_dealer_score(void) {
    char buf[4];
    sprintf(buf, "%2d", dealer.score);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
}

static void display_win(void) {
    game_state = BJ_RESULT;
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "WlN ", " WIN");
    display_player_score();
    display_dealer_score();
}

static void display_lose(void) {
    game_state = BJ_RESULT;
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "LOSE", "lOSE");
    display_player_score();
    display_dealer_score();
}

static void display_tie(void) {
    game_state = BJ_RESULT;
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "TlE ", " TIE");
    display_player_score();
}

static void display_bust(void) {
    game_state = BJ_RESULT;
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "8UST ", " BUST");
    display_player_score();
}

static void display_title(void) {
    game_state = BJ_TITLE_SCREEN;
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "BLACK ", "21  ");
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, " JACK ", "BLaKJK");
}

static void begin_playing(void) {
    watch_clear_display();
    game_state = BJ_PLAYING;
    reset_hands();
    // Give player their first 2 cards
    give_card(&player);
    give_card(&player);
    display_player_hand();
    display_player_score();
    give_card(&dealer);
    display_dealer_hand();
    display_dealer_score();
}

static void perform_hit(void) {
    if (player.score == 21) {
        return; // Assume hitting on 21 is a mistake and ignore
    }
    give_card(&player);
    if (player.score > 21) {
        display_bust();
    } else {
        display_player_hand();
        display_player_score();
    }
}

static void perform_stand(void) {
    game_state = BJ_DEALER_PLAYING;
    watch_display_text(WATCH_POSITION_BOTTOM, "Stnd");
    display_player_score();
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
        display_dealer_score();
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

static void handle_button_presses(bool hit) {
    switch (game_state)
    {
    case BJ_TITLE_SCREEN:
        begin_playing();
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
    case BJ_RESULT:
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
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            if (state->tap_control_on) {
                movement_enable_tap_detection_if_available();
            }
            break;
        case EVENT_TICK:
            if (game_state == BJ_DEALER_PLAYING) {
                see_if_dealer_hits();
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
        case EVENT_DOUBLE_TAP:
            handle_button_presses(false);
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_ALARM_BUTTON_UP:
        case EVENT_SINGLE_TAP:
            handle_button_presses(true);
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
