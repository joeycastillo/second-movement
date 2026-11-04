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

#pragma once

#include "movement.h"
#include "watch_utility.h"

/*
 * The virtual pet face is a virtual pet, which can grow, live happily depending on how you take care of it,
 * or die.
 *
 * Life phases:
 * - Egg (5-120 min)
 * - Baby (60 min)
 * - Child (~2 days)
 * - Adult
 * 
 * Depending on how you take care of it, you can get a different type of pet.
 * Types:
 * - Needy (More than 4 care mistakes)
 * - Average (2-4 care mistakes)
 * - Easy (less than 2 care mistakes)
 * The type describes the amount of care one needs. If you take bad care of your pet, it will need more care in the future.
 * Care mistakes only count in the child phase.
 * 
 * Happiness: The pet will appear:
 * - Sad if happiness is below 2
 * - Neutral if happiness is between 2 and 4
 * - Happy is happiness is above 4 (max 6)
 * Happiness can be improved with snacks (dont overfeed!) or by playing a game. This stat will go down with time.
 * 
 * Hunger: Your pet will get hungry over time, just like it will be less happy if you don't play with it!
 * 
 * Sickness: Sometimes your pet gets sick. It happens when you don't take good care of your pet. To heal your pet, give it medicine.
 * Sickness will be shown as a skull in the top left (weekday digits).
 * 
 * Poop: Sometimes your pet will poop. It will be on the screen, usually in the lower or upper right corner, as a box with a visible odor going back and forth.
 * To remove poop, clean the pet. If you fail to remove poop, the pet might become sick.
 * 
 * Attention: When your pet needs attention, it will make a noise and the bell indicator will appear. If you don't take care of your pet within 15 minutes,
 * it might get sick! (It counts as a care mistake)
 * 
 * Sleep: Just like you, your pet sleeps. In every life phase it will sleep a different amount of time.
 * Sleep is visible on with "ZZ" in the top left. A bed will be visible. You can't wake it up, it will wake up on its own.
 * 
 * Pause: If you want to store your watch, or just need to pause for some reason, you can pause the pet. Go to the action menu and pause.
 * When paused, the pet will be sleeping, even in daytime. To unpause, press the alarm button.
 * If the pet hasn't been seen by its caretaker in about a day, it will automatically pause.
 * 
 * Buttons:
 * Press Light to open the status menu. In the status menu, you can advance the menu by pressing the Light button again.
 * When at the last menu item, pressing Light will exit the menu and show the pet.
 * The following stats can be seen:
 * Hu - hunger
 * Ha - happiness
 * Al - days alive (age)
 * Ph - type/phase
 * 
 * Press Alarm to open the action menu. Here you can feed your pet, play with it or give it medicine. Use Light to advance, Alarm to confirm your choice.
 * When at the last item, advancing will close the menu.
 * 
 * Unfortunately, not all pets life forever. This watch face keeps track of time using the RTC counter. This is uninterrupted and won't change when the watch time or date changes.
 * This also means that every 388 days after inserting the battery, the pet will die of "age", or in this case, of integer overflow.
 * 
 * Game (higher/lower):
 * If your happiness level is low, you can increase it by playing a game. The game is always a "higher/lower" card game.
 * The game has 5 rounds. In each round, a number is displayed. Guess if the next number will be higher or lower.
 * - Press Light for higher, Mode for lower.
 * - Press Alarm to go back.
 * At the end of the game, the score is displayed.
 * Did you win? Your pet will be very happy!
 * 
 * By the way, this watch face will consume more power than your average watch face.
 * Game updates are always done in the background and you should not expect this to last a long time on a single CR2016 coin cell.
 * Game updates are done every minute, only when needed. But the logic can be expensive in terms of processing power.
 */


/* 
 * DO NOT TOUCH THESE
 * if you do, you might have to change the size of the corresponding fields in virtual_pet_state_t.
 */

#define VIRTUAL_PET_CHARACTER_STRING_LENGTH 3
#define VIRTUAL_PET_DEFAULT_BUZZER_ENABLED 1
#define VIRTUAL_PET_GRIEVING_TIME 2 // minutes after death, the game won't respond to anything.

#define VIRTUAL_PET_MAX_HUNGER 6
#define VIRTUAL_PET_MAX_HAPPINESS 6
#define VIRTUAL_PET_MAX_NEEDS 6
#define VIRTUAL_PET_MAX_POOPS 2
#define VIRTUAL_PET_MAX_SICKNESS 5
#define VIRTUAL_PET_MAX_SNACKS_DAILY 35

