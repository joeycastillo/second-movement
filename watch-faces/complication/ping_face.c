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
#include "ping_face.h"
#include "delay.h"
#include "watch_common_display.h"

typedef enum {
    PADDLE_RETRACTED = 0,
    PADDLE_EXTENDING,
    PADDLE_EXTENDED,
    PADDLE_RETRACTING,
} PingPaddleState;

typedef enum {
    SCREEN_TITLE = 0,
    SCREEN_SCORE,
    SCREEN_PLAYING,
    SCREEN_LOSE,
    SCREEN_COUNT
} PingCurrScreen;

typedef enum {
    DIFF_BABY = 0,  // FREQ_BABY FPS
    DIFF_EASY,      // FREQ_EASY FPS
    DIFF_NORM,      // FREQ_NORM FPS
    DIFF_HARD,      // FREQ_NORM FPS, smaller travel-distance for ball
    DIFF_COUNT
} PingDifficulty;

typedef enum {
    RESULT_LOSE = -1,
    RESULT_NONE = 0,
    RESULT_HIT = 1,
    RESULT_FIRST_HIT = 2,
} PingResult;

#define FREQ_BABY 2
#define FREQ_EASY 4
#define FREQ_NORM 8

#define BALL_POS_MAX 11
#define BALL_OFF_SCREEN 100
#define MAX_HI_SCORE 9999  // Max hi score to store and display on the title screen.
#define MAX_DISP_SCORE 39  // The top-right digits can't properly display above 39

typedef struct {
    uint8_t ball_pos;  // 0 to 11; 0 is the bottom-right and 11 is the top right.
    // | 6 | 7 | 8 | 9 | 10 | 11 |
    // | 5 | 4 | 3 | 2 |  1 |  0 |
    PingPaddleState paddle_pos;
    uint8_t ball_char_pos;  // Derived from ball_pos
    bool ball_is_clockwise;
    bool ball_is_moving;
    uint16_t curr_score;
    PingCurrScreen curr_screen;
    bool paddle_hit;
    bool paddle_released;
    uint8_t curr_freq;
    bool moving_from_tap;
} game_state_t;

static game_state_t game_state;
static int8_t _ticks_show_title = 0;
static bool _is_custom_lcd;

static int8_t start_tune[] = {
    BUZZER_NOTE_C5, 15,
    BUZZER_NOTE_E5, 15,
    BUZZER_NOTE_G5, 15,
    0
};

static int8_t lose_tune[] = {
    BUZZER_NOTE_D3, 10,
    BUZZER_NOTE_C3SHARP_D3FLAT, 10,
    BUZZER_NOTE_C3, 10,
    0
};

static uint8_t ball_pos_to_char_pos(uint8_t ball_pos) {
    switch (ball_pos)
    {
    case 5:
    case 6:
        return 4;
    case 4:
    case 7:
        return 5;
    case 3:
    case 8:
        return 6;
    case 2:
    case 9:
        return 7;
    case 1:
    case 10:
        return 8;
    case 0:
    case 11:
        return 9;
    default:
        return BALL_OFF_SCREEN;
    }
}

static bool paddle_and_ball_on_same_segment(void) {
    if (game_state.paddle_pos == PADDLE_EXTENDED) {
        if (game_state.ball_pos == 9 || game_state.ball_pos == 2) {
            return true;
        }
    }
    else if (game_state.paddle_pos == PADDLE_EXTENDING || game_state.paddle_pos == PADDLE_RETRACTING) {
        if (game_state.ball_pos == 10 || game_state.ball_pos == 1) {
            return true;
        }
    }
    else if (game_state.paddle_pos == PADDLE_RETRACTED) {
        if (game_state.ball_pos == 11 || game_state.ball_pos == 0) {
            return true;
        }
    }
    return false;
}

static bool paddle_hit_ball(void) {
    if (game_state.paddle_pos == PADDLE_EXTENDED) {
        if (game_state.ball_pos >= 9 && game_state.ball_is_clockwise) {
            return true;
        }
        if (game_state.ball_pos <= 2 && !game_state.ball_is_clockwise) {
            return true;
        }
    }
    else if (game_state.paddle_pos == PADDLE_EXTENDING) {
        if (game_state.ball_pos >= 10 && game_state.ball_is_clockwise) {
            return true;
        }
        if (game_state.ball_pos <= 1 && !game_state.ball_is_clockwise) {
            return true;
        }
    }
    return false;
}

