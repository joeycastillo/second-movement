/*
 * MIT License
 *
 * Copyright (c) 2026 Evelien Dekkers
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

// Emulator only: need time() to seed the random number generator
#if __EMSCRIPTEN__
#include <time.h>
#include <emscripten.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "virtual_pet_face.h"
#include "watch_utility.h"
#include "watch_common_display.h"
#include "watch_rtc.h"

/*
 * Game constants, wanted these in the .h file but the compiler gave me warnings!
 */

static virtual_pet_bottom_text_t virtual_pet_phase_text[] = {
    // Minutes for every subtraction of a stat {hunger, happiness}
    {"Egg   ", "Egg   "}, // Egg
    {"Baby  ", "BABY  "}, // Baby
    {"Child ", "Child "}, // Child
    {"Adult ", " ADult"}, // Adult
    {"Dead  ", "Dead  "}, // Dead
};

static virtual_pet_phase_attribute_t virtual_pet_phase_attributes[] = {
    // Minutes for every subtraction of a stat, phase length {length, hunger, happiness}
    { // Egg
        .phase_length = VIRTUAL_PET_HATCH_TIME,
    },
    { // Baby
        .phase_length = 45,
        .time_hunger = 5, // 30 mins
        .time_happiness = 4, // 24 mins
        .time_needs = 2, // 12 mins
    },
    { // Child
        .phase_length = 12*60, // 12 hrs, 12 hrs of sleep in between
        .time_hunger = 15, // 1,5 hrs
        .time_happiness = 10, // 1 hr
        .time_needs = 8, // 48 mins
    },
    { // Adult (hunger and happiness determined by type, see below.)
    },
    { // Dead
    },
};

/*
 * Sleeping times depending on life phase
 */
static virtual_pet_sleep_time_t virtual_pet_sleep_time[] = {
    { // No sleeping time for an egg, but this is when it won't hatch.
        .hour_start = 18,
        .hour_end = 12,
    },
    { // Baby
        .hour_start = 18,
        .hour_end = 11,
    },
    { // Child
        .hour_start = 20, 
        .hour_end = 8,
    },
    { // Adult
        .hour_start = 21,
        .hour_end = 7,
    },
    { // Dead
    },
};

/*
 * Types of pet and their needs (adult stage only)
 */
static virtual_pet_bottom_text_t virtual_pet_type_text[] = {
    // Minutes for every subtraction of a stat {hunger, happiness}
    {"Needy ", "Needy "}, // Needy
    {"Avg   ", "Averag"}, // Average
    {"Easy  ", "EASY  "}, // Easy
    {"Secret", "Secret"}, // Secret
};

static virtual_pet_type_attribute_t virtual_pet_type_attributes[] = {
    // Minutes for every subtraction of a stat {hunger, happiness}
    { // Needy
        .time_hunger = 40, // 4 hrs
        .time_happiness = 25, // 2.5 hrs
        .time_needs = 20, // 2 hrs
    },
    { // Average
        .time_hunger = 55, // 5.5 hrs
        .time_happiness = 35, // 3.5 hrs
        .time_needs = 25, // 2.5 hrs
    },
    { // Easy
        .time_hunger = 70, // 7 hrs
        .time_happiness = 40, // 4 hrs
        .time_needs = 30, // 3 hrs
    },
    { // Secret is suuuuper chill
        .time_hunger = 100, // 10 hrs
        .time_happiness = 60, // 6 hrs
        .time_needs = 45, // 4.5 hrs
    },
};

/*
 * Status menu and its item names
 */
static virtual_pet_top_left_text_t virtual_pet_status_menu_text[] = {
    // Minutes for every subtraction of a stat {hunger, happiness}
    {"Hgr", "HU"}, // Hunger
    {"Hpy", "HA"}, // Happiness
    {"Age", "AL"}, // Age
    {"Typ", "PH"}, // Type
};

/*
 * Action menu and its item names
 */
static virtual_pet_bottom_text_t virtual_pet_action_menu_text[] = {
    {"Eat   ", "EAT   "}, // Eat
    {"Snack ", "SNACK "}, // Snack
    {"Play  ", "PLAY  "}, // Game
    {"Clean ", "CLEAN "}, // Clean
    {"Heal  ", "HEAL  "}, // Meds
    {"beep  ", "BEEP  "}, // buzzer enable/disable
};

/*
 * Animations/sprites
 * 
 * Every character has its own sprites. In general, there are 3 expressions: Happy, neutral, sad.
 * For every expression, there will be a different sprite. Sprites will be a character map just like
 * the ones found in watch_common_display.h. We will use our own method to display characters. No checks
 * as we already designed around the limitations of the stock LCD.
 * 
 * As far as animations go, each one will have 3 frames. Eating, snacking, healing, cleaning.
 * Idle animations are wanted, but not yet worked out. They may have 4 frames.
 * 
 * Not doing any animations first, just characters and expressions.
 * 
 * For each life phase, the character will look different.
 * 
 */


static uint8_t egg_animation[] = {
    0b01011100, 0, 0,
    0b00111111, 0, 0,
};

static uint8_t baby_animation[] = {
    0b01111011, 0, 0,
    0b01101111, 0, 0,
};

static uint8_t baby_eating_animation[] = {
    0b01101111, 0, 0, // -
    0b01001111, 0, 0, // o
};

static uint8_t child_animation_sad[] = {
    0, 0b00111001, 0,
    0b00001111, 0, 0,
};
static uint8_t child_animation_neutral[] = {
    0, 0b00110000, 0,
    0b00000110, 0, 0,
};
static uint8_t child_animation_happy[] = {
    0, 0b00001111, 0,
    0b00111001, 0, 0,
};
static uint8_t child_eating_animation[] = {
    0, 0b00110000, 0, // -
    0, 0b00111111, 0, // o
};

#define VIRTUAL_PET_ANIMATION_ADULT_EYE_OPEN 0b01100011
#define VIRTUAL_PET_ANIMATION_ADULT_EYE_CLOSED 0b01000000
#define VIRTUAL_PET_ANIMATION_SLEEP_DISPLAY_OFFSET 2

static uint8_t adult_animation_sad[] = {
    VIRTUAL_PET_ANIMATION_ADULT_EYE_OPEN, 0b01010100, VIRTUAL_PET_ANIMATION_ADULT_EYE_OPEN,
    VIRTUAL_PET_ANIMATION_ADULT_EYE_CLOSED, 0b01010100, VIRTUAL_PET_ANIMATION_ADULT_EYE_CLOSED,
};
static uint8_t adult_animation_neutral[] = {
    VIRTUAL_PET_ANIMATION_ADULT_EYE_OPEN, 0b01000000, VIRTUAL_PET_ANIMATION_ADULT_EYE_OPEN,
    VIRTUAL_PET_ANIMATION_ADULT_EYE_CLOSED, 0b01000000, VIRTUAL_PET_ANIMATION_ADULT_EYE_CLOSED,
};
static uint8_t adult_animation_happy[] = {
    0b00100011, 0b01000000, 0b00100011,
    0b01100010, 0b01000000, 0b01100010,
};
static uint8_t adult_eating_animation[] = {
    VIRTUAL_PET_ANIMATION_ADULT_EYE_OPEN, 0b01000000, VIRTUAL_PET_ANIMATION_ADULT_EYE_OPEN, // -
    0b01100010, 0b00111111, 0b01100010, // o
};

static uint8_t death_animation[] = { // Handled by _draw_character()
    0b00110111, 0, 0,
    0b00110111, 0, 0,
};

static uint8_t bed_animation[] = {
    0b01010000, 0b01000110, 0,
    0b01010000, 0b01000110, 0,
};

static uint8_t food_item_animation_frames[] = {
    0, 0, 1, 1, 2, 2, 3, 3,
};
static uint8_t food_character_animation_frames[] = {
    0, 1, 0, 1, 0, 1, 0, 0,
};
static uint8_t food_character_animation_bite_sounds[] = {
    0, 0, 1, 0, 1, 0, 1, 0,
};

