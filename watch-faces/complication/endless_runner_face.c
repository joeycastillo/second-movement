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
#include "endless_runner_face.h"
#include "delay.h"

typedef enum {
    JUMPING_FINAL_FRAME = 0,
    NOT_JUMPING,
    JUMPING_START,
} RunnerJumpState;

typedef enum {
    SCREEN_TITLE = 0,
    SCREEN_SCORE,
    SCREEN_PLAYING,
    SCREEN_LOSE,
    SCREEN_TIME,
    SCREEN_COUNT
} RunnerCurrScreen;

typedef enum {
    DIFF_BABY = 0,  // FREQ_SLOW FPS;       MIN_ZEROES 0's min;  Jump is JUMP_FRAMES_EASY frames
    DIFF_EASY,      //      FREQ FPS;       MIN_ZEROES 0's min;  Jump is JUMP_FRAMES_EASY frames
    DIFF_NORM,      //      FREQ FPS;       MIN_ZEROES 0's min;  Jump is JUMP_FRAMES frames
    DIFF_HARD,      //      FREQ FPS;  MIN_ZEROES_HARD 0's min;  jump is JUMP_FRAMES frames
    DIFF_FUEL,      // Mode where the top-right displays the amoount of fuel that you can be above the ground for, dodging obstacles. When on the ground, your fuel recharges.
    DIFF_FUEL_1,    // Same as DIFF_FUEL, but if your fuel is 0, then you won't recharge
    DIFF_COUNT
} RunnerDifficulty;

#define NUM_GRID 12  // This the length that the obstacle track can be on
#define FREQ 8  // Frequency request for the game
#define FREQ_SLOW 4  // Frequency request for baby mode
#define JUMP_FRAMES 2  // Wait this many frames on difficulties above EASY before coming down from the jump button pressed
#define JUMP_FRAMES_EASY 3 // Wait this many frames on difficulties at or below EASY before coming down from the jump button pressed
#define MIN_ZEROES 4  // At minimum, we'll have this many spaces between obstacles
#define MIN_ZEROES_HARD 3 // At minimum, we'll have this many spaces between obstacles on hard mode
#define MAX_HI_SCORE 9999  // Max hi score to store and display on the title screen.
#define MAX_DISP_SCORE 39  // The top-right digits can't properly display above 39
#define JUMP_FRAMES_FUEL 30  // The max fuel that fuel that the fuel mode game will hold
#define JUMP_FRAMES_FUEL_RECHARGE 3 // How much fuel each frame on the ground adds
#define MAX_DISP_SCORE_FUEL 9  // Since the fuel mode displays the score in the weekday slot, two digits will display wonky data

typedef struct {
    uint32_t obst_pattern;
    uint16_t obst_indx : 8;
    uint16_t jump_state : 5;
    uint16_t sec_before_moves : 3;
    uint16_t curr_score : 10;
    uint16_t curr_screen : 4;
    bool loc_2_on;
    bool loc_3_on;
    bool success_jump;
    bool fuel_mode;
    uint8_t fuel;
} game_state_t;

// always-on, left, right, bottom, jump-top, jump-left, jump-right
int8_t classic_ball_arr_com[] = {1, 0, 1, 0, 2, 1, 2};
int8_t classic_ball_arr_seg[] = {20, 20, 21, 21, 20, 17, 21};
int8_t custom_ball_arr_com[] = {2, 1, 1, 0, 3, 3, 2};
int8_t custom_ball_arr_seg[] = {15, 15, 14, 15, 14, 15, 14};

// obstacle 0-11
int8_t classic_obstacle_arr_com[] = {0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1};
int8_t classic_obstacle_arr_seg[] = {18, 19, 20, 21, 22, 23, 0, 1, 2, 4, 5, 6};
int8_t custom_obstacle_arr_com[] = {1, 1, 1, 1, 1, 0, 1, 0, 3, 0, 0, 2};
int8_t custom_obstacle_arr_seg[] = {22, 16, 15, 14, 1, 2, 3, 4, 4, 5, 6, 7};

int8_t *ball_arr_com;
int8_t *ball_arr_seg;
int8_t *obstacle_arr_com;
int8_t *obstacle_arr_seg;