#define VIRTUAL_PET_POOP_SICKNESS_CHANCE 3 // 0-9
#define VIRTUAL_PET_SNACK_SICKNESS_CHANCE 1 // 0-9
#define VIRTUAL_PET_METER_EMPTY_SICKNESS_CHANCE 0 // 0-9
#define VIRTUAL_PET_ADULT_RANDOM_SICKNESS_CHANCE 1 // 0-9
#define VIRTUAL_PET_MEDS_NOT_WORKING_CHANCE 3 // 0-9
#define VIRTUAL_PET_SLEEPING_ANIMATION_CHANCE 0 // 0-9

#define VIRTUAL_PET_MAX_GAME_ROUNDS 5
#define VIRTUAL_PET_GAME_WIN_SCORE 3

// For debugging or development purposes only. Don't cheat on your pet!
#define VIRTUAL_PET_SHOW_MINUTES_ALIVE 0
#define VIRTUAL_PET_RANDOM_HATCH_TIME true
#define VIRTUAL_PET_HATCH_TIME 3 // overridden by above
#define VIRTUAL_PET_PRE_EVOLVE_TIME 12 // seconds before hatching, make egg more active and move to foreground
#define VIRTUAL_PET_BACKGROUND_TASK_TUNE false
#define VIRTUAL_PET_SECONDS_AFTER_ACTIVATE 3
#define VIRTUAL_PET_CRYO_TIME 72000 // seconds of inactivity after the pet goes into a very deep sleep (20h)

#define VIRTUAL_PET_STARTING_PHASE VIRTUAL_PET_PHASE_EGG
#define VIRTUAL_PET_STARTING_TYPE VIRTUAL_PET_TYPE_EASY // Only applicable if starting phase is adult
#define VIRTUAL_PET_STARTING_HUNGER 1
#define VIRTUAL_PET_STARTING_HAPPINESS 1
#define VIRTUAL_PET_STARTING_NEEDS 6
#define VIRTUAL_PET_STARTING_POOPS 1
#define VIRTUAL_PET_STARTING_SICKNESS 0
#define VIRTUAL_PET_STARTING_ATTENTION 0
#define VIRTUAL_PET_STARTING_CARE_MISTAKES 0
#define VIRTUAL_PET_STARTING_GAMES_PLAYED 0

#define VIRTUAL_PET_GAME_REACTION_TICKS 1
#define VIRTUAL_PET_GAMES_PLAYED_FOR_SECRET_TYPE 20

typedef struct {
    char menu_text[7];
    char menu_text_fallback[7];
} virtual_pet_bottom_text_t;

typedef struct {
    char menu_text[4];
    char menu_text_fallback[3];
} virtual_pet_top_left_text_t;

typedef enum {
    VIRTUAL_PET_PHASE_EGG = 0,
    VIRTUAL_PET_PHASE_BABY,
    VIRTUAL_PET_PHASE_CHILD,
    VIRTUAL_PET_PHASE_ADULT,
    VIRTUAL_PET_PHASE_DEAD,
} virtual_pet_phase;

typedef struct {
    uint16_t phase_length;
    uint8_t time_hunger;
    uint8_t time_happiness;
    uint8_t time_needs;
} virtual_pet_phase_attribute_t;

typedef enum {
    VIRTUAL_PET_TYPE_NEEDY = 0,
    VIRTUAL_PET_TYPE_AVERAGE,
    VIRTUAL_PET_TYPE_EASY,
    VIRTUAL_PET_TYPE_SECRET,
} virtual_pet_type;

typedef struct {
    uint8_t time_hunger;
    uint8_t time_happiness;
    uint8_t time_needs;
} virtual_pet_type_attribute_t;

/*
 * Emotions, used to make code easier to read. Returned by function.
 */
typedef enum {
    VIRTUAL_PET_EMOTION_SAD = 0,
    VIRTUAL_PET_EMOTION_NEUTRAL,
    VIRTUAL_PET_EMOTION_HAPPY,
    VIRTUAL_PET_EMOTION_SLEEPING,
    VIRTUAL_PET_EMOTION_EATING,
} virtual_pet_emotion;

typedef struct {
    uint8_t hour_start;
    uint8_t hour_end;
} virtual_pet_sleep_time_t;

/*
 * Display modes. These change button behaviour.
 */
typedef enum {
    VIRTUAL_PET_DISPLAY_MODE_PET = 0,
    VIRTUAL_PET_DISPLAY_MODE_STATUS_MENU,
    VIRTUAL_PET_DISPLAY_MODE_ACTION_MENU,
    VIRTUAL_PET_DISPLAY_MODE_GAME,
} virtual_pet_display_mode;

/*
 * Status menu, action menu and their items
 */