static uint8_t food_item_animation[] = {
    0b01111111,
    0b01111100,
    0b00001100,
    0b00000000,
};

static uint8_t poop_animation[] = {
    0b01011110,
    0b01011100,
    0b01111100,
    0b01011100,
};
static uint8_t sickness_animation[] = {
    0b11100011,
};

static uint8_t little_ghost_animation[] = {// Handled by _draw_little_ghost()
    0b01100111,
    0b01110011,
};

static virtual_pet_phase_animation_t virtual_pet_phase_animations[] = {
    // Minutes for every subtraction of a stat, phase length {length, hunger, happiness}
    { // Egg
        .display_offset = 3,
        .idle_animation_chance = 4,
        .sad = egg_animation,
        .neutral = egg_animation,
        .happy = egg_animation,
        .can_sleep = false,
    },
    { // Baby
        .display_offset = 3,
        .idle_animation_chance = 3,

        .sad = baby_animation,
        .neutral = baby_animation,
        .happy = baby_animation,

        .sleeping = bed_animation,
        .can_sleep = true,
        .eating = baby_eating_animation,
        .food_position = 0,
    },
    { // Child
        .display_offset = 1,
        .idle_animation_chance = 1,

        .sad = child_animation_sad,
        .neutral = child_animation_neutral,
        .happy = child_animation_happy,

        .sleeping = bed_animation,
        .can_sleep = true,
        .eating = child_eating_animation,
        .food_position = 0,
    },
    { // Adult (hunger and happiness determined by type, see below.)
        .display_offset = 1,
        .idle_animation_chance = 2,

        .sad = adult_animation_sad,
        .neutral = adult_animation_neutral,
        .happy = adult_animation_happy,

        .sleeping = bed_animation,
        .can_sleep = true,
        .eating = adult_eating_animation,
        .food_position = 0,
    },
    { // Dead
        .display_offset = 1,
        .idle_animation_chance = 10,

        .sad = death_animation,
        .neutral = death_animation,
        .happy = death_animation,

        .can_sleep = false,
    },
};

/*
 * Note sequences.
 * - Boot (first time it comes on screen)
 * - Attention
 * - About to hatch
 * - Hatch
 * - Transformation/Evolution
 * - Eating 'bite' sound
 * - Happy (after winning game or getting food)
 * - Sad (after wrong action, overfeeding or losing game)
 * - Death
 * - Game start
 * - Game win
 * - Game lose
 * - For debugging: background task tune
 */

static int8_t virtual_pet_start_tune[] = {
    BUZZER_NOTE_C7, 8,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_E7, 8,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_F7, 8,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G7, 8,
    BUZZER_NOTE_REST, 26,
    BUZZER_NOTE_E7, 8,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_F7, 8,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_G7, 8,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_A7, 8,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_C8, 8,
    0
};

static int8_t virtual_pet_attention_tune[] = {
    BUZZER_NOTE_C8, 9,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_A7, 9,
    BUZZER_NOTE_REST, 14,
    BUZZER_NOTE_C8, 7,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_A7, 7,
    BUZZER_NOTE_REST, 29,
    BUZZER_NOTE_C8, 9,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_A7, 9,
    BUZZER_NOTE_REST, 14,
    BUZZER_NOTE_C8, 7,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_A7, 7,
    0
};

static int8_t virtual_pet_pre_hatch_tune[] = {
    BUZZER_NOTE_B6, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_E7, 5,
    0
};

static int8_t virtual_pet_hatch_tune[] = {
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_E7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_E7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_E7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_E7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_E7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_C8, 18,
    0
};

static int8_t virtual_pet_transformation_tune[] = {
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_E7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_F7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_G7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_E7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_F7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_G7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_E7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_F7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_G7, 5,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_C8, 16,
    0
};

static int8_t virtual_pet_eating_tune[] = {
    BUZZER_NOTE_F7, 5,
    BUZZER_NOTE_REST, 3,
    BUZZER_NOTE_G7, 5,
    0
};

static int8_t virtual_pet_happy_tune[] = {
    BUZZER_NOTE_A7, 6,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_B7, 6,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_C8, 6,
    0
};

static int8_t virtual_pet_sad_tune[] = {
    BUZZER_NOTE_C7, 5,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_C7, 12,
    0
};

static int8_t virtual_pet_death_tune[] = {
    BUZZER_NOTE_C7, 16,
    BUZZER_NOTE_REST, 32,
    BUZZER_NOTE_C7, 32,
    BUZZER_NOTE_REST, 32,
    BUZZER_NOTE_C7, 64,
    BUZZER_NOTE_REST, 32,
    BUZZER_NOTE_C7, 128,
    0
};

static int8_t virtual_pet_game_start_tune[] = {
    BUZZER_NOTE_G7, 12,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_G7, 12,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_C8, 12,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_E7, 12,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_G7, 12,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_C8, 12,
    0
};

static int8_t virtual_pet_game_win_tune[] = {
    BUZZER_NOTE_F7, 12,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_G7, 12,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_F7, 12,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_G7, 12,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_C8, 20,
    0
};

static int8_t virtual_pet_game_lose_tune[] = {
    BUZZER_NOTE_A7, 12,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_G7, 12,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_F7, 12,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_D7, 12,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_C7, 20,
    0
};

#if VIRTUAL_PET_BACKGROUND_TASK_TUNE
static int8_t virtual_pet_background_task_tune[] = {
    BUZZER_NOTE_F7, 8,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_G7, 8,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_F7, 8,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_G7, 8,
    0
};
#endif