static game_state_t game_state;
static const uint8_t _num_bits_obst_pattern = sizeof(game_state.obst_pattern) * 8;

int8_t start_tune[] = {
    BUZZER_NOTE_C5, 15,
    BUZZER_NOTE_E5, 15,
    BUZZER_NOTE_G5, 15,
    0
};

int8_t lose_tune[] = {
    BUZZER_NOTE_D3, 10,
    BUZZER_NOTE_C3SHARP_D3FLAT, 10,
    BUZZER_NOTE_C3, 10,
    0
};

static void print_binary(uint32_t value, int bits) {
#if __EMSCRIPTEN__
    for (int i = bits - 1; i >= 0; i--) {
        // Print each bit
        printf("%u", (value >> i) & 1);
        // Optional: add a space every 4 bits for readability
        if (i % 4 == 0 && i != 0) {
            printf(" ");
        }
    }
    printf("\r\n");
#else
    (void) value;
    (void) bits;
#endif
    return;
}

static uint32_t get_random(uint32_t max) {
    #if __EMSCRIPTEN__
    return rand() % max;
    #else
    return arc4random_uniform(max);
    #endif
}

static uint32_t get_random_nonzero(uint32_t max) {
    uint32_t random;
    do
    {
        random = get_random(max);
    } while (random == 0);
    return random;
}

static uint32_t get_random_kinda_nonzero(uint32_t max) {
    // Returns a number that's between 1 and max, unless max is 0 or 1, then it returns 0 to max.
    if (max == 0) return 0;
    else if (max == 1) return get_random(max);
    return get_random_nonzero(max);
}

static uint32_t get_random_fuel(uint32_t prev_val) {
    static uint8_t prev_rand_subset = 0;
    uint32_t rand;
    uint8_t max_ones, subset;
    uint32_t rand_legal = 0;
    prev_val = prev_val & ~0xFFFF;

    for (int i = 0; i < 2; i++) {
        subset = 0;
        max_ones = 8;
        if (prev_rand_subset > 4)
            max_ones -= prev_rand_subset;
        rand = get_random_kinda_nonzero(max_ones);
        if (rand > 5 && prev_rand_subset) rand = 5;  // The gap of one or two is awkward
        for (uint32_t j = 0; j < rand; j++) {
            subset |= (1 << j);
        }
        if (prev_rand_subset >= 7)
            subset = subset << 1;
        subset &= 0xFF;
        rand_legal |= subset << (8 * i);
        prev_rand_subset = rand;
    }

    rand_legal = prev_val | rand_legal;
    print_binary(rand_legal, 32);
    return rand_legal;
}

static uint32_t get_random_legal(uint32_t prev_val, uint16_t difficulty) {
/** @brief A legal random number starts with the previous number (which should be the 12 bits on the screen).
  * @param prev_val The previous value to tack onto. The return will have its first NUM_GRID MSBs be the same as prev_val, and the rest be new
  * @param difficulty To dictate how spread apart the obsticles must be
  * @return the new random value, where it's first NUM_GRID MSBs are the same as prev_val
  */
    uint8_t min_zeros = (difficulty == DIFF_HARD) ? MIN_ZEROES_HARD : MIN_ZEROES;
    uint32_t max = (1 << (_num_bits_obst_pattern - NUM_GRID)) - 1;
    uint32_t rand = get_random_nonzero(max);
    uint32_t rand_legal = 0;
    prev_val = prev_val & ~max;

    for (int i = (NUM_GRID + 1); i <= _num_bits_obst_pattern; i++) {
        uint32_t mask = 1 << (_num_bits_obst_pattern - i);
        bool msb = (rand & mask) >> (_num_bits_obst_pattern - i);
        if (msb) {
            rand_legal = rand_legal << min_zeros;
            i+=min_zeros;
        }
        rand_legal |= msb;
        rand_legal = rand_legal << 1;
    }

    rand_legal = rand_legal & max;
    for (int i = 0; i <= min_zeros; i++) {
        if (prev_val & (1 << (i + _num_bits_obst_pattern - NUM_GRID))){
            rand_legal = rand_legal >> (min_zeros - i);
            break;
        }
    }
    rand_legal = prev_val | rand_legal;
    print_binary(rand_legal, 32);
    return rand_legal;
}

