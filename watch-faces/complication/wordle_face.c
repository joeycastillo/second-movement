/*
 * MIT License
 *
 * Copyright (c) 2024 <David Volovskiy>
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
#include "wordle_face.h"
#include "watch_utility.h"
#include "watch_common_display.h"

static uint32_t get_random(uint32_t max) {
    #if __EMSCRIPTEN__
    return rand() % max;
    #else
    return arc4random_uniform(max);
    #endif
}

static uint8_t get_first_pos(wordle_letter_result *word_elements_result) {
    for (size_t i = 0; i < WORDLE_LENGTH; i++) {
        if (word_elements_result[i] != WORDLE_LETTER_CORRECT)
            return i;
    }
    return 0;
}

static uint8_t get_next_pos(uint8_t curr_pos, wordle_letter_result *word_elements_result) {
    for (size_t pos = curr_pos; pos < WORDLE_LENGTH;) {
        if (word_elements_result[++pos] != WORDLE_LETTER_CORRECT)
            return pos;
    }
    return WORDLE_LENGTH;
}

static uint8_t get_prev_pos(uint8_t curr_pos, wordle_letter_result *word_elements_result) {
    if (curr_pos == 0) return 0;
    for (int8_t pos = curr_pos; pos >= 0;) {
        if (word_elements_result[--pos] != WORDLE_LETTER_CORRECT)
            return pos;
    }
    return curr_pos;
}

static void get_next_letter(const uint8_t curr_pos, uint8_t *word_elements, const bool *known_wrong_letters, const bool skip_wrong_letter) {
    do {
        if (word_elements[curr_pos] >= WORDLE_NUM_VALID_LETTERS) word_elements[curr_pos] = 0;
        else word_elements[curr_pos] = (word_elements[curr_pos] + 1) % WORDLE_NUM_VALID_LETTERS;
    } while (skip_wrong_letter && known_wrong_letters[word_elements[curr_pos]]);  
}

static void get_prev_letter(const uint8_t curr_pos, uint8_t *word_elements, const bool *known_wrong_letters, const bool skip_wrong_letter) {
    do {
        if (word_elements[curr_pos] >= WORDLE_NUM_VALID_LETTERS) word_elements[curr_pos] = WORDLE_NUM_VALID_LETTERS - 1;
        else word_elements[curr_pos] = (word_elements[curr_pos] + WORDLE_NUM_VALID_LETTERS - 1) % WORDLE_NUM_VALID_LETTERS;
    } while (skip_wrong_letter && known_wrong_letters[word_elements[curr_pos]]);  
}

static void display_letter(wordle_state_t *state, bool display_dash) {
    char buf[3];
    if (state->word_elements[state->position] >= WORDLE_NUM_VALID_LETTERS) {
        if (display_dash)
            watch_display_character('-', state->position + 5);
        else
            watch_display_character(' ', state->position + 5);
        return;
    }
    sprintf(buf, "%c", _valid_letters[state->word_elements[state->position]]);
    watch_display_character(buf[0], state->position + 5);
}

static void display_all_letters(wordle_state_t *state) {
    uint8_t prev_pos = state->position;
    watch_display_character(' ', 4);
    for (size_t i = 0; i < WORDLE_LENGTH; i++) {
        state->position = i;
        display_letter(state, false);
    }
    state->position = prev_pos;
}

#if !WORDLE_ALLOW_NON_WORD_AND_REPEAT_GUESSES
static void display_not_in_dict(wordle_state_t *state) {
    state->curr_screen = WORDLE_SCREEN_NO_DICT;
    watch_display_text(WATCH_POSITION_BOTTOM, "nodict");
    state->ignore_btn_ticks = WORDLE_TICK_BAD_GUESS;
}

static void display_already_guessed(wordle_state_t *state) {
    state->curr_screen = WORDLE_SCREEN_ALREADY_GUESSED;
    watch_display_text(WATCH_POSITION_BOTTOM, "GUESSD");
    state->ignore_btn_ticks = WORDLE_TICK_BAD_GUESS;
}

static uint32_t check_word_in_dict(uint8_t *word_elements) {
    bool is_exact_match;
    for (uint16_t i = 0; i < WORDLE_NUM_WORDS; i++) {
        is_exact_match = true;
        for (size_t j = 0; j < WORDLE_LENGTH; j++) {
            if (_valid_letters[word_elements[j]] != _valid_words[i][j]) {
                is_exact_match = false;
                break;  
            }
        }
        if (is_exact_match) return i;
    }
    for (uint16_t i = 0; i < WORDLE_NUM_POSSIBLE_WORDS; i++) {
        is_exact_match = true;
        for (size_t j = 0; j < WORDLE_LENGTH; j++) {
            if (_valid_letters[word_elements[j]] != _possible_words[i][j]) {
                is_exact_match = false;
                break;  
            }
        }
        if (is_exact_match) return WORDLE_NUM_WORDS + i;
    }
    return WORDLE_NUM_WORDS + WORDLE_NUM_POSSIBLE_WORDS;
}
#endif

static bool check_word(wordle_state_t *state) {
    // Exact
    bool is_exact_match = true;
    bool answer_already_accounted[WORDLE_LENGTH] = { false };
    for (size_t i = 0; i < WORDLE_LENGTH; i++) {
        if (_valid_letters[state->word_elements[i]] == _valid_words[state->curr_answer][i]) {
            state->word_elements_result[i] = WORDLE_LETTER_CORRECT;
            answer_already_accounted[i] = true;
        }
        else {
            state->word_elements_result[i] = WORDLE_LETTER_WRONG;
            is_exact_match = false;
        }
    }
    if (is_exact_match) return true;
    // Wrong Location
    for (size_t i = 0; i < WORDLE_LENGTH; i++) {
        if (state->word_elements_result[i] != WORDLE_LETTER_WRONG) continue;
        for (size_t j = 0; j < WORDLE_LENGTH; j++) {
            if (answer_already_accounted[j]) continue;
            if (_valid_letters[state->word_elements[i]] == _valid_words[state->curr_answer][j]) {
                state->word_elements_result[i] = WORDLE_LETTER_WRONG_LOC;
                answer_already_accounted[j] = true;
                break;
            }
        }
    }
    return false;
}

static void show_skip_wrong_letter_indicator(bool skipping, wordle_screen curr_screen) {
    if (curr_screen >= WORDLE_SCREEN_PLAYING) return;
    if (skipping)
        watch_display_character('H', 3);
    else
        watch_display_character(' ', 3);
}

static void update_known_wrong_letters(wordle_state_t *state) {
    bool wrong_loc[WORDLE_NUM_VALID_LETTERS] = {false};
    // To ignore letters that appear, but are in the wrong location, as letters that are guessed
    // more often than they appear in the word will display as WORDLE_LETTER_WRONG
    for (size_t i = 0; i < WORDLE_LENGTH; i++) {
        if (state->word_elements_result[i] == WORDLE_LETTER_WRONG_LOC) {
            for (size_t j = 0; j < WORDLE_NUM_VALID_LETTERS; j++) {
                if (state->word_elements[i] == j)
                    wrong_loc[j] = true;
            }
        }
    }
    for (size_t i = 0; i < WORDLE_LENGTH; i++) {
        if (state->word_elements_result[i] == WORDLE_LETTER_WRONG) {
            for (size_t j = 0; j < WORDLE_NUM_VALID_LETTERS; j++) {
                if (state->word_elements[i] == j && !wrong_loc[j])
                    state->known_wrong_letters[j] = true;
            }
        }
    }
}

static void display_attempt(uint8_t attempt) {
    char buf[3];
    sprintf(buf, "%d", attempt+1);
    watch_display_character(buf[0], 3);
}

static void display_playing(wordle_state_t *state) {
    state->curr_screen = WORDLE_SCREEN_PLAYING;
    display_attempt(state->attempt);
    display_all_letters(state);
}

static void reset_all_elements(wordle_state_t *state) {
    for (size_t i = 0; i < WORDLE_LENGTH; i++) {
        state->word_elements[i] = WORDLE_NUM_VALID_LETTERS;
        state->word_elements_result[i] = WORDLE_LETTER_WRONG;
    }
    for (size_t i = 0; i < WORDLE_NUM_VALID_LETTERS; i++){
        state->known_wrong_letters[i] = false;
    }
#if !WORDLE_ALLOW_NON_WORD_AND_REPEAT_GUESSES
    for (size_t i = 0; i < WORDLE_MAX_ATTEMPTS; i++) {
        state->guessed_words[i] = WORDLE_NUM_WORDS + WORDLE_NUM_POSSIBLE_WORDS;
    }
#endif
    state->using_random_guess = false;
    state->attempt = 0;
}

static void reset_incorrect_elements(wordle_state_t *state) {
    for (size_t i = 0; i < WORDLE_LENGTH; i++) {
        if (state->word_elements_result[i] != WORDLE_LETTER_CORRECT)
            state->word_elements[i] = WORDLE_NUM_VALID_LETTERS;
    }
}

static bool is_in_do_not_use_list(uint16_t guess, const uint16_t *not_to_use, uint8_t max_repeats) {
    for (size_t i = 0; i < max_repeats; i++) {
        if (guess == not_to_use[i]) return true;
    }
    return false;
}

static void reset_board(wordle_state_t *state) {
    reset_all_elements(state);
    do {
        state->curr_answer = get_random(WORDLE_NUM_WORDS);
     } while (is_in_do_not_use_list(state->curr_answer, state->not_to_use, WORDLE_MAX_BETWEEN_REPEATS));
    watch_clear_colon();
    state->position = get_first_pos(state->word_elements_result);
    display_playing(state);
    watch_display_character('-', 5);
#if __EMSCRIPTEN__
    printf("ANSWER: %s\r\n", _valid_words[state->curr_answer]);
#endif
}

static void display_title(wordle_state_t *state) {
    state->curr_screen = WORDLE_SCREEN_TITLE;
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "WO ", "WO");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    watch_display_text(WATCH_POSITION_BOTTOM, "WordLE");
    show_skip_wrong_letter_indicator(state->skip_wrong_letter, state->curr_screen);
}

#if WORDLE_USE_DAILY_STREAK != 2
static void display_continue_result(bool continuing) {
    watch_display_character(continuing ? 'y' : 'n', 9);
}

static void display_continue(wordle_state_t *state) {
    state->curr_screen = WORDLE_SCREEN_CONTINUE;
    watch_display_text(WATCH_POSITION_BOTTOM, "Cont ");
    show_skip_wrong_letter_indicator(state->skip_wrong_letter, state->curr_screen);
    display_continue_result(state->continuing);
}
#endif

static void display_streak(wordle_state_t *state) {
    char buf[10];
    state->curr_screen = WORDLE_SCREEN_STREAK;
#if WORDLE_USE_DAILY_STREAK == 2
    if (state->streak > 99)
        sprintf(buf, "St--dy");
    else
        sprintf(buf, "St%2ddy", state->streak);
#else
    sprintf(buf, "St%4d", state->streak);
#endif
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "WO ", "WO");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    watch_set_colon();
    show_skip_wrong_letter_indicator(state->skip_wrong_letter, state->curr_screen);
}

#if WORDLE_USE_DAILY_STREAK == 2
static void display_wait(wordle_state_t *state) {
    state->curr_screen = WORDLE_SCREEN_WAIT;
    if (state->streak < 40) {
        char buf[5];
        sprintf(buf,"%2d", state->streak);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }
    else {  // Streak too long to display in top-right
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    }
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "WO ", "WO");
    watch_display_text(WATCH_POSITION_BOTTOM, " WaIt ");
    show_skip_wrong_letter_indicator(state->skip_wrong_letter, state->curr_screen);
}
#endif

static uint32_t get_day_unix_time(void) {
    watch_date_time_t now = watch_rtc_get_date_time();
#if WORDLE_USE_DAILY_STREAK == 2
    now.unit.hour = now.unit.minute = now.unit.second = 0;
#endif
    return watch_utility_date_time_to_unix_time(now, 0);
}

static void display_lose(wordle_state_t *state, uint8_t subsecond) {
    char buf[10];
    sprintf(buf," %s", subsecond % 2 ? _valid_words[state->curr_answer] : "     ");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "LOSE", "L ");
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static void display_win(wordle_state_t *state, uint8_t subsecond) {
    (void) state;
    char buf[10];
    sprintf(buf," %s ", subsecond % 2 ? "NICE" : "JOb ");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "WIN", "W ");
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static bool is_playing(const wordle_state_t *state) {
    if (state->attempt > 0) return true;
    for (size_t i = 0; i < WORDLE_LENGTH; i++) {
        if (state->word_elements[i] != WORDLE_NUM_VALID_LETTERS)
            return true;
    }
    return false;
}

static void display_result(wordle_state_t *state, uint8_t subsecond) {
    char buf[10];
    buf[0] = ' ';
    for (size_t i = 0; i < WORDLE_LENGTH; i++)
    {
        switch (state->word_elements_result[i])
        {
        case WORDLE_LETTER_WRONG:
            buf[i+1] = '-';
            break;
        case WORDLE_LETTER_CORRECT:
            buf[i+1] = _valid_letters[state->word_elements[i]];
            break;
        case WORDLE_LETTER_WRONG_LOC:
            if (subsecond % 2)
                buf[i+1] = ' ';
            else
               buf[i+1] = _valid_letters[state->word_elements[i]]; 
        default:
            break;
        }
    }
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static bool act_on_btn(wordle_state_t *state, const wordle_pin_enum pin) {
    if (state->ignore_btn_ticks > 0) return true;
    switch (state->curr_screen)
    {
    case WORDLE_SCREEN_RESULT:
        reset_incorrect_elements(state);
        state->position = get_first_pos(state->word_elements_result);
        display_playing(state);
        return true;
    case WORDLE_SCREEN_TITLE:
#if WORDLE_USE_DAILY_STREAK == 2
        if (state->day_last_game_started == get_day_unix_time()) {
            display_wait(state);
        }
        else if (is_playing(state))
            display_playing(state);
        else
            display_streak(state);
#else
        if (is_playing(state)) {
            state->continuing = true;
            display_continue(state);
        }
        else
            display_streak(state);
#endif
        return true;
    case WORDLE_SCREEN_STREAK:
        state->day_last_game_started = get_day_unix_time();
        reset_board(state);
        return true;
    case WORDLE_SCREEN_WIN:
    case WORDLE_SCREEN_LOSE:
        display_title(state);
        return true;
    case WORDLE_SCREEN_NO_DICT:
    case WORDLE_SCREEN_ALREADY_GUESSED:
        state->position = get_first_pos(state->word_elements_result);
        display_playing(state);
        return true;
#if WORDLE_USE_DAILY_STREAK == 2
    case WORDLE_SCREEN_WAIT:
        (void) pin;
        display_title(state);
        return true;
#else
    case WORDLE_SCREEN_CONTINUE:
        switch (pin)
        {
        case BTN_ALARM:
            if (state->continuing)
                display_playing(state);
            else {
                reset_board(state);
                state->streak = 0;
                display_streak(state);
            }
            break;
        case BTN_LIGHT:
            state->continuing = !state->continuing;
            display_continue_result(state->continuing);
            break;
        default:
            break;
        }
        return true;
#endif
    default:
        return false;
    }
    return false;
}

static void win_lose_shared(wordle_state_t *state) {
        reset_all_elements(state);
        state->ignore_btn_ticks = WORDLE_TICK_WIN_LOSE;
        state->not_to_use[state->not_to_use_position] = state->curr_answer;
        state->not_to_use_position = (state->not_to_use_position + 1) % WORDLE_MAX_BETWEEN_REPEATS;        
}

static void get_result(wordle_state_t *state) {
#if !WORDLE_ALLOW_NON_WORD_AND_REPEAT_GUESSES
    // Check if it's in the dict
    uint16_t in_dict = check_word_in_dict(state->word_elements);
    if (in_dict == WORDLE_NUM_WORDS + WORDLE_NUM_POSSIBLE_WORDS) {
        display_not_in_dict(state);
        return;
    }

    // Check if already guessed
    for (size_t i = 0; i < WORDLE_MAX_ATTEMPTS; i++) {
        if(in_dict == state->guessed_words[i]) {
            display_already_guessed(state);
            return;
        }
    }

    state->guessed_words[state->attempt] = in_dict;
#endif
    bool exact_match = check_word(state);
    if (exact_match) {
        state->curr_screen = WORDLE_SCREEN_WIN;
        win_lose_shared(state);
        if (state->streak < 0x7F)
            state->streak++;
#if WORDLE_USE_DAILY_STREAK == 2
        state->day_last_game_started = get_day_unix_time();  // On the edge-case where we solve the puzzle at midnight
#endif
        return;
    }
    if (++state->attempt >= WORDLE_MAX_ATTEMPTS) {
        state->curr_screen = WORDLE_SCREEN_LOSE;
        win_lose_shared(state);
        state->streak = 0;
        return;
    }
    update_known_wrong_letters(state);
    state->curr_screen = WORDLE_SCREEN_RESULT;
    state->ignore_btn_ticks = WORDLE_TICKS_RESULT;
    return;
}

#if (WORDLE_USE_RANDOM_GUESS != 0)
static void insert_random_guess(wordle_state_t *state) {
    uint16_t random_guess;
    do {  // Don't allow the guess to be the same as the answer
        random_guess = get_random(_num_random_guess_words);
    } while (random_guess == state->curr_answer); 
    for (size_t i = 0; i < WORDLE_LENGTH; i++) {
        for (size_t j = 0; j < WORDLE_NUM_VALID_LETTERS; j++)
        {
            if (_valid_words[random_guess][i] == _valid_letters[j])
                state->word_elements[i] = j;     
        }
    } 
    state->position = WORDLE_LENGTH - 1;
    display_all_letters(state);
    state->using_random_guess = true;
}
#endif

static void _activate(wordle_state_t *state) {
#if WORDLE_USE_DAILY_STREAK != 0
    uint32_t now = get_day_unix_time();
    uint32_t one_day = 60 *60 * 24;
    if ((WORDLE_USE_DAILY_STREAK == 2 && now >= (state->day_last_game_started + (2*one_day)))
        || (now >= (state->day_last_game_started + one_day) && is_playing(state))) {
        state->streak = 0;
        reset_board(state);
    }
#endif
    state->using_random_guess = false;
    if (is_playing(state) && state->curr_screen >= WORDLE_SCREEN_RESULT) {
        reset_incorrect_elements(state);
        state->position = get_first_pos(state->word_elements_result); 
    }
    movement_request_tick_frequency(WORDLE_FREQ);
    watch_clear_all_indicators();
    watch_clear_colon();
    display_title(state);
}

void wordle_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(wordle_state_t));
        memset(*context_ptr, 0, sizeof(wordle_state_t));
        wordle_state_t *state = (wordle_state_t *)*context_ptr;
        state->curr_screen = WORDLE_SCREEN_TITLE;
        state->skip_wrong_letter = true;
        reset_all_elements(state);
        memset(state->not_to_use, 0xff, sizeof(state->not_to_use));
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

void wordle_face_activate(void *context) {
    wordle_state_t *state = (wordle_state_t *)context;
    _activate(state);

}

bool wordle_face_loop(movement_event_t event, void *context) {
    wordle_state_t *state = (wordle_state_t *)context;

    switch (event.event_type) {
        case EVENT_TICK:
            if (state->ignore_btn_ticks > 0) state->ignore_btn_ticks--;
            switch (state->curr_screen)
            {
            case WORDLE_SCREEN_PLAYING:
                if (event.subsecond % 2) {
                    display_letter(state, true);
                } else {
                    watch_display_character(' ', state->position + 5);
                }
                break;
            case WORDLE_SCREEN_RESULT:
                display_result(state, event.subsecond);
                break;
            case WORDLE_SCREEN_LOSE:
                display_lose(state, event.subsecond);
                break;
            case WORDLE_SCREEN_WIN:
                display_win(state, event.subsecond);
                break;
            default:
                break;
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if (act_on_btn(state, BTN_LIGHT)) break;
            get_next_letter(state->position, state->word_elements, state->known_wrong_letters, state->skip_wrong_letter);
            display_letter(state, true);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (state->curr_screen < WORDLE_SCREEN_PLAYING) {
                state->skip_wrong_letter = !state->skip_wrong_letter;
                show_skip_wrong_letter_indicator(state->skip_wrong_letter, state->curr_screen);
                break;
            }
            if (state->curr_screen != WORDLE_SCREEN_PLAYING) break;
            get_prev_letter(state->position, state->word_elements, state->known_wrong_letters, state->skip_wrong_letter);
            display_letter(state, true);
            break; 
        case EVENT_ALARM_BUTTON_UP:
            if (act_on_btn(state, BTN_ALARM)) break;
            display_letter(state, true);
            if (state->word_elements[state->position] == WORDLE_NUM_VALID_LETTERS) break;
#if (WORDLE_USE_RANDOM_GUESS != 0)
            if (HAL_GPIO_BTN_LIGHT_read() &&
            (state->using_random_guess || (state->attempt == 0 && state->position == 0))) {
                insert_random_guess(state);
                break;
            }
#endif
            state->position = get_next_pos(state->position, state->word_elements_result);
            if (state->position >= WORDLE_LENGTH) {
                get_result(state);
                state->using_random_guess = false;
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (state->curr_screen != WORDLE_SCREEN_PLAYING) break;
            display_letter(state, true);
            state->position = get_prev_pos(state->position, state->word_elements_result);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
        case EVENT_ACTIVATE:
            break;
        case EVENT_TIMEOUT:
            if (state->curr_screen >= WORDLE_SCREEN_RESULT) {
                reset_incorrect_elements(state);
                state->position = get_first_pos(state->word_elements_result); 
                display_title(state);
            }
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            if (state->curr_screen != WORDLE_SCREEN_TITLE)
                display_title(state);
            break;
        case EVENT_MODE_LONG_PRESS:
            if (state->curr_screen >= WORDLE_SCREEN_PLAYING) {
                _activate(state);
            } else {
                movement_move_to_face(0);
            }
            break;
            default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void wordle_face_resign(void *context) {
    (void) context;
}