static inline void button_beep() {
    // play a beep as confirmation for a button press (if applicable)
    if (movement_button_should_sound()) watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
}
static inline void button_beep_return() {
    // play a beep as confirmation for a button press (if applicable)
    if (movement_button_should_sound()) watch_buzzer_play_note_with_volume(BUZZER_NOTE_C8, 50, movement_button_volume());
}
static inline void clear_bottom() {
    watch_display_text(WATCH_POSITION_BOTTOM, "      ");
}
static inline void clear_top() {
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "   ", "  ");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
}
static uint32_t rtc_counter_to_seconds(rtc_counter_t counter) {
    uint32_t freq = watch_rtc_get_frequency();
    return counter / freq;
}
static uint32_t get_time_alive(void *context, rtc_counter_t current_counter) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    if (state->birth_counter == 0) return 0;
    return (rtc_counter_to_seconds(current_counter - state->birth_counter)) / 60;
}
static int random(int min, int max){
   return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

static void _setup_pet(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    state->hunger = VIRTUAL_PET_STARTING_HUNGER;
    state->happiness = VIRTUAL_PET_STARTING_HAPPINESS;
    state->poops = VIRTUAL_PET_STARTING_POOPS;
    state->sickness = VIRTUAL_PET_STARTING_SICKNESS;
    state->needs = VIRTUAL_PET_STARTING_NEEDS;
    state->needs_attention = VIRTUAL_PET_STARTING_ATTENTION;
    state->care_mistakes = VIRTUAL_PET_STARTING_CARE_MISTAKES;

    state->passing_counter = 0;
    state->start_counter = 0;
    state->birth_counter = 0;
    state->phase = VIRTUAL_PET_STARTING_PHASE;
    state->minutes_to_hatch = VIRTUAL_PET_RANDOM_HATCH_TIME ? random(5, 120) : virtual_pet_phase_attributes[VIRTUAL_PET_PHASE_EGG].phase_length;
    state->awake_in_phase_counter = 0;
    state->last_game_logic_loop_counter = 0;
    state->games_played = VIRTUAL_PET_STARTING_GAMES_PLAYED;
    state->egg_idle_animation_chance = virtual_pet_phase_animations[VIRTUAL_PET_PHASE_EGG].idle_animation_chance;
    state->snacks_today = 0;

    state->current_character_animation_frame = 0;
    state->current_poop_animation_frame = 0;
    state->current_sickness_animation_frame = 0;
    state->current_food_animation_frame = 0;
    state->current_little_ghost_animation_frame = 0;
    state->current_sleeping_animation_frame = 0;
    state->current_eating_animation_frame = 0;

    if (state->phase == VIRTUAL_PET_PHASE_ADULT) state->type = VIRTUAL_PET_STARTING_TYPE;
}

static bool _grieving_time_over(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    rtc_counter_t counter = watch_rtc_get_counter();
    return counter >= state->passing_counter + (watch_rtc_get_frequency() * VIRTUAL_PET_GRIEVING_TIME * 60);
}

static void _exit_menu(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    state->display_mode = VIRTUAL_PET_DISPLAY_MODE_PET;
    state->current_menu_index = 0;
    clear_bottom();
    clear_top();
}

static void wake_up_from_sleep(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    watch_stop_sleep_animation();
    movement_request_wake();
    // after this, it still doesn't fully wake up - the screen does not update as usual. need some help here.
    state->came_out_of_sleep = true;
#if __EMSCRIPTEN__
    EM_ASM({
    console.log('Pet face: waking up from sleep');
    });
#endif
}
static void go_back_to_sleep(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    watch_start_sleep_animation(500);
    movement_request_sleep();
    state->came_out_of_sleep = false;
#if __EMSCRIPTEN__
    EM_ASM({
    console.log('Pet face: going back to sleep');
    });
#endif
}

static void bring_to_foreground(void *context) {
    // always call this before playing notes!
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    movement_move_to_face(state->face_idx);
    watch_buzzer_abort_sequence(); // no "tick" when changing face
    _exit_menu(state);
    if (watch_sleep_animation_is_running()) wake_up_from_sleep(state);
}

static void _set_pet_type(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    if (state->games_played >= VIRTUAL_PET_GAMES_PLAYED_FOR_SECRET_TYPE) {
        state->type = VIRTUAL_PET_TYPE_SECRET;
        return;
    }
    switch(state->care_mistakes) {
        case 0:
        case 1:
            state->type = VIRTUAL_PET_TYPE_EASY;
            break;
        case 2:
        case 3:
        case 4:
            state->type = VIRTUAL_PET_TYPE_AVERAGE;
            break;
        default:
            state->type = VIRTUAL_PET_TYPE_NEEDY;
            break;
    }
    return;
}

static void _ask_for_attention(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    bring_to_foreground(state);
    if (state->buzzer_enabled) movement_play_sequence(virtual_pet_attention_tune, BUZZER_PRIORITY_SIGNAL);
}

static void _make_pet_dead(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    state->phase = VIRTUAL_PET_PHASE_DEAD;
    state->passing_counter = watch_rtc_get_counter();
    bring_to_foreground(state);
    if (state->buzzer_enabled) movement_play_sequence(virtual_pet_death_tune, BUZZER_PRIORITY_SIGNAL);
}

static void _make_pet_sick(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    if (state->phase == VIRTUAL_PET_PHASE_CHILD) state->care_mistakes++;
    if (state->sickness == VIRTUAL_PET_MAX_SICKNESS) {
        _make_pet_dead(state);
    } else {
        state->sickness++;
        _ask_for_attention(state);
    }
}

static void _make_pet_sick_with_chance(void *context, uint8_t chance) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    int x = random(0, 9);
    if (x <= chance) {
        _make_pet_sick(state);
    }
}

static void _set_needs_attention(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    if (state->hunger > 0 && state->happiness > 0 && state->sickness == 0)
        state->needs_attention = false;
    else
        state->needs_attention = true;
}

static void _start_eating_animation(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    state->is_eating = true;
    state->current_character_animation_frame = 0;
    state->current_eating_animation_frame = 0;
}

static bool _is_sleeping_time(void *context, watch_date_time_t current_time) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    virtual_pet_sleep_time_t sleep_time = virtual_pet_sleep_time[state->phase];
    if (state->phase == VIRTUAL_PET_PHASE_DEAD) return false;
    if (sleep_time.hour_end < sleep_time.hour_start) {
        return (current_time.unit.hour >= sleep_time.hour_start || current_time.unit.hour < sleep_time.hour_end);
    } else {
        return (current_time.unit.hour >= sleep_time.hour_start && current_time.unit.hour < sleep_time.hour_end);
    }
}

static void _set_is_sleeping(void *context, watch_date_time_t current_time) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    bool is_sleep_time = _is_sleeping_time(state, current_time);
    if (is_sleep_time) {
        if (!state->is_sleeping && virtual_pet_phase_animations[state->phase].can_sleep) {
            state->is_sleeping = true;
        }
    } else {
        if (state->is_sleeping) {
            state->is_sleeping = false;
            // reset snacks_today
            state->snacks_today = 0;
            //update stats after waking up
            if (state->phase > VIRTUAL_PET_PHASE_EGG) {
                for (int i = 0; i < 2; i++) {
                    if(state->hunger > 1) state->hunger--;
                    if(state->happiness > 1) state->happiness--;
                    if(state->needs < VIRTUAL_PET_MAX_NEEDS) state->needs++;
                }
            }
            //ask for attention once the pet wakes up and it already needs attention
            if (state->needs_attention) {
                _ask_for_attention(state);
            }
        }
    }
}

static virtual_pet_emotion _get_emotion(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;

    if (state->display_mode == VIRTUAL_PET_DISPLAY_MODE_GAME) {
        return (state->game_round_won) ? VIRTUAL_PET_EMOTION_HAPPY : VIRTUAL_PET_EMOTION_SAD;
    }

    if (state->is_sleeping) return VIRTUAL_PET_EMOTION_SLEEPING;
    if (state->is_eating) return VIRTUAL_PET_EMOTION_EATING;

    if (state->happiness > VIRTUAL_PET_MAX_HAPPINESS*0.5 && state->sickness == 0 && state->poops == 0)
        return VIRTUAL_PET_EMOTION_HAPPY;
    else if (state->happiness > VIRTUAL_PET_MAX_HAPPINESS*0.25 && state->sickness == 0)
        return VIRTUAL_PET_EMOTION_NEUTRAL;
    return VIRTUAL_PET_EMOTION_SAD;
}

static uint8_t *_get_current_character_animation(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    switch(_get_emotion(state)) {
        case VIRTUAL_PET_EMOTION_HAPPY:
            return virtual_pet_phase_animations[state->phase].happy;
        case VIRTUAL_PET_EMOTION_SAD:
            return virtual_pet_phase_animations[state->phase].sad;
        case VIRTUAL_PET_EMOTION_SLEEPING:
            return virtual_pet_phase_animations[state->phase].sleeping;
        case VIRTUAL_PET_EMOTION_EATING:
            return virtual_pet_phase_animations[state->phase].eating;
        default:
            return virtual_pet_phase_animations[state->phase].neutral;
    }
}

static uint8_t _get_next_character_frame_index(void *context) {
    // Only works for two frames right now.
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    uint8_t current_frame = state->current_character_animation_frame;

    if (state->is_eating && state->current_eating_animation_frame == 0) return 0;

    if (current_frame == 0) {
        if (state->is_eating) return 1;
        int x = random(0, 9);
        if (state->phase == VIRTUAL_PET_PHASE_EGG && x <= state->egg_idle_animation_chance) return 1;
        if (!state->is_sleeping && x <= virtual_pet_phase_animations[state->phase].idle_animation_chance) return 1;
        if (state->is_sleeping && x <= VIRTUAL_PET_SLEEPING_ANIMATION_CHANCE) return 1;
    }
    return 0;
}

static uint8_t _get_next_eating_character_frame_index(void *context) {
    // Only works for two frames right now.
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    return food_character_animation_frames[state->current_eating_animation_frame];
}