static void display_ball(bool jumping) {
    if (!jumping) {
        watch_set_pixel(ball_arr_com[3], ball_arr_seg[3]);
        watch_set_pixel(ball_arr_com[2], ball_arr_seg[2]);
        watch_set_pixel(ball_arr_com[1], ball_arr_seg[1]);
        watch_set_pixel(ball_arr_com[0], ball_arr_seg[0]);
        watch_clear_pixel(ball_arr_com[6], ball_arr_seg[6]);
        watch_clear_pixel(ball_arr_com[5], ball_arr_seg[5]);
        watch_clear_pixel(ball_arr_com[4], ball_arr_seg[4]);
    }
    else {
        watch_clear_pixel(ball_arr_com[3], ball_arr_seg[3]);
        watch_clear_pixel(ball_arr_com[2], ball_arr_seg[2]);
        watch_clear_pixel(ball_arr_com[1], ball_arr_seg[1]);
        watch_set_pixel(ball_arr_com[0], ball_arr_seg[0]);
        watch_set_pixel(ball_arr_com[6], ball_arr_seg[6]);
        watch_set_pixel(ball_arr_com[5], ball_arr_seg[5]);
        watch_set_pixel(ball_arr_com[4], ball_arr_seg[4]);
    }
}

static void display_score(uint8_t score) {
    char buf[3];
    if (game_state.fuel_mode) {
        score %= (MAX_DISP_SCORE_FUEL + 1);
        sprintf(buf, "%1d", score);
        watch_display_text(WATCH_POSITION_TOP_LEFT, buf);
    }
    else {
        score %= (MAX_DISP_SCORE + 1);
        sprintf(buf, "%2d", score);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }
}

static void add_to_score(endless_runner_state_t *state) {
    if (game_state.curr_score <= MAX_HI_SCORE) {
        game_state.curr_score++;
        if (game_state.curr_score > state -> hi_score)
            state -> hi_score = game_state.curr_score;
    }
    game_state.success_jump = true;
    display_score(game_state.curr_score);
}

static void display_fuel(uint8_t subsecond, uint8_t difficulty) {
    char buf[4];
    if (difficulty == DIFF_FUEL_1 && game_state.fuel == 0 && subsecond % (FREQ/2) == 0) {
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");  // Blink the 0 fuel to show it cannot be refilled.
        return;
    }
    sprintf(buf, "%2d", game_state.fuel);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
}

static void check_and_reset_hi_score(endless_runner_state_t *state) {
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
        [DIFF_HARD]   = " H",
        [DIFF_FUEL]   = " F",
        [DIFF_FUEL_1] = "1F",
        [DIFF_NORM]   = " N"
    };
    watch_display_text(WATCH_POSITION_TOP_RIGHT, labels[difficulty]);
    game_state.fuel_mode = difficulty >= DIFF_FUEL && difficulty <= DIFF_FUEL_1;
}

static void change_difficulty(endless_runner_state_t *state) {
    state -> difficulty = (state -> difficulty + 1) % DIFF_COUNT;
    display_difficulty(state -> difficulty);
    if (state -> soundOn) {
        if (state -> difficulty == 0) watch_buzzer_play_note(BUZZER_NOTE_B4, 30);
        else  watch_buzzer_play_note(BUZZER_NOTE_C5, 30);
    }
}

static void display_sound_indicator(bool soundOn) {
    if (soundOn){
        watch_set_indicator(WATCH_INDICATOR_BELL);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_BELL);
    }
}

static void toggle_sound(endless_runner_state_t *state) {
    state -> soundOn = !state -> soundOn;
    display_sound_indicator(state -> soundOn);
    if (state -> soundOn){
        watch_buzzer_play_note(BUZZER_NOTE_C5, 30);
    }
}

static void enable_tap_control(endless_runner_state_t *state) {
    if (!state->tap_control_on) {
        movement_enable_tap_detection_if_available();
        state->tap_control_on = true;
    }
}

static void disable_tap_control(endless_runner_state_t *state) {
    if (state->tap_control_on) {
        movement_disable_tap_detection_if_available();
        state->tap_control_on = false;
    }
}