static uint8_t get_next_ball_pos(bool ball_hit, uint8_t difficulty) {
    int8_t offset_next;
    if (ball_hit) {
        bool ball_on_top = game_state.ball_pos > 5;
        game_state.ball_is_clockwise = !ball_on_top;
        // ball is at the same frame as the paddle
        if (game_state.paddle_pos == PADDLE_EXTENDED) {
            return ball_on_top ? 9 : 2;
        } else if (game_state.paddle_pos == PADDLE_EXTENDING) {
            return ball_on_top ? 10 : 1;
        }
    }
    if (game_state.ball_is_clockwise) {
        offset_next = 1;
    } else {
        offset_next = -1;
    }
    int8_t next_pos = game_state.ball_pos + offset_next;
    if (next_pos > BALL_POS_MAX || next_pos < 0) {
        return BALL_OFF_SCREEN;
    }
    if (difficulty == DIFF_HARD) {
        if (next_pos == 4) {
            next_pos = 8;
        } else if (next_pos == 7) {
            next_pos = 3;
        }
    }
    return next_pos;
}

static void display_ball(void) {
    uint8_t char_pos = ball_pos_to_char_pos(game_state.ball_pos);
    uint8_t char_display;
    bool overlap = paddle_and_ball_on_same_segment();
    if (game_state.ball_pos > 5) {
        if (overlap) {
            char_display = 'q';
        } else {
            char_display = '#';
        }
    } else {
        if (!_is_custom_lcd && (char_pos == 4 || char_pos == 6)) {
            char_display = 'n'; // No need to check for overlap on these segments
        } else {
            if (overlap) {
                char_display = 'd';
            } else {
                char_display = 'o';
            }
        }
    }
    watch_display_character(char_display, char_pos);
}

static PingResult update_ball(uint8_t difficulty) {
    bool ball_hit = paddle_hit_ball();
    bool first_hit = false;
    if (!game_state.ball_is_moving) {
        if (ball_hit) {
            game_state.ball_is_moving = true;
            first_hit = true;
        } else {
            return RESULT_NONE;
        }
    }
    game_state.ball_pos = get_next_ball_pos(ball_hit, difficulty);
    if (game_state.ball_pos == BALL_OFF_SCREEN) {
        return RESULT_LOSE;
    }
    display_ball();
    if (ball_hit) {
        return first_hit ? RESULT_FIRST_HIT : RESULT_HIT;
    } else {
        return RESULT_NONE;
    }
}

static void display_paddle(void) {
    switch (game_state.paddle_pos)
    {
    case PADDLE_EXTENDING:
    case PADDLE_RETRACTING:
        watch_display_character('-', 9);
        watch_display_character('1', 8);
        break;
    case PADDLE_EXTENDED:
        watch_display_character('-', 9);
        watch_display_character('-', 8);
        watch_display_character('1', 7);
        break;
    case PADDLE_RETRACTED:
    default:
        watch_display_character('1', 9);
        break;
    }
}

static void update_paddle(void) {
    switch (game_state.paddle_pos)
    {
    case PADDLE_RETRACTED:
        if (game_state.paddle_hit) {
            game_state.paddle_pos = PADDLE_EXTENDING;
        }
        break;
    case PADDLE_EXTENDING:
        if (!game_state.moving_from_tap && !HAL_GPIO_BTN_ALARM_read()) {
            game_state.paddle_pos = PADDLE_RETRACTED;
            watch_display_character(' ', 8);
            game_state.moving_from_tap = false;
        } else {
            game_state.paddle_pos = PADDLE_EXTENDED;
        }
        break;
    case PADDLE_EXTENDED:
        game_state.paddle_pos = PADDLE_RETRACTING;
        watch_display_character(' ', 7);
        break;
    case PADDLE_RETRACTING:
        game_state.paddle_pos = PADDLE_RETRACTED;
        watch_display_character(' ', 8);
        game_state.moving_from_tap = false;
        break;
    default:
        break;
    }
    game_state.paddle_hit = false;
    display_paddle();
}

static inline bool paddle_is_extending(void) {
    return game_state.paddle_pos == PADDLE_EXTENDING || game_state.paddle_pos == PADDLE_EXTENDED;
}