static void watch_display_byte(uint8_t segdata, uint8_t position) {
    digit_mapping_t segmap;

    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        segmap = Custom_LCD_Display_Mapping[position];
    } else {
        segmap = Classic_LCD_Display_Mapping[position];
    }
    
    for (int i = 0; i < 8; i++) {
        if (segmap.segment[i].value == segment_does_not_exist) {
            // Segment does not exist; skip it.
            segdata = segdata >> 1;
            continue;
        }
        uint8_t com = segmap.segment[i].address.com;
        uint8_t seg = segmap.segment[i].address.seg;

        if (segdata & 1) {
            watch_set_pixel(com, seg);
        }
        else {
            watch_clear_pixel(com, seg);
        }

        segdata = segdata >> 1;
    }
}

static void _draw_character(void *context) {
    // Only works for two frames right now.
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;

    state->current_character_animation_frame = (state->is_eating) ? _get_next_eating_character_frame_index(state) : _get_next_character_frame_index(state);

    for (uint8_t i = 0; i < VIRTUAL_PET_CHARACTER_STRING_LENGTH; i++) {
        watch_display_byte(
            _get_current_character_animation(state)[state->current_character_animation_frame*3+i],
            ((state->is_sleeping) ? VIRTUAL_PET_ANIMATION_SLEEP_DISPLAY_OFFSET : virtual_pet_phase_animations[state->phase].display_offset) + ((watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) ? 4 : 4) + i
        );
    }
    if (state->phase == VIRTUAL_PET_PHASE_CHILD && !state->is_sleeping) watch_set_colon();
    else watch_clear_colon();
}

static void _draw_poop(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    state->current_poop_animation_frame = (state->current_poop_animation_frame == sizeof(poop_animation)/sizeof(poop_animation[0])-1) ? 0 : state->current_poop_animation_frame + 1;
    for (uint8_t i = 0; i < state->poops; i++) {
        watch_display_byte(
            (state->is_sleeping) ? 0 :
            poop_animation[
                state->current_poop_animation_frame
            ],
            (i == 1) ? 9 : 3
        );
    }
}

static void _draw_sickness(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    if (state->sickness == 0) return;
    state->current_sickness_animation_frame = (state->current_sickness_animation_frame == sizeof(sickness_animation)/sizeof(sickness_animation[0])-1) ? 0 : state->current_sickness_animation_frame + 1;
    watch_display_byte(
        (state->is_sleeping) ? 0 :
        sickness_animation[state->current_sickness_animation_frame],
        0
    );
}

static void _draw_attention(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    // don't draw while dead or while sleeping.
    if (state->needs_attention && state->passing_counter == 0 && !state->is_sleeping) watch_set_indicator(WATCH_INDICATOR_BELL);
    else watch_clear_indicator(WATCH_INDICATOR_BELL);
}

static void _draw_little_ghost(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    if (state->phase != VIRTUAL_PET_PHASE_DEAD) return;
    state->current_little_ghost_animation_frame = (state->current_little_ghost_animation_frame == sizeof(little_ghost_animation)/sizeof(little_ghost_animation[0])-1) ? 0 : state->current_little_ghost_animation_frame + 1;
    watch_display_byte(
        little_ghost_animation[state->current_little_ghost_animation_frame],
        (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) ? 1 : 0
    );
}

static void _draw_food(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    bool last_frame = (state->current_eating_animation_frame == sizeof(food_item_animation_frames)/sizeof(food_item_animation_frames[0])-1);
    if (last_frame) {
        state->is_eating = false;
        if (state->buzzer_enabled) movement_play_sequence(virtual_pet_happy_tune, BUZZER_PRIORITY_BUTTON);
    } 

    // play "bite" sound
    if (food_character_animation_bite_sounds[state->current_eating_animation_frame] == 1)
        if (state->buzzer_enabled) movement_play_sequence(virtual_pet_eating_tune, BUZZER_PRIORITY_BUTTON);

    watch_display_byte(
        last_frame ? 0 : food_item_animation[food_item_animation_frames[state->current_eating_animation_frame]],
        virtual_pet_phase_animations[state->phase].food_position
    );

    if (!last_frame) state->current_eating_animation_frame++;
}

static void _draw_pet(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;

    _draw_character(state);
    // only update when not eating for added gameplay
    if (state->phase > VIRTUAL_PET_PHASE_EGG && !state->is_eating) _draw_attention(state);
    if (state->phase > VIRTUAL_PET_PHASE_EGG && state->phase != VIRTUAL_PET_PHASE_DEAD) {
        _draw_poop(state);
        _draw_sickness(state);
        if (state->is_eating) _draw_food(state);
    } else if (state->phase == VIRTUAL_PET_PHASE_DEAD) {
        _draw_little_ghost(state);
    }
    if (state->is_sleeping) {
        switch (state->current_sleeping_animation_frame) {
            case 0:
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "2 ");
                state->current_sleeping_animation_frame++;
                break;
            case 1:
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "22");
                state->current_sleeping_animation_frame++;
                break;
            default:
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
                state->current_sleeping_animation_frame = 0;
                break;
        }
    }
    // make "tick" noise when hatching egg
    // getting ui and background task to sync up is annoying!
    if (state->phase == VIRTUAL_PET_PHASE_EGG) {
        rtc_counter_t current_counter = watch_rtc_get_counter();
        if (state->last_game_logic_loop_counter != 0) {
            state->awake_in_phase_counter += current_counter - state->last_game_logic_loop_counter;
        }
        state->last_game_logic_loop_counter = current_counter;
        uint32_t phase_length_s = (60*state->minutes_to_hatch);
        uint16_t seconds_until_hatch = phase_length_s - rtc_counter_to_seconds(state->awake_in_phase_counter);

        if (seconds_until_hatch <= VIRTUAL_PET_PRE_HATCH_TIME && seconds_until_hatch > 3 && state->current_character_animation_frame == 1) {
            if (state->buzzer_enabled) movement_play_sequence(virtual_pet_pre_hatch_tune, BUZZER_PRIORITY_BUTTON);
        }
    }
}

static void _draw_pet_low_energy(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;

    if (state->phase == VIRTUAL_PET_PHASE_ADULT) state->current_character_animation_frame = 1;
    state->current_poop_animation_frame = (state->current_poop_animation_frame == 0) ? 1 : 3;

    _draw_character(state);
    if (state->phase > VIRTUAL_PET_PHASE_EGG) _draw_attention(state);
    if (state->phase > VIRTUAL_PET_PHASE_EGG && state->phase != VIRTUAL_PET_PHASE_DEAD) {
        _draw_poop(state);
        _draw_sickness(state);
    } else if (state->phase == VIRTUAL_PET_PHASE_DEAD) {
        _draw_little_ghost(state);
    }
    if (state->is_sleeping) {
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "22");
    }
}

static void _draw_progress_bar(uint8_t value, uint8_t max) {
    char buf[7] = { 0 };
    for (int i = 0; i < value; i++) {
        buf[i] = '+';
    }
    for (int i = value; i < max; i++) {
        buf[i] = '-';
    }
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static void _draw_status_menu(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    char buf[7] = { 0 };
    watch_clear_colon();
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, virtual_pet_status_menu_text[state->current_menu_index].menu_text, virtual_pet_status_menu_text[state->current_menu_index].menu_text_fallback);

    switch(state->current_menu_index) {
        case VIRTUAL_PET_STATUS_MENU_HUNGER:
            _draw_progress_bar(state->hunger, VIRTUAL_PET_MAX_HUNGER);
            break;
        case VIRTUAL_PET_STATUS_MENU_HAPPINESS:
            _draw_progress_bar(state->happiness, VIRTUAL_PET_MAX_HAPPINESS);
            break;
        case VIRTUAL_PET_STATUS_MENU_AGE:
            sprintf(buf, "%6d", (uint16_t) ((VIRTUAL_PET_SHOW_MINUTES_ALIVE) ? state->time_alive : state->time_alive / 1440));
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
            break;
        case VIRTUAL_PET_STATUS_MENU_TYPE:
            switch(state->phase) {
                case VIRTUAL_PET_PHASE_ADULT:
                    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, virtual_pet_type_text[state->type].menu_text, virtual_pet_type_text[state->type].menu_text_fallback);
                    break;
                default:
                    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, virtual_pet_phase_text[state->phase].menu_text, virtual_pet_phase_text[state->phase].menu_text_fallback);
                    break;
            }
            break;
    }
}