static void display_title(endless_runner_state_t *state) {
    game_state.curr_screen = SCREEN_TITLE;
    watch_clear_colon();
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "ENdLS", "ER  ");
    watch_display_text(WATCH_POSITION_BOTTOM, "RUNNER");
    display_sound_indicator(state -> soundOn);
}

static void display_score_screen(endless_runner_state_t *state) {
    uint16_t hi_score = state -> hi_score;
    uint8_t difficulty = state -> difficulty;
    bool sound_on = state -> soundOn;
    memset(&game_state, 0, sizeof(game_state));
    game_state.curr_screen = SCREEN_SCORE;
    game_state.sec_before_moves = 1; // The first obstacles will all be 0s, which is about an extra second of delay.
    if (sound_on) game_state.sec_before_moves--; // Start chime is about 1 second
    watch_set_colon();
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "RUN  ", "ER  ");
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

static void display_time(void) {
    static watch_date_time_t previous_date_time;
    watch_date_time_t date_time = movement_get_local_date_time();
    movement_clock_mode_t clock_mode_24h = movement_clock_mode_24h();
    char buf[6 + 1];

    // If the hour needs updating or it's the first time displaying the time
    if ((game_state.curr_screen != SCREEN_TIME) || (date_time.unit.hour != previous_date_time.unit.hour)) {
        uint8_t hour = date_time.unit.hour;
        game_state.curr_screen = SCREEN_TIME;
        if (!watch_sleep_animation_is_running()) {
            watch_set_colon();
            watch_start_indicator_blink_if_possible(WATCH_INDICATOR_COLON, 500);
        }
        if (clock_mode_24h != MOVEMENT_CLOCK_MODE_12H) watch_set_indicator(WATCH_INDICATOR_24H);
        else {
            if (hour >= 12) watch_set_indicator(WATCH_INDICATOR_PM);
            hour %= 12;
            if (hour == 0) hour = 12;
        }
        sprintf( buf, clock_mode_24h == MOVEMENT_CLOCK_MODE_024H ? "%02d%02d  " : "%2d%02d  ", hour, date_time.unit.minute);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    }
    // If only the minute need updating
    else {
        sprintf( buf, "%02d", date_time.unit.minute);
        watch_display_text(WATCH_POSITION_MINUTES, buf);
    }
    previous_date_time.reg = date_time.reg;
}

static void begin_playing(endless_runner_state_t *state) {
    uint8_t difficulty = state -> difficulty;
    game_state.curr_screen = SCREEN_PLAYING;
    watch_clear_colon();
    display_sound_indicator(state -> soundOn);
    movement_request_tick_frequency((state -> difficulty == DIFF_BABY) ? FREQ_SLOW : FREQ);
    if (game_state.fuel_mode) {
        watch_clear_display();
        game_state.obst_pattern = get_random_fuel(0);
        if ((16 * JUMP_FRAMES_FUEL_RECHARGE) < JUMP_FRAMES_FUEL) // 16 frames of zeros at the start of a level
            game_state.fuel = JUMP_FRAMES_FUEL - (16 * JUMP_FRAMES_FUEL_RECHARGE); // Have it below its max to show it counting up when starting.
        if (game_state.fuel < JUMP_FRAMES_FUEL_RECHARGE) game_state.fuel = JUMP_FRAMES_FUEL_RECHARGE;
    }
    else {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
        watch_display_text(WATCH_POSITION_BOTTOM, "      ");
        game_state.obst_pattern = get_random_legal(0, difficulty);
    }
    game_state.jump_state = NOT_JUMPING;
    display_ball(game_state.jump_state != NOT_JUMPING);
    display_score( game_state.curr_score);
    if (state -> soundOn){
        watch_buzzer_play_sequence(start_tune, NULL);
    }
}

static void display_lose_screen(endless_runner_state_t *state) {
    game_state.curr_screen = SCREEN_LOSE;
    game_state.curr_score = 0;
    watch_clear_display();
    watch_display_text(WATCH_POSITION_BOTTOM, " LOSE ");
    if (state -> soundOn) {
        watch_buzzer_play_sequence(lose_tune, NULL);
        delay_ms(600);
    }
}