typedef enum {
    VIRTUAL_PET_ACTION_MENU_ITEM_EAT = 0,
    VIRTUAL_PET_ACTION_MENU_ITEM_SNACK,
    VIRTUAL_PET_ACTION_MENU_ITEM_GAME,
    VIRTUAL_PET_ACTION_MENU_ITEM_CLEAN,
    VIRTUAL_PET_ACTION_MENU_ITEM_MEDICATION,
    VIRTUAL_PET_ACTION_MENU_ITEM_BUZZER,
    VIRTUAL_PET_ACTION_MENU_ITEM_PAUSE,
    VIRTUAL_PET_ACTION_MENU_LENGTH,
} virtual_pet_action_menu_item;

typedef enum {
    VIRTUAL_PET_STATUS_MENU_HUNGER = 0,
    VIRTUAL_PET_STATUS_MENU_HAPPINESS,
    VIRTUAL_PET_STATUS_MENU_AGE,
    VIRTUAL_PET_STATUS_MENU_TYPE,
    VIRTUAL_PET_STATUS_MENU_LENGTH,
} virtual_pet_status_menu_item;

typedef enum {
    VIRTUAL_PET_GAME_MODE_ROUND_ACTIVE, // press higher or lower
    VIRTUAL_PET_GAME_MODE_ROUND_SHOW_GUESS, // show number that it was
    VIRTUAL_PET_GAME_MODE_REACTION, // react to button press with happy or sad noise
    VIRTUAL_PET_GAME_MODE_SCORE, // show the score at the end
} virtual_pet_game_mode;

typedef struct {
    uint8_t display_offset;
    uint8_t idle_animation_chance;

    uint8_t *sad;
    uint8_t *neutral;
    uint8_t *happy;
    uint8_t *sleeping;
    uint8_t *eating;
    bool can_sleep;
    uint8_t food_position;
} virtual_pet_phase_animation_t;

/*
 * The pet and game status
 */
typedef struct {
    // Anything you need to keep track of, put it here!
    uint8_t face_idx; // needed for boot sound
    bool face_active;
    uint32_t activate_ts;
    bool paused;

    uint32_t next_background_task_ts;

    rtc_counter_t start_counter : 32; // rtc counter when starting
    rtc_counter_t birth_counter : 32; // rtc counter when egg hatched
    rtc_counter_t awake_in_phase_counter : 32; 
    rtc_counter_t last_game_logic_loop_counter : 32;
    rtc_counter_t passing_counter : 32; // rtc counter when died
    uint8_t minutes_to_hatch : 8;
    uint32_t seconds_until_background_task : 32; // = when requesting a background task, this is the amount of seconds until we are in the exact minute for the next task.

    uint32_t time_alive;
    virtual_pet_phase phase : 3;
    virtual_pet_type type : 2;

    uint8_t hunger : 3;
    uint8_t happiness : 3;
    uint8_t needs : 3; // hidden, for poops
    uint8_t poops : 2;
    uint8_t sickness : 3;
    bool needs_attention;
    uint8_t care_mistakes : 3;
    bool is_sleeping;
    bool is_eating;
    uint8_t snacks_today : 6;

    virtual_pet_display_mode display_mode : 2;
    uint8_t current_menu_index : 3; // works for both menus
    bool buzzer_enabled;
    uint8_t current_character_animation_frame : 4;

    uint8_t current_poop_animation_frame : 4;
    uint8_t current_sickness_animation_frame : 1;
    uint8_t current_food_animation_frame : 4;
    uint8_t current_little_ghost_animation_frame : 1;
    uint8_t current_sleeping_animation_frame : 2;
    uint8_t current_eating_animation_frame : 4;

    virtual_pet_game_mode game_display_mode : 3;
    uint8_t game_round : 3;
    uint8_t game_score : 3;
    uint8_t game_mode_ticks : 4;

    uint8_t game_shown_number : 4;
    uint8_t game_hidden_number : 4;
    bool game_round_won;
    uint8_t egg_idle_animation_chance : 4;

    uint8_t games_played : 5; // When 20, secret type is unlocked in child phase.
} virtual_pet_state_t;


void virtual_pet_face_setup(uint8_t watch_face_index, void ** context_ptr);
void virtual_pet_face_activate(void *context);
bool virtual_pet_face_loop(movement_event_t event, void *context);
void virtual_pet_face_resign(void *context);
movement_watch_face_advisory_t virtual_pet_face_advise(void *context);

#define virtual_pet_face ((watch_face_t){ \
    virtual_pet_face_setup, \
    virtual_pet_face_activate, \
    virtual_pet_face_loop, \
    virtual_pet_face_resign, \
    virtual_pet_face_advise, \
})