static void _draw_action_menu(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    watch_clear_colon();
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, virtual_pet_action_menu_text[state->current_menu_index].menu_text, virtual_pet_action_menu_text[state->current_menu_index].menu_text_fallback);
    switch(state->current_menu_index) {
        case VIRTUAL_PET_ACTION_MENU_ITEM_BUZZER:
            watch_display_character((state->buzzer_enabled) ? 'y' : 'N', 9);
            break;
    }
}

static void _draw_game(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    char buf[7] = {0};
    watch_clear_colon();

    switch(state->game_display_mode) {
        case VIRTUAL_PET_GAME_MODE_ROUND_SHOW_GUESS:
            watch_display_character(state->game_hidden_number + '0', 7);
            // fall through
        case VIRTUAL_PET_GAME_MODE_ROUND_ACTIVE:
            watch_display_character(state->game_shown_number + '0', 5);
            break;
        case VIRTUAL_PET_GAME_MODE_SCORE:
            sprintf(buf, " %d-%d", state->game_score, state->game_round - state->game_score);
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
            break;
        default:
            break;

    }
}

static bool _pet_loop(movement_event_t event, void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;

    switch (event.event_type) {
        case EVENT_LIGHT_BUTTON_DOWN:
            break; // no light
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            _draw_pet(state);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if (state->is_eating) break;
            if (state->is_sleeping) break;
            if (state->is_eating) watch_display_character(' ', virtual_pet_phase_animations[state->phase].food_position);
            switch(state->phase) {
                case VIRTUAL_PET_PHASE_EGG:
                case VIRTUAL_PET_PHASE_DEAD:
                    if (state->passing_counter > 0 && _grieving_time_over(state)) {
                        _setup_pet(state);
                    }
                    break;
                default:
                    state->display_mode = VIRTUAL_PET_DISPLAY_MODE_STATUS_MENU;
                    clear_bottom();
                    clear_top();
                    _draw_status_menu(state);
                    if (state->buzzer_enabled) button_beep();
                    break;
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->is_sleeping) break;
            if (state->is_eating) {
                state->is_eating = false;
                watch_display_character(' ', virtual_pet_phase_animations[state->phase].food_position);
            }
            switch(state->phase) {
                case VIRTUAL_PET_PHASE_EGG:
                case VIRTUAL_PET_PHASE_DEAD:
                    if (state->passing_counter > 0 && _grieving_time_over(state)) {
                        _setup_pet(state);
                    }
                    return true;
                default:
                    state->display_mode = VIRTUAL_PET_DISPLAY_MODE_ACTION_MENU;
                    clear_bottom();
                    clear_top();
                    _draw_action_menu(state);
                    if (state->buzzer_enabled) button_beep();
                    break;
            }
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            if (state->came_out_of_sleep == false && !watch_sleep_animation_is_running()) watch_start_sleep_animation(500);
            _draw_pet_low_energy(state);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    // return true if the watch can enter standby mode. Generally speaking, you should always return true.
    return true;
}

static bool _status_menu_loop(movement_event_t event, void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;

    switch (event.event_type) {
        case EVENT_LIGHT_BUTTON_DOWN:
            break; // no light
        case EVENT_LIGHT_BUTTON_UP:
            if (state->current_menu_index == VIRTUAL_PET_STATUS_MENU_LENGTH - 1) {
                _exit_menu(state);
                event.event_type = EVENT_TICK;
                if (state->buzzer_enabled) button_beep_return();
                return _pet_loop(event, state);
            } else {
                state->current_menu_index++;
            }
            if (state->buzzer_enabled) button_beep();
            // fall through
        case EVENT_TICK:
            _draw_status_menu(state);
            // fall through
        default:
            return movement_default_loop_handler(event);
    }

    // return true if the watch can enter standby mode. Generally speaking, you should always return true.
    return true;
}

static bool _handle_action_menu_choice(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    // return true if happy sound, else sad sound
    if (state->is_sleeping) return false;
    switch(state->current_menu_index) {
        case VIRTUAL_PET_ACTION_MENU_ITEM_EAT:
            if (state->hunger >= VIRTUAL_PET_MAX_HUNGER || state->sickness > 0) {
                return false;
            } else {
                if (state->buzzer_enabled) button_beep();
                state->hunger++;
                _start_eating_animation(state);
                return true;
            }
            break;
        case VIRTUAL_PET_ACTION_MENU_ITEM_SNACK:
            if (state->happiness >= VIRTUAL_PET_MAX_HAPPINESS || state->sickness > 0) {
                int x = random(0, 9);
                if (x <= VIRTUAL_PET_SNACK_SICKNESS_CHANCE) {
                    _make_pet_sick(state);
                    return false;
                }
                if (state->buzzer_enabled) button_beep();
                _start_eating_animation(state);
                if (state->snacks_today < VIRTUAL_PET_MAX_SNACKS_DAILY) state->snacks_today++;
                return true;
            } else {
                state->happiness++;
                if (state->snacks_today < VIRTUAL_PET_MAX_SNACKS_DAILY) state->snacks_today++;
                if (state->buzzer_enabled) button_beep();
                _start_eating_animation(state);
                return true;
            }
            break;
        case VIRTUAL_PET_ACTION_MENU_ITEM_CLEAN:
            if (state->poops > 0) {
                state->poops = 0;
                return true;
            } else {
                return false;
            }
            break;
        case VIRTUAL_PET_ACTION_MENU_ITEM_MEDICATION:
            if (state->sickness > 0) {
                int x = random(0, 9);
                if (x <= VIRTUAL_PET_MEDS_NOT_WORKING_CHANCE) {
                    // return true because false confuses the caretaker about the action of heal
                    return true;
                }
                state->sickness = 0;
                return true;
            } else {
                return false;
            }
            break;
        case VIRTUAL_PET_ACTION_MENU_ITEM_GAME:
            if (state->sickness > 0) return false;
            if (state->phase < VIRTUAL_PET_PHASE_CHILD) return false;
            return true;
        default:
            break;
    }
    return true;
}

static void _setup_game_shown_numbers(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    int previous_number = state->game_shown_number;
    do {
        state->game_shown_number = random(2,8);
    } while (state->game_shown_number == previous_number);

    previous_number = state->game_hidden_number;
    do {
        state->game_hidden_number = random(1,9);
    } while (state->game_hidden_number == state->game_shown_number || state->game_hidden_number == previous_number);
    // makes sure the numbers can never be the same. I want higher or lower, not equal!!
    
#if __EMSCRIPTEN__
    EM_ASM({
    console.log('Pet face: game number is ' + $0);
    }, state->game_hidden_number);
#endif
}

static void _setup_game(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    state->game_shown_number = 0;
    state->game_hidden_number = 0;
    _setup_game_shown_numbers(state);
    state->game_display_mode = VIRTUAL_PET_GAME_MODE_ROUND_ACTIVE;
    state->game_round = 0;
    state->game_score = 0;
    state->game_mode_ticks = 0;
    state->game_round_won = false;
}

static bool _exit_game(movement_event_t event, void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    _setup_game(state);
    _exit_menu(state);
    event.event_type = EVENT_TICK;
    return _pet_loop(event, state);
}

static bool _game_mode_loop(movement_event_t event, void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    state->current_character_animation_frame = 1; // don't animate the character

    switch (event.event_type) {
        case EVENT_LIGHT_BUTTON_DOWN:
            break; // no light
        case EVENT_ALARM_BUTTON_UP:
            // Go back
            if (state->buzzer_enabled) button_beep_return();
            return _exit_game(event, state);
            break;
        case EVENT_TICK:
            switch (state->game_display_mode) {
                case VIRTUAL_PET_GAME_MODE_REACTION:
                    _draw_character(state);
                    state->game_mode_ticks++;
                    if (state->game_mode_ticks >= VIRTUAL_PET_GAME_REACTION_TICKS + 1) {
                        state->game_mode_ticks = 0;
                        state->game_round = (state->game_round == VIRTUAL_PET_MAX_GAME_ROUNDS) ? state->game_round : state->game_round + 1;
                        if (state->game_round == VIRTUAL_PET_MAX_GAME_ROUNDS) {
                            state->game_display_mode = VIRTUAL_PET_GAME_MODE_SCORE;
                            clear_bottom();
                            
                            // play won/lost tune
                            int8_t *note_sequence = virtual_pet_game_win_tune;
                            if (state->game_score < VIRTUAL_PET_GAME_WIN_SCORE) note_sequence = virtual_pet_game_lose_tune;
                            if (state->buzzer_enabled) movement_play_sequence(note_sequence, BUZZER_PRIORITY_BUTTON);

                            // update secret stat
                            if (state->games_played < VIRTUAL_PET_GAMES_PLAYED_FOR_SECRET_TYPE) state->games_played++;

                            // update pet stats
                            // give two happiness points if won
                            for (int i = 0; i < 2; i++) {
                                if (state->happiness < VIRTUAL_PET_MAX_HAPPINESS && state->game_score >= VIRTUAL_PET_GAME_WIN_SCORE) state->happiness++;
                            }
                            if (state->needs < VIRTUAL_PET_MAX_NEEDS) state->needs++;
                        } else {
                            state->game_display_mode = VIRTUAL_PET_GAME_MODE_ROUND_ACTIVE;
                            clear_bottom();
                        }
                        _draw_game(state);
                    }
                    break;
                case VIRTUAL_PET_GAME_MODE_SCORE:
                    state->game_mode_ticks++;
                    if (state->game_mode_ticks >= VIRTUAL_PET_GAME_REACTION_TICKS + 2) {
                        return _exit_game(event, state);
                    }
                    _draw_game(state);
                    break;
                case VIRTUAL_PET_GAME_MODE_ROUND_ACTIVE:
                    // if (state->game_mode_ticks >= 3) {
                    //     if (state->buzzer_enabled) movement_play_note(BUZZER_NOTE_C7, 50);
                    // }
                    // else state->game_mode_ticks++;
                    _draw_game(state);
                    break;
                case VIRTUAL_PET_GAME_MODE_ROUND_SHOW_GUESS:
                    if (state->game_mode_ticks >= VIRTUAL_PET_GAME_REACTION_TICKS) {
                        state->game_display_mode = VIRTUAL_PET_GAME_MODE_REACTION;
                        clear_bottom();
                        state->game_mode_ticks = 0;
                        int8_t *note_sequence = virtual_pet_happy_tune;
                        if (_get_emotion(state) == VIRTUAL_PET_EMOTION_SAD) note_sequence = virtual_pet_sad_tune;
                        if (state->buzzer_enabled) movement_play_sequence(note_sequence, BUZZER_PRIORITY_BUTTON);
                        _setup_game_shown_numbers(state);
                        _draw_character(state);
                    } else {
                        state->game_mode_ticks++;
                        _draw_game(state);
                    }
                    break;
                default:
                    _draw_game(state);
                    break;
            }
            break;
        case EVENT_MODE_BUTTON_UP:
            // Choice: Lower
            if (state->game_display_mode != VIRTUAL_PET_GAME_MODE_ROUND_ACTIVE) break;
            if (state->buzzer_enabled) button_beep();
            if (state->game_hidden_number < state->game_shown_number) {
                state->game_score++;
                state->game_round_won = true;
            } else {
                state->game_round_won = false;
            }
            state->game_mode_ticks = 0;
            state->game_display_mode = VIRTUAL_PET_GAME_MODE_ROUND_SHOW_GUESS;
            clear_bottom();
            _draw_game(state);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if (state->game_display_mode != VIRTUAL_PET_GAME_MODE_ROUND_ACTIVE) break;
            // Choice: Higher
            if (state->buzzer_enabled) button_beep();
            if (state->game_hidden_number > state->game_shown_number) {
                state->game_score++;
                state->game_round_won = true;
            } else {
                state->game_round_won = false;
            }
            state->game_mode_ticks = 0;
            state->game_display_mode = VIRTUAL_PET_GAME_MODE_ROUND_SHOW_GUESS;
            clear_bottom();
            _draw_game(state);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    // return true if the watch can enter standby mode. Generally speaking, you should always return true.
    return true;
}

static bool _action_menu_loop(movement_event_t event, void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;

    switch (event.event_type) {
        case EVENT_LIGHT_BUTTON_DOWN:
            break; // no light
        case EVENT_ALARM_BUTTON_UP:
            if (state->current_menu_index == VIRTUAL_PET_ACTION_MENU_LENGTH - 1) {
                _exit_menu(state);
                event.event_type = EVENT_TICK;
                if (state->buzzer_enabled) button_beep_return();
                return _pet_loop(event, state);
            } else {
                state->current_menu_index++;
            }
            if (state->buzzer_enabled) button_beep();
            // fall through
        case EVENT_TICK:
            _draw_action_menu(state);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // do, action!
            switch(state->current_menu_index) {
                case VIRTUAL_PET_ACTION_MENU_ITEM_BUZZER:
                    state->buzzer_enabled ^= 1;
                    watch_display_character((state->buzzer_enabled) ? 'y' : 'N', 9);
                    if (state->buzzer_enabled) button_beep();
                    break;
                default:
                    if (_handle_action_menu_choice(state)) {
                        switch(state->current_menu_index) {
                        // no happy sound when making the choice, for eating this comes after done eating.
                        // also no sound for game.
                            case VIRTUAL_PET_ACTION_MENU_ITEM_GAME:
                                _exit_menu(state);
                                _setup_game(state);
                                movement_play_sequence(virtual_pet_game_start_tune, BUZZER_PRIORITY_BUTTON);
                                state->display_mode = VIRTUAL_PET_DISPLAY_MODE_GAME;
                                event.event_type = EVENT_TICK;
                                return _game_mode_loop(event, state);
                                // fall through
                            case VIRTUAL_PET_ACTION_MENU_ITEM_EAT:
                            case VIRTUAL_PET_ACTION_MENU_ITEM_SNACK:
                                break;
                            default:
                                if (state->buzzer_enabled) movement_play_sequence(virtual_pet_happy_tune, BUZZER_PRIORITY_BUTTON);
                        }
                    } else {
                        if (state->buzzer_enabled) movement_play_sequence(virtual_pet_sad_tune, BUZZER_PRIORITY_BUTTON);
                    }
                    _exit_menu(state);
                    event.event_type = EVENT_TICK;
                    return _pet_loop(event, state);
            }
            // fall through
        default:
            return movement_default_loop_handler(event);
    }

    // return true if the watch can enter standby mode. Generally speaking, you should always return true.
    return true;
}

static void  _determine_seconds_until_next_background_task(uint8_t *times, uint8_t amount_of_times, void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    // okay this one's a doozy. times is an array of intervals in minutes, such as every 20 minutes, 35 minutes, etc.
    // determine the one that will be applicable the soonest and set its time until that happens in state->seconds_until_background_task.
    // this is to save power and not request a background task every darn minute.
    //
    // the logic is state->time_alive % time == 0
    uint32_t counter = rtc_counter_to_seconds(watch_rtc_get_counter());
    uint32_t timestamps[amount_of_times];
    uint32_t soonest_timestamp = 0;

    for (int i = 0; i < amount_of_times; i++) {
        timestamps[i] = counter + (times[i] - (state->time_alive % times[i])) * 60;
    }
    for (int i = 0; i < amount_of_times; i++) {
        if (timestamps[i] < soonest_timestamp || soonest_timestamp == 0) soonest_timestamp = timestamps[i];
    }
    state->seconds_until_background_task = soonest_timestamp - counter;
}

static void _schedule_background_task(void *context, uint32_t seconds_from_now) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    // TODO
    uint32_t now_ts = rtc_counter_to_seconds(watch_rtc_get_counter());
    state->next_background_task_ts = now_ts + seconds_from_now;
    // handled by advisory.
    // state->next_background_task_ts is set and will be used by it.
    // call cb_minute_alarm_fired() to make sure advisory is called!
#if __EMSCRIPTEN__
    EM_ASM({
    console.log('Pet face: background task planned ' + $0 + ' seconds from now');
    }, seconds_from_now);
#endif
}

static void _schedule_background_task_nosleep(void *context, uint32_t seconds_from_now) {
    // ideally, this should never, ever, be used.
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    movement_cancel_background_task();
    uint32_t now_ts = movement_get_utc_timestamp();
    uint32_t target_ts = now_ts + seconds_from_now;
    watch_date_time_t target_dt = watch_utility_date_time_from_unix_time(target_ts, 0);
    movement_schedule_background_task_for_face(state->face_idx, target_dt);

#if __EMSCRIPTEN__
    EM_ASM({
    console.log('Pet face: background task planned ' + $0 + ' seconds from now (nosleep)');
    }, seconds_from_now);
#endif
}

static bool _should_evolve(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    uint32_t awake_s;
    uint32_t evolve_s;

    if (state->phase > VIRTUAL_PET_PHASE_EGG && state->sickness > 0) return false;

    switch(state->phase) {
        case VIRTUAL_PET_PHASE_DEAD:
        case VIRTUAL_PET_PHASE_ADULT:
            return false;
        case VIRTUAL_PET_PHASE_EGG:
            awake_s = rtc_counter_to_seconds(state->awake_in_phase_counter);
            evolve_s = (60*state->minutes_to_hatch);
            return awake_s >= evolve_s;
        default:
            awake_s = rtc_counter_to_seconds(state->awake_in_phase_counter);
            evolve_s = (60*virtual_pet_phase_attributes[state->phase].phase_length);
            return awake_s >= evolve_s;
    }
}

uint8_t _get_time_hunger(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    switch(state->phase) {
        case VIRTUAL_PET_PHASE_EGG:
        case VIRTUAL_PET_PHASE_DEAD:
            return 0;
        case VIRTUAL_PET_PHASE_BABY:
        case VIRTUAL_PET_PHASE_CHILD:
            return virtual_pet_phase_attributes[state->phase].time_hunger;
        default:
            return virtual_pet_type_attributes[state->type].time_hunger;
    }
}
uint8_t _get_time_happiness(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    switch(state->phase) {
        case VIRTUAL_PET_PHASE_EGG:
        case VIRTUAL_PET_PHASE_DEAD:
            return 0;
        case VIRTUAL_PET_PHASE_BABY:
        case VIRTUAL_PET_PHASE_CHILD:
            return virtual_pet_phase_attributes[state->phase].time_happiness;
        default:
            return virtual_pet_type_attributes[state->type].time_happiness;
    }
}
uint8_t _get_time_needs(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    switch(state->phase) {
        case VIRTUAL_PET_PHASE_EGG:
        case VIRTUAL_PET_PHASE_DEAD:
            return 0;
        case VIRTUAL_PET_PHASE_BABY:
        case VIRTUAL_PET_PHASE_CHILD:
            return virtual_pet_phase_attributes[state->phase].time_needs;
        default:
            return virtual_pet_type_attributes[state->type].time_needs;
    }
}

static void _game_logic_loop(movement_event_t event, void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    bool playing_game = state->display_mode == VIRTUAL_PET_DISPLAY_MODE_GAME;
    rtc_counter_t current_counter = watch_rtc_get_counter();

    if (state->phase == VIRTUAL_PET_PHASE_DEAD || state->passing_counter > 0)
        return; // nothing happens while dead. no background task is needed.

    // Die when rtc counter overflows. Don't want the game to crash or time alive to be inaccurate!
    if (state->birth_counter != 0 && state->passing_counter == 0 && current_counter <= state->start_counter) {
        // Pets die of age at some point :'(
        // I assume this only happens 388 days after inserting the battery.
        _make_pet_dead(state);
        return;
    } 
    state->time_alive = get_time_alive(state, current_counter);
    
    watch_date_time_t current_time = movement_get_local_date_time();
    // Pet asleep? If so, don't continue any game logic.
    _set_is_sleeping(state, current_time);
    //these two are distinct. above sets animation, below here is if it's sleeping time (applicable to egg)
    if (_is_sleeping_time(state, current_time)) {
        if (playing_game) _exit_menu(state);

        // Next background task can be at the next full hour - because
        // that's when it will wake up
        uint8_t minutes_until_top_of_hour = 60 - current_time.unit.minute;
        state->seconds_until_background_task = minutes_until_top_of_hour * 60 - current_time.unit.second;
        _schedule_background_task(state, state->seconds_until_background_task);
        state->last_game_logic_loop_counter = current_counter;
    
        return;
    }
    if (state->last_game_logic_loop_counter != 0) {
        state->awake_in_phase_counter += current_counter - state->last_game_logic_loop_counter;
    }
    state->last_game_logic_loop_counter = current_counter;

    //if is egg
    if (state->phase == VIRTUAL_PET_PHASE_EGG) {
        virtual_pet_sleep_time_t sleep_time = virtual_pet_sleep_time[state->phase];
        if (current_time.unit.hour == sleep_time.hour_end && current_time.unit.minute == 0 && current_time.unit.second == 0) state->awake_in_phase_counter = 0;
        // now if it should not evolve..
        if (!_should_evolve(state)) {

            uint32_t phase_length_s = (60*state->minutes_to_hatch);
            uint16_t seconds_until_hatch = phase_length_s - rtc_counter_to_seconds(state->awake_in_phase_counter);
            
            if (seconds_until_hatch <= VIRTUAL_PET_PRE_HATCH_TIME + 1) {
                state->egg_idle_animation_chance = 9;
                bring_to_foreground(state);
                if (current_time.unit.second + seconds_until_hatch != 60) _schedule_background_task_nosleep(state, seconds_until_hatch);
            } else if (seconds_until_hatch <= (60 + VIRTUAL_PET_PRE_HATCH_TIME) && seconds_until_hatch > VIRTUAL_PET_PRE_HATCH_TIME) {
                _schedule_background_task_nosleep(state, seconds_until_hatch-VIRTUAL_PET_PRE_HATCH_TIME);
            } else {
                state->seconds_until_background_task = seconds_until_hatch;
                state->seconds_until_background_task -= (state->seconds_until_background_task > 60) ? 60 : 0;
                _schedule_background_task(state, state->seconds_until_background_task);
            }
            return;
        }
    }

    _set_needs_attention(state);

    // Evolution/transformation
    if (_should_evolve(state)) {
        state->phase++;
        if (state->phase == VIRTUAL_PET_PHASE_ADULT) _set_pet_type(state);
        if (state->phase == VIRTUAL_PET_PHASE_BABY) state->birth_counter = current_counter;

        int8_t *note_sequence = virtual_pet_transformation_tune;
        if (state->phase == VIRTUAL_PET_PHASE_BABY) note_sequence = virtual_pet_hatch_tune;

        bring_to_foreground(state);
        if (state->buzzer_enabled) movement_play_sequence(note_sequence, BUZZER_PRIORITY_SIGNAL);
        state->awake_in_phase_counter = 0;
        clear_bottom();
    }

    state->time_alive = get_time_alive(state, current_counter);

    if (state->time_alive > 0 && current_time.unit.second == 0) {
        uint8_t time_hunger = _get_time_hunger(state);
        uint8_t time_happiness = _get_time_happiness(state);
        uint8_t time_needs = _get_time_needs(state);

        // Hunger
        if (state->time_alive % time_hunger == 0) {
            if (state->hunger == 0) {
                if (state->phase == VIRTUAL_PET_PHASE_CHILD) state->care_mistakes++;
                if (!playing_game) _make_pet_sick_with_chance(state, VIRTUAL_PET_METER_EMPTY_SICKNESS_CHANCE);
            } else {
                state->hunger--;
                if (state->hunger == 0) {
                    if (!playing_game) _ask_for_attention(state);
                }
            }
        }
        // Happiness
        if (state->time_alive % time_happiness == 0) {
            if (state->snacks_today == VIRTUAL_PET_MAX_SNACKS_DAILY) _make_pet_dead(state);
            if (state->happiness == 0) {
                if (state->phase == VIRTUAL_PET_PHASE_CHILD) state->care_mistakes++;
                if (!playing_game) _make_pet_sick_with_chance(state, VIRTUAL_PET_METER_EMPTY_SICKNESS_CHANCE);
            } else {
                state->happiness--;
                if (state->happiness == 0) {
                    if (!playing_game) _ask_for_attention(state);
                }
            }
        }
        // Needs (poop)
        if (state->time_alive % time_needs == 0) {
            if (state->needs == VIRTUAL_PET_MAX_NEEDS) {
                if (state->poops == VIRTUAL_PET_MAX_POOPS) {
                    if (!playing_game) _make_pet_sick_with_chance(state, VIRTUAL_PET_POOP_SICKNESS_CHANCE);
                } else {
                    state->poops++;
                    state->needs = 0;
                    if (state->time_alive < 5 && state->phase == VIRTUAL_PET_PHASE_BABY) {
                        if (!playing_game) _make_pet_sick_with_chance(state, 9);
                    }
                }
            } else {
                state->needs++;
            }
        }
        // Randomly getting sick
        if (state->time_alive % 1440 == 0 && state->phase == VIRTUAL_PET_PHASE_ADULT) {
            if (!playing_game) _make_pet_sick_with_chance(state, VIRTUAL_PET_ADULT_RANDOM_SICKNESS_CHANCE);
        }

        // plan background task for next time
        uint32_t phase_length_s = 0;

        //determine time for next task.
        uint8_t times[3] = {
            time_hunger,
            time_happiness,
            time_needs,
        };
        _determine_seconds_until_next_background_task(times, 3, state);
        _schedule_background_task(state, state->seconds_until_background_task - current_time.unit.second);
        
        if (state->phase < VIRTUAL_PET_PHASE_ADULT) {
            uint32_t s_until_evolve = 0;
            phase_length_s = (60*virtual_pet_phase_attributes[state->phase].phase_length);
            
            s_until_evolve = phase_length_s - rtc_counter_to_seconds(state->awake_in_phase_counter);
            if (s_until_evolve < state->seconds_until_background_task) {
                state->seconds_until_background_task = s_until_evolve;
                _schedule_background_task_nosleep(state, state->seconds_until_background_task);
            }
        }
    } else if (state->time_alive == 0) {
        // wish I had found this edge case sooner.. If debugging and starting from a phase different
        // from egg, there would be no new background task planned. LOL
        state->seconds_until_background_task = 60 - current_time.unit.second;
        _schedule_background_task(state, state->seconds_until_background_task);
    }
    return;
}

void virtual_pet_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(virtual_pet_state_t));
        memset(*context_ptr, 0, sizeof(virtual_pet_state_t));
        virtual_pet_state_t *state = (virtual_pet_state_t *)*context_ptr;
        state->face_idx = watch_face_index;
        _setup_pet(state);
        state->display_mode = VIRTUAL_PET_DISPLAY_MODE_PET;
        state->buzzer_enabled = VIRTUAL_PET_DEFAULT_BUZZER_ENABLED;
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
#if __EMSCRIPTEN__
    // simulator only: seed the random number generator
    time_t t;
    srand((unsigned) time(&t));