static void display_obstacle(bool obstacle, int grid_loc, endless_runner_state_t *state) {
    static bool prev_obst_pos_two = 0;
    switch (grid_loc)
    {
    case 2:
        game_state.loc_2_on = obstacle;
        if (obstacle)
            watch_set_pixel(obstacle_arr_com[grid_loc], obstacle_arr_seg[grid_loc]);
        else if (game_state.jump_state != NOT_JUMPING) {
            watch_clear_pixel(obstacle_arr_com[grid_loc], obstacle_arr_seg[grid_loc]);
            if (game_state.fuel_mode && prev_obst_pos_two)
                add_to_score(state);
        }
        prev_obst_pos_two = obstacle;
        break;
    case 3:
        game_state.loc_3_on = obstacle;
        if (obstacle)
            watch_set_pixel(obstacle_arr_com[grid_loc], obstacle_arr_seg[grid_loc]);
        else if (game_state.jump_state != NOT_JUMPING)
            watch_clear_pixel(obstacle_arr_com[grid_loc], obstacle_arr_seg[grid_loc]);
        break;
    
    case 1:
        if (!game_state.fuel_mode && obstacle)  // If an obstacle is here, it means the ball cleared it
            add_to_score(state);
        //fall through
    default:
        if (obstacle)
            watch_set_pixel(obstacle_arr_com[grid_loc], obstacle_arr_seg[grid_loc]);
        else
            watch_clear_pixel(obstacle_arr_com[grid_loc], obstacle_arr_seg[grid_loc]);
        break;
    }
}

static void stop_jumping(endless_runner_state_t *state) {
    game_state.jump_state = NOT_JUMPING;
    display_ball(game_state.jump_state != NOT_JUMPING);
    if (state -> soundOn){
        if (game_state.success_jump)
            watch_buzzer_play_note(BUZZER_NOTE_C5, 60);
        else
            watch_buzzer_play_note(BUZZER_NOTE_C3, 60);
    }
    game_state.success_jump = false;
}

static void display_obstacles(endless_runner_state_t *state) {
    for (int i = 0; i < NUM_GRID; i++) {
        // Use a bitmask to isolate each bit and shift it to the least significant position
        uint32_t mask = 1 << ((_num_bits_obst_pattern - 1) - i);
        bool obstacle = (game_state.obst_pattern & mask) >> ((_num_bits_obst_pattern - 1) - i);
        display_obstacle(obstacle, i, state);
    }
    game_state.obst_pattern = game_state.obst_pattern << 1;
    game_state.obst_indx++;
    if (game_state.fuel_mode) {
        if (game_state.obst_indx >= (_num_bits_obst_pattern / 2)) {
                game_state.obst_indx = 0;
                game_state.obst_pattern = get_random_fuel(game_state.obst_pattern);
        }
    }
    else if (game_state.obst_indx >= _num_bits_obst_pattern - NUM_GRID) {
        game_state.obst_indx = 0;
        game_state.obst_pattern = get_random_legal(game_state.obst_pattern, state -> difficulty);
    }
}

static void update_game(endless_runner_state_t *state, uint8_t subsecond) {
    uint8_t curr_jump_frame = 0;
    if (game_state.sec_before_moves != 0) {
        if (subsecond == 0) --game_state.sec_before_moves;
        return;
    }
    display_obstacles(state);
    switch (game_state.jump_state)
    {
    case NOT_JUMPING:
        if (game_state.fuel_mode) {
            for (int i = 0; i < JUMP_FRAMES_FUEL_RECHARGE; i++)
            {
                if(game_state.fuel >= JUMP_FRAMES_FUEL || (state -> difficulty == DIFF_FUEL_1 && !game_state.fuel))
                    break;
                game_state.fuel++;
            }
        }
        break;
    case JUMPING_FINAL_FRAME:
        stop_jumping(state);
        break;
    default:
        if (game_state.fuel_mode) {
            if (!game_state.fuel)
                game_state.jump_state = JUMPING_FINAL_FRAME;
            else
                game_state.fuel--;
            if (!HAL_GPIO_BTN_ALARM_read() && !HAL_GPIO_BTN_LIGHT_read()) stop_jumping(state);
        }
        else {
            curr_jump_frame = game_state.jump_state - NOT_JUMPING;
            if (curr_jump_frame >= JUMP_FRAMES_EASY || (state -> difficulty >= DIFF_NORM && curr_jump_frame >= JUMP_FRAMES))
                game_state.jump_state = JUMPING_FINAL_FRAME;
            else
                game_state.jump_state++;
        }
        break;
    }
    if (game_state.jump_state == NOT_JUMPING && (game_state.loc_2_on || game_state.loc_3_on)) {
        delay_ms(200);  // To show the player jumping onto the obstacle before displaying the lose screen.
        display_lose_screen(state);
    }
    else if (game_state.fuel_mode)
        display_fuel(subsecond, state -> difficulty);
}

