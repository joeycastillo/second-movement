/*
 * MIT License
 *
 * Copyright (c) 2023 Chris Ellis
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
#include "higher_lower_game_face.h"
#include "watch_common_display.h"


#define KING   12
#define QUEEN  11
#define JACK   10

#define TITLE_TEXT "Hi-Lo"
#define GAME_BOARD_SIZE 6
#define MAX_BOARDS 40
#define GUESSES_PER_SCREEN 5
#define WIN_SCORE (MAX_BOARDS * GUESSES_PER_SCREEN)
#define BOARD_DISPLAY_START 4
#define BOARD_DISPLAY_END 9
#define MIN_CARD_VALUE 2
#define MAX_CARD_VALUE KING
#define CARD_RANK_COUNT (MAX_CARD_VALUE - MIN_CARD_VALUE + 1)
#define CARD_SUIT_COUNT 4
#define DECK_SIZE (CARD_SUIT_COUNT * CARD_RANK_COUNT)
#define FLIP_BOARD_DIRECTION false

typedef struct card_t {
    uint8_t value;
    bool revealed;
} card_t;

typedef enum {
    A, B, C, D, E, F, G
} segment_t;

typedef enum {
    HL_GUESS_EQUAL,
    HL_GUESS_HIGHER,
    HL_GUESS_LOWER
} guess_t;

typedef enum {
    HL_GS_TITLE_SCREEN,
    HL_GS_GUESSING,
    HL_GS_WIN,
    HL_GS_LOSE,
    HL_GS_SHOW_SCORE,
} game_state_t;

static game_state_t game_state = HL_GS_TITLE_SCREEN;
static card_t game_board[GAME_BOARD_SIZE] = {0};
static uint8_t guess_position = 0;
static uint8_t score = 0;
static uint8_t completed_board_count = 0;
static uint8_t deck[DECK_SIZE] = {0};
static uint8_t current_card = 0;

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

static void reset_board(bool first_round) {
    // First card is random on the first board, and carried over from the last position on subsequent boards
    const uint8_t first_card_value = first_round
                                     ? get_next_card()
                                     : game_board[GAME_BOARD_SIZE - 1].value;

    game_board[0].value = first_card_value;
    game_board[0].revealed = true; // Always reveal first card

    // Fill remainder of board
    for (size_t i = 1; i < GAME_BOARD_SIZE; ++i) {
        game_board[i] = (card_t) {
                .value = get_next_card(),
                .revealed = false
        };
    }
}

static void init_game(void) {
    watch_clear_display();
    watch_display_text(WATCH_POSITION_BOTTOM, TITLE_TEXT);
    watch_display_text(WATCH_POSITION_TOP_LEFT, "HL");
    reset_deck();
    reset_board(true);
    score = 0;
    completed_board_count = 0;
    guess_position = 1;
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

static inline size_t get_display_position(size_t board_position) {
    return FLIP_BOARD_DIRECTION ? BOARD_DISPLAY_START + board_position : BOARD_DISPLAY_END - board_position;
}

static void render_board_position(size_t board_position) {
    const size_t display_position = get_display_position(board_position);
    const bool revealed = game_board[board_position].revealed;

    //// Current position indicator spot
    //if (board_position == guess_position) {
    //    watch_display_character('-', display_position);
    //    return;
    //}

    if (!revealed) {
        // Higher or lower indicator (currently just an empty space)
        watch_display_character(' ', display_position);
        //set_segment_at_position(F, display_position);
        return;
    }

    const uint8_t value = game_board[board_position].value;
    switch (value) {
        case KING: // K (≡)
            watch_display_character(' ', display_position);
            set_segment_at_position(A, display_position);
            set_segment_at_position(D, display_position);
            set_segment_at_position(G, display_position);
            break;
        case QUEEN: // Q (=)
            watch_display_character(' ', display_position);
            set_segment_at_position(A, display_position);
            set_segment_at_position(D, display_position);
            break;
        case JACK: // J (-)
            watch_display_character('-', display_position);
            break;
        default: {
            const char display_char = (value - MIN_CARD_VALUE) + '0';
            watch_display_character(display_char, display_position);
        }
    }
}

static void render_board(void) {
    for (size_t i = 0; i < GAME_BOARD_SIZE; ++i) {
        render_board_position(i);
    }
}

static void render_board_count(void) {
    // Render completed boards (screens)
    char buf[3] = {0};
    snprintf(buf, sizeof(buf), "%2hhu", completed_board_count);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
}

static void render_final_score(void) {
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "SCORE", "SC  ");
    char buf[7] = {0};
    const uint8_t complete_boards = score / GUESSES_PER_SCREEN;
    snprintf(buf, sizeof(buf), "%2hu %03hu", complete_boards, score);
    watch_set_colon();
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static guess_t get_answer(void) {
    if (guess_position < 1 || guess_position > GAME_BOARD_SIZE)
        return HL_GUESS_EQUAL; // Maybe add an error state, shouldn't ever hit this.

    game_board[guess_position].revealed = true;
    const uint8_t previous_value = game_board[guess_position - 1].value;
    const uint8_t current_value = game_board[guess_position].value;

    if (current_value > previous_value)
        return HL_GUESS_HIGHER;
    else if (current_value < previous_value)
        return HL_GUESS_LOWER;
    else
        return HL_GUESS_EQUAL;
}

static void do_game_loop(guess_t user_guess) {
    switch (game_state) {
        case HL_GS_TITLE_SCREEN:
            init_game();
            render_board();
            render_board_count();
            game_state = HL_GS_GUESSING;
            break;
        case HL_GS_GUESSING: {
            const guess_t answer = get_answer();

            // Render answer indicator
            switch (answer) {
                case HL_GUESS_EQUAL:
                    watch_display_text(WATCH_POSITION_TOP_LEFT, "==");
                    break;
                case HL_GUESS_HIGHER:
                    watch_display_text(WATCH_POSITION_TOP_LEFT, "HI");
                    break;
                case HL_GUESS_LOWER:
                    watch_display_text(WATCH_POSITION_TOP_LEFT, "LO");
                    break;
            }

            // Scoring
            if (answer == user_guess) {
                score++;
            } else if (answer == HL_GUESS_EQUAL) {
                // No score for two consecutive identical cards
            } else {
                // Incorrect guess, game over
                watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "End", "GO");
                game_board[guess_position].revealed = true;
                watch_display_text(WATCH_POSITION_BOTTOM, "------");
                render_board_position(guess_position - 1);
                render_board_position(guess_position);
                if (game_board[guess_position].value == JACK && guess_position < GAME_BOARD_SIZE) // Adds a space in case the revealed option is '-'
                    watch_display_character(' ', get_display_position(guess_position + 1));
                game_state = HL_GS_LOSE;
                return;
            }

            if (score >= WIN_SCORE) {
                // Win, perhaps some kind of animation sequence?
                watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "WIN", "WI");
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
                watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "WINNER", "winnEr");
                game_state = HL_GS_WIN;
                return;
            }

            // Next guess position
            const bool final_board_guess = guess_position == GAME_BOARD_SIZE - 1;
            if (final_board_guess) {
                // Seed new board
                completed_board_count++;
                render_board_count();
                guess_position = 1;
                reset_board(false);
                render_board();
            } else {
                guess_position++;
                render_board_position(guess_position - 1);
                render_board_position(guess_position);
            }
            break;
        }
        case HL_GS_WIN:
        case HL_GS_LOSE:
            // Show score screen on button press from either state
            watch_clear_display();
            render_final_score();
            game_state = HL_GS_SHOW_SCORE;
            break;
        case HL_GS_SHOW_SCORE:
            watch_clear_display();
            watch_display_text(WATCH_POSITION_BOTTOM, TITLE_TEXT);
            watch_display_text(WATCH_POSITION_TOP_LEFT, "HL");
            game_state = HL_GS_TITLE_SCREEN;
            break;
        default:
            watch_display_text(WATCH_POSITION_BOTTOM, "ERROR");
            break;
    }
}

static void light_button_handler(void) {
    do_game_loop(HL_GUESS_HIGHER);
}

static void alarm_button_handler(void) {
    do_game_loop(HL_GUESS_LOWER);
}

void higher_lower_game_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(higher_lower_game_face_state_t));
        memset(*context_ptr, 0, sizeof(higher_lower_game_face_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
        memset(game_board, 0, sizeof(game_board));
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

void higher_lower_game_face_activate(void *context) {
    higher_lower_game_face_state_t *state = (higher_lower_game_face_state_t *) context;
    (void) state;
    // Handle any tasks related to your watch face coming on screen.
    game_state = HL_GS_TITLE_SCREEN;
    stack_deck();
}

bool higher_lower_game_face_loop(movement_event_t event, void *context) {
    higher_lower_game_face_state_t *state = (higher_lower_game_face_state_t *) context;
    (void) state;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            // Show your initial UI here.
            watch_display_text(WATCH_POSITION_BOTTOM, TITLE_TEXT);
            watch_display_text(WATCH_POSITION_TOP_LEFT, "HL");
            break;
        case EVENT_TICK:
            // If needed, update your display here.
            break;
        case EVENT_LIGHT_BUTTON_UP:
            light_button_handler();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            // Don't trigger light
            break;
        case EVENT_ALARM_BUTTON_UP:
            alarm_button_handler();
            break;
        case EVENT_TIMEOUT:
            // Your watch face will receive this event after a period of inactivity. If it makes sense to resign,
            // you may uncomment this line to move back to the first watch face in the list:
            // movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    // return true if the watch can enter standby mode. Generally speaking, you should always return true.
    // Exceptions:
    //  * If you are displaying a color using the low-level watch_set_led_color function, you should return false.
    //  * If you are sounding the buzzer using the low-level watch_set_buzzer_on function, you should return false.
    // Note that if you are driving the LED or buzzer using Movement functions like movement_illuminate_led or
    // movement_play_alarm, you can still return true. This guidance only applies to the low-level watch_ functions.
    return true;
}

void higher_lower_game_face_resign(void *context) {
    (void) context;

    // handle any cleanup before your watch face goes off-screen.
}