#endif
}

void virtual_pet_face_activate(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    _exit_menu(state);
    // Handle any tasks related to your watch face coming on screen.
}

bool virtual_pet_face_loop(movement_event_t event, void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;

    switch (event.event_type) {
        case EVENT_LIGHT_BUTTON_UP:
            // gotta turn off the led
            movement_default_loop_handler(event);
            // fall through
        case EVENT_ALARM_BUTTON_UP:
            if (state->came_out_of_sleep) state->came_out_of_sleep = false;
            break;
        case EVENT_BACKGROUND_TASK:
            // Game logic in background
            _game_logic_loop(event, state);
            // for low power display drawing, it's EVENT_LOW_ENERGY_UPDATE
#if VIRTUAL_PET_BACKGROUND_TASK_TUNE
            movement_play_sequence(virtual_pet_background_task_tune, BUZZER_PRIORITY_SIGNAL);
#endif
            return true; // Don't do any display stuff for background tasks
        case EVENT_TIMEOUT:
            // event_timeout only gets called when the face is not the main face.
            // If we want to close the menu after a while, we would have to implement that ourselves.
            _exit_game(event, state);
            movement_move_to_face(0);
            if (state->came_out_of_sleep) go_back_to_sleep(state);
            break;
        case EVENT_ACTIVATE:
            if (state->start_counter == 0) {
                // start vpet!
                // don't play boot sound when it's the main face. no need to drive it and it breaks the emulator sound otherwise
                if (state->face_idx > 0 && state->buzzer_enabled) movement_play_sequence(virtual_pet_start_tune, BUZZER_PRIORITY_SIGNAL);
                _setup_pet(state);
                state->start_counter = watch_rtc_get_counter();
                // set birth counter if not egg (otherwise only gets set when hatching, which doesn't happen when starting from baby or larger.)
                if (state->phase > VIRTUAL_PET_PHASE_EGG && state->birth_counter == 0) state->birth_counter = state->start_counter;
            }
            break;
        default:
            break;
    }
    // Render display, button actions.
    switch (state->display_mode) {
        case VIRTUAL_PET_DISPLAY_MODE_PET:
            return _pet_loop(event, state);
        case VIRTUAL_PET_DISPLAY_MODE_STATUS_MENU:
            return _status_menu_loop(event, state);
        case VIRTUAL_PET_DISPLAY_MODE_ACTION_MENU:
            return _action_menu_loop(event, state);
        case VIRTUAL_PET_DISPLAY_MODE_GAME:
            return _game_mode_loop(event, state);
        default:
            break;
    }
    return true;
}

void virtual_pet_face_resign(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    if (state->phase == VIRTUAL_PET_PHASE_DEAD) return; // no bg task while dead.
    state->is_eating = false;

    // one minute until background task, in which the actual time until next background task will be calculated.
    state->seconds_until_background_task = 1;
    _schedule_background_task(state, state->seconds_until_background_task);

    // handle any cleanup before your watch face goes off-screen.
}

// handled every top of minute.
movement_watch_face_advisory_t virtual_pet_face_advise(void *context) {
    virtual_pet_state_t *state = (virtual_pet_state_t *)context;
    movement_watch_face_advisory_t retval = { 0 };
    uint32_t now_ts = rtc_counter_to_seconds(watch_rtc_get_counter());
    if (now_ts >= state->next_background_task_ts && state->start_counter != 0) {
        retval.wants_background_task = true;
    }
    return retval;
}
