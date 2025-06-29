/*
 * MIT License
 *
 * Copyright (c) 2022 Spencer Bywater
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
#include "probability_face.h"
#include "watch_common_display.h"

#define DEFAULT_DICE_SIDES 2
#define PROBABILITY_ANIMATION_TICK_FREQUENCY 8
#define TAP_DETECTION_SECONDS 5
#define ANIMATION_FRAMES 4
#define SEGMENTS_PER_FRAME 2
const uint16_t NUM_DICE_TYPES = 8; // Keep this consistent with # of dice types below
const uint16_t DICE_TYPES[] = {2, 4, 6, 8, 10, 12, 20, 100};

// Animation frame data: each frame defines which pixels to set
// Each frame can have up to SEGMENTS_PER_FRAME pixels
// Use {255, 255} to indicate end of pixel list for a frame
typedef struct {
    uint8_t com;
    uint8_t seg;
} com_seg_t;

static const com_seg_t classic_lcd_animation_frames[ANIMATION_FRAMES][SEGMENTS_PER_FRAME] = {
    // Frame 0: Second #1 F and C
    {{1, 4}, {1, 6}},
    // Frame 1: Second #1 A and D
    {{2, 4}, {0, 6}},
    // Frame 2: Second #1 B and E
    {{2, 5}, {0, 5}},
    // Frame 3: No pixels set (end animation)
    {{255, 255}, {255, 255}}
};

static const com_seg_t custom_lcd_animation_frames[ANIMATION_FRAMES][SEGMENTS_PER_FRAME] = {
    // Frame 0: Second #1 F and C
    {{2, 6}, {2, 7}},
    // Frame 1: Second #1 A and D
    {{3, 6}, {0, 7}},
    // Frame 2: Second #1 B and E
    {{3, 7}, {0, 6}},
    // Frame 3: No pixels set (end animation)
    {{255, 255}, {255, 255}}
};

// --------------
// Custom methods
// --------------

static void abort_tap_detection(probability_state_t *state) {
    state->tap_detection_ticks = 0;
    movement_disable_tap_detection_if_available();
}

static void cycle_dice_type(probability_state_t *state) {
    // Change how many sides the die has
    for (int i = 0; i < NUM_DICE_TYPES; i++)
    {
        if (DICE_TYPES[i] == state->dice_sides)
        {
            if (i == NUM_DICE_TYPES - 1)
            {
                state->dice_sides = DICE_TYPES[0];
            }
            else
            {
                state->dice_sides = DICE_TYPES[i + 1];
            }
            break;
        }
    }
    state->rolled_value = 0;
}

static void display_dice_roll(probability_state_t *state)
{
    char buf[8];

    // Display die type in top right position
    if (state->dice_sides == 100) {
        // Show "00" for d100
        watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, "00", " C");
    } else {
        sprintf(buf, "%2d", state->dice_sides);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }

    // Display rolled value
    if (state->rolled_value == 0) {
        // No roll yet - show dashes
        watch_display_text(WATCH_POSITION_BOTTOM, "----  ");
    } else if (state->dice_sides == 2) {
        // Coin flip: show "Heads" or "Tails" across hours, minutes, and first digit of seconds
        if (state->rolled_value == 1) {
            // Heads
            watch_display_text(WATCH_POSITION_BOTTOM, "HEAdS ");
        } else {
            // Tails
            watch_display_text(WATCH_POSITION_BOTTOM, "TAiLS ");
        }
    } else {
        // Normal case: show rolled value using hours and minutes
        if (state->rolled_value == 100) {
            // Show " 1:00" for 100
            watch_display_text(WATCH_POSITION_BOTTOM, " 100");
        } else {
            // Show "  :XX" for 1-99
            sprintf(buf, "%4d", state->rolled_value);
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
        }
    }
}

static void generate_random_number(probability_state_t *state)
{
// Emulator: use rand. Hardware: use arc4random.
#if __EMSCRIPTEN__
    state->rolled_value = rand() % state->dice_sides + 1;
#else
    state->rolled_value = arc4random_uniform(state->dice_sides) + 1;
#endif
}

static void roll_dice(probability_state_t *state)
{
    generate_random_number(state);
    state->is_rolling = true;
    // Dice rolling animation begins on next tick and new roll will be displayed on completion
    movement_request_tick_frequency(PROBABILITY_ANIMATION_TICK_FREQUENCY);
}

static void display_dice_roll_animation(probability_state_t *state)
{
    if (state->is_rolling)
    {
        const com_seg_t (*animation_frames)[SEGMENTS_PER_FRAME] = classic_lcd_animation_frames;
        if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
            animation_frames = custom_lcd_animation_frames;
        }

        // Clear main display areas on first frame
        if (state->animation_frame == 0)
        {
            watch_display_text(WATCH_POSITION_HOURS, "  ");
            watch_display_text(WATCH_POSITION_MINUTES, "  ");
            watch_display_text(WATCH_POSITION_SECONDS, "  ");
        }

        // Clear pixels from previous frame (except on first frame)
        if (state->animation_frame > 0)
        {
            const com_seg_t *prev_frame = animation_frames[state->animation_frame - 1];
            for (int i = 0; i < SEGMENTS_PER_FRAME; i++)
            {
                if (prev_frame[i].com == 255) break; // End of pixel list
                watch_clear_pixel(prev_frame[i].com, prev_frame[i].seg);
            }
        }

        // Set pixels for current frame
        const com_seg_t *current_frame = animation_frames[state->animation_frame];
        for (int i = 0; i < SEGMENTS_PER_FRAME; i++)
        {
            if (current_frame[i].com == 255) break; // End of pixel list
            watch_set_pixel(current_frame[i].com, current_frame[i].seg);
        }

        // Advance to next frame
        state->animation_frame++;

        // Check if animation is complete
        if (state->animation_frame >= ANIMATION_FRAMES)
        {
            state->animation_frame = 0;
            state->is_rolling = false;
            movement_request_tick_frequency(1);
            display_dice_roll(state);
        }
    }
}

// ---------------------------
// Standard watch face methods
// ---------------------------
void probability_face_setup(uint8_t watch_face_index, void **context_ptr)
{
    (void)watch_face_index;
    if (*context_ptr == NULL)
    {
        *context_ptr = malloc(sizeof(probability_state_t));
        memset(*context_ptr, 0, sizeof(probability_state_t));
    }
// Emulator only: Seed random number generator
#if __EMSCRIPTEN__
    srand(time(NULL));
#endif
}

void probability_face_activate(void *context)
{
    probability_state_t *state = (probability_state_t *)context;

    state->dice_sides = DEFAULT_DICE_SIDES;
    state->rolled_value = 0;

    // Display face identifier
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "Prb", "PR");

    // Set tick frequency to 1 for proper tap detection timing
    movement_request_tick_frequency(1);

    // Enable tap detection for a few seconds when face is activated
    if (movement_enable_tap_detection_if_available()) {
        state->tap_detection_ticks = TAP_DETECTION_SECONDS;
    }
}

bool probability_face_loop(movement_event_t event, void *context)
{
    probability_state_t *state = (probability_state_t *)context;

    if (state->is_rolling && event.event_type != EVENT_TICK)
    {
        return true;
    }

    switch (event.event_type)
    {
    case EVENT_ACTIVATE:
        display_dice_roll(state);
        break;
    case EVENT_TICK:
        display_dice_roll_animation(state);

        if (!state->is_rolling && state->tap_detection_ticks > 0) {
            state->tap_detection_ticks--;
            if (state->tap_detection_ticks == 0) {
                movement_disable_tap_detection_if_available();
            }
        }
        break;
    case EVENT_LIGHT_BUTTON_DOWN:
        // Cycle through die types
        cycle_dice_type(state);
        display_dice_roll(state);
        break;
    case EVENT_ALARM_BUTTON_UP:
        // Roll the die
        roll_dice(state);
        break;
    case EVENT_SINGLE_TAP:
        // Single tap cycles die type
        cycle_dice_type(state);
        display_dice_roll(state);

        // Reset tap detection timer to keep accelerometer active
        state->tap_detection_ticks = TAP_DETECTION_SECONDS;
        break;
    case EVENT_LOW_ENERGY_UPDATE:
        watch_display_text(WATCH_POSITION_BOTTOM, "SLEEP ");
        break;
    default:
        movement_default_loop_handler(event);
        break;
    }

    return true;
}

void probability_face_resign(void *context)
{
    probability_state_t *state = (probability_state_t *)context;

    // Disable tap detection to save battery
    abort_tap_detection(state);
}