void endless_runner_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(endless_runner_state_t));
        memset(*context_ptr, 0, sizeof(endless_runner_state_t));
        endless_runner_state_t *state = (endless_runner_state_t *)*context_ptr;
        state->difficulty = DIFF_NORM;
        state->tap_control_on = false;
    }
}

void endless_runner_face_activate(void *context) {
    (void) context;
    bool is_custom_lcd = watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM;
    ball_arr_com = is_custom_lcd ? custom_ball_arr_com : classic_ball_arr_com;
    ball_arr_seg = is_custom_lcd ? custom_ball_arr_seg : classic_ball_arr_seg;
    obstacle_arr_com = is_custom_lcd ? custom_obstacle_arr_com : classic_obstacle_arr_com;
    obstacle_arr_seg = is_custom_lcd ? custom_obstacle_arr_seg : classic_obstacle_arr_seg;
    if (watch_sleep_animation_is_running()) {
        watch_stop_blink();
    }
}

bool endless_runner_face_loop(movement_event_t event, void *context) {
    endless_runner_state_t *state = (endless_runner_state_t *)context;
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
            case SCREEN_SCORE:
            case SCREEN_LOSE:
            case SCREEN_TIME:
                break;
            default:
                update_game(state, event.subsecond);
                break;
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
        case EVENT_ALARM_BUTTON_UP:
            switch (game_state.curr_screen) {
                case SCREEN_SCORE:
                    enable_tap_control(state);
                    begin_playing(state);
                    break;
                case SCREEN_TITLE:
                    enable_tap_control(state);
                    // fall through
                case SCREEN_TIME:
                case SCREEN_LOSE:
                    watch_clear_display();
                    display_score_screen(state);
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (game_state.curr_screen == SCREEN_SCORE)
                change_difficulty(state);
            break;
        case EVENT_SINGLE_TAP:
        case EVENT_DOUBLE_TAP:
            if (state->difficulty > DIFF_HARD) break; // Don't do this on fuel modes
            // Allow starting a new game by tapping.
            if (game_state.curr_screen == SCREEN_SCORE) {
                begin_playing(state);
                break;
            }
            else if (game_state.curr_screen == SCREEN_LOSE) {
                display_score_screen(state);
                break;
            }
            //fall through
        case EVENT_LIGHT_BUTTON_DOWN:
        case EVENT_ALARM_BUTTON_DOWN:
            if (game_state.curr_screen == SCREEN_PLAYING && game_state.jump_state == NOT_JUMPING){
                if (game_state.fuel_mode && !game_state.fuel) break;
                game_state.jump_state = JUMPING_START;
                display_ball(game_state.jump_state != NOT_JUMPING);
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (game_state.curr_screen == SCREEN_TITLE || game_state.curr_screen == SCREEN_SCORE)
                toggle_sound(state);
            break;
        case EVENT_TIMEOUT:
            disable_tap_control(state);
            if (game_state.curr_screen != SCREEN_SCORE)
                display_score_screen(state);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            if (game_state.curr_screen != SCREEN_TIME) {
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "RUN  ", "ER  ");
                display_sound_indicator(state -> soundOn);
                display_difficulty(state->difficulty);
            }
            display_time();
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void endless_runner_face_resign(void *context) {
    endless_runner_state_t *state = (endless_runner_state_t *)context;
    disable_tap_control(state);
}