static void display_score(uint8_t score) {
    char buf[3];
    score %= (MAX_DISP_SCORE + 1);
    sprintf(buf, "%2d", score);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
}

static void add_to_score(ping_state_t *state) {
    if (game_state.curr_score <= MAX_HI_SCORE) {
        game_state.curr_score++;
        if (game_state.curr_score > state -> hi_score)
            state -> hi_score = game_state.curr_score;
    }
    display_score(game_state.curr_score);
}

static void check_and_reset_hi_score(ping_state_t *state) {
    // Resets the hi score at the beginning of each month.
    watch_date_time_t date_time = movement_get_local_date_time();
    if ((state -> year_last_hi_score != date_time.unit.year) || 
        (state -> month_last_hi_score != date_time.unit.month))
    {
        // The high score resets itself every new month.
        state -> hi_score = 0;
        state -> year_last_hi_score = date_time.unit.year;
        state -> month_last_hi_score = date_time.unit.month;
    }
}

static void display_difficulty(uint16_t difficulty) {
    static const char *labels[] = {
        [DIFF_BABY]   = " b",
        [DIFF_EASY]   = " E",
        [DIFF_NORM]   = " N",
        [DIFF_HARD]   = " H"
    };
    watch_display_text(WATCH_POSITION_TOP_RIGHT, labels[difficulty]);
}

static void change_difficulty(ping_state_t *state) {
    state -> difficulty = (state -> difficulty + 1) % DIFF_COUNT;
    display_difficulty(state -> difficulty);
    if (state -> soundOn) {
        if (state -> difficulty == 0) watch_buzzer_play_note(BUZZER_NOTE_B4, 30);
        else  watch_buzzer_play_note(BUZZER_NOTE_C5, 30);
    }
}

static void display_sound_indicator(bool soundOn) {
    if (soundOn) {
        watch_set_indicator(WATCH_INDICATOR_BELL);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_BELL);
    }
}

static void toggle_sound(ping_state_t *state) {
    state -> soundOn = !state -> soundOn;
    display_sound_indicator(state -> soundOn);
    if (state -> soundOn) {
        watch_buzzer_play_note(BUZZER_NOTE_C5, 30);
    }
}

static void enable_tap_control(ping_state_t *state) {
    if (!state->tap_control_on) {
        movement_enable_tap_detection_if_available();
        state->tap_control_on = true;
    }
}

static void disable_tap_control(ping_state_t *state) {
    if (state->tap_control_on) {
        movement_disable_tap_detection_if_available();
        state->tap_control_on = false;
    }
}

static void display_title(ping_state_t *state) {
    movement_request_tick_frequency(1);
    game_state.curr_screen = SCREEN_TITLE;
    watch_clear_colon();
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "Ping", "PI  ");
    watch_display_text(WATCH_POSITION_BOTTOM, " Ping ");
    display_sound_indicator(state -> soundOn);
    _ticks_show_title = 1;
}

static void display_score_screen(ping_state_t *state) {
    uint16_t hi_score = state -> hi_score;
    uint8_t difficulty = state -> difficulty;
    movement_request_tick_frequency(1);
    bool sound_on = state -> soundOn;
    memset(&game_state, 0, sizeof(game_state));
    game_state.curr_screen = SCREEN_SCORE;
    watch_set_colon();
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "PI  ", "PI  ");
    if (hi_score > MAX_HI_SCORE) {
        watch_display_text(WATCH_POSITION_BOTTOM, "HS  --");
    }
    else {
        char buf[10];
        sprintf(buf, "HS%4d", hi_score);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    }
    display_difficulty(difficulty);
    display_sound_indicator(sound_on);
}

static void begin_playing(ping_state_t *state) {
    game_state.curr_screen = SCREEN_PLAYING;
    watch_clear_colon();
    display_sound_indicator(state -> soundOn);
    switch (state -> difficulty)
    {
    case DIFF_BABY:
        game_state.curr_freq = FREQ_BABY;
        break;
    case DIFF_EASY:
        game_state.curr_freq = FREQ_EASY;
        break;
    case DIFF_NORM:
    case DIFF_HARD:
    default:
        game_state.curr_freq = FREQ_NORM;
        break;
    }
    movement_request_tick_frequency(game_state.curr_freq);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    watch_display_text(WATCH_POSITION_BOTTOM, "      ");
    game_state.paddle_pos = PADDLE_RETRACTED;
    game_state.ball_pos = 1;
    game_state.paddle_hit = false;
    game_state.ball_is_moving = false;
    game_state.ball_is_clockwise = false;
    game_state.curr_score = 0;
    display_paddle();
    display_ball();
    display_score( game_state.curr_score);
}

static void display_lose_screen(ping_state_t *state) {
    game_state.curr_screen = SCREEN_LOSE;
    game_state.curr_score = 0;
    watch_clear_display();
    watch_display_text(WATCH_POSITION_BOTTOM, " LOSE ");
    if (state -> soundOn) {
        watch_buzzer_play_sequence(lose_tune, NULL);
        delay_ms(600);
    }
}

static void update_game(ping_state_t *state) {
    if (game_state.ball_is_moving) {
        watch_display_character(' ', ball_pos_to_char_pos(game_state.ball_pos)); // remove the old ball.
    }
    update_paddle();
    int game_result = update_ball(state -> difficulty);
    if (game_result == RESULT_LOSE) {
        display_lose_screen(state);
    } else if (game_result == RESULT_HIT) {
        add_to_score(state);
        if (state -> soundOn) {
            watch_buzzer_play_note(BUZZER_NOTE_C5, 60);
        }
    } else if (game_result == RESULT_FIRST_HIT && state -> soundOn) {
        watch_buzzer_play_sequence(start_tune, NULL);
    }
}

void ping_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(ping_state_t));
        memset(*context_ptr, 0, sizeof(ping_state_t));
        ping_state_t *state = (ping_state_t *)*context_ptr;
        state->difficulty = DIFF_NORM;
        state->tap_control_on = false;
    }
}

void ping_face_activate(void *context) {
    (void) context;
    _is_custom_lcd = watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM;
    if (watch_sleep_animation_is_running()) {
        watch_stop_blink();
    }
}

bool ping_face_loop(movement_event_t event, void *context) {
    ping_state_t *state = (ping_state_t *)context;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            disable_tap_control(state);
            check_and_reset_hi_score(state);
            display_title(state);
            break;
        case EVENT_TICK:
            switch (game_state.curr_screen)
            {
            case SCREEN_TITLE:
                if (_ticks_show_title > 0) {_ticks_show_title--;}
                else {
                    watch_clear_display();
                    display_score_screen(state);
                }
            case SCREEN_SCORE:
            case SCREEN_LOSE:
                break;
            case SCREEN_PLAYING:
            default:
                update_game(state);
                break;
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
        case EVENT_LIGHT_BUTTON_UP:
            switch (game_state.curr_screen) {
                case SCREEN_SCORE:
                    enable_tap_control(state);
                    begin_playing(state);
                    break;
                case SCREEN_TITLE:
                    enable_tap_control(state);
                    // fall through
                case SCREEN_LOSE:
                    watch_clear_display();
                    display_score_screen(state);
                default:
                    break;
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (game_state.curr_screen == SCREEN_SCORE)
                change_difficulty(state);
            break;
        case EVENT_SINGLE_TAP:
        case EVENT_DOUBLE_TAP:
            // Allow starting a new game by tapping.
            if (game_state.curr_screen == SCREEN_SCORE) {
                begin_playing(state);
                break;
            }
            else if (game_state.curr_screen == SCREEN_LOSE) {
                display_score_screen(state);
                break;
            }
            else if (game_state.curr_screen == SCREEN_PLAYING) {
                game_state.moving_from_tap = true;
                game_state.paddle_hit = true;
            }
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            if (game_state.curr_screen == SCREEN_PLAYING) {
                game_state.moving_from_tap = false;
                game_state.paddle_hit = true;
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (game_state.curr_screen == SCREEN_TITLE || game_state.curr_screen == SCREEN_SCORE)
                toggle_sound(state);
            break;
        case EVENT_TIMEOUT:
            disable_tap_control(state);
            if (game_state.curr_screen != SCREEN_SCORE) {
                display_score_screen(state);
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void ping_face_resign(void *context) {
    ping_state_t *state = (ping_state_t *)context;
    disable_tap_control(state);
}
