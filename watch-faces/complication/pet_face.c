/*
 * MIT License
 *
 * Copyright (c) 2026 Daniel Schrempf
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
#include "pet_face.h"
#include "watch_common_display.h"

// ----------
// ANIMATIONS
// ----------

static const pet_layer_def_t _pet_layers[PET_LAYER_COUNT] = {
    [PET_LAYER_CHARACTER] = {
        //   0     1     2     3     4     5     6     7     8     9
        {0x00, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        PET_FRAME_COLON,
    },
    [PET_LAYER_STATUS] = {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, SEG_G | SEG_B | SEG_C},
        0,
    },
};

static const uint8_t _pet_food_pips[PET_FOOD_MAX] = {SEG_B, SEG_C, SEG_F, SEG_E};

static const pet_frame_t _pet_frames_happy[] = {
    //  0         1         2         3         4         5         6         7         8         9            flags            hold
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 6},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 5},
};

static const pet_frame_t _pet_frames_confused[] = {
    //  0         1         2         3         4         5         6         7         8         9            flags            hold
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 8},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_C | SEG_F, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 8},
};

static const pet_frame_t _pet_frames_upset[] = {
    //  0         1         2         3         4         5         6         7         8         9            flags            hold
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_A | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_A | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_A | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_A | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_A | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_A | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 3},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_A | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 8},
};

static const pet_frame_t _pet_frames_angry[] = {
    //  0         1         2         3         4         5         6         7         8         9            flags            hold
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_A | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_A | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_A | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_A | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_A | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_A | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 3},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_A | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 8},
};

static const pet_frame_t _pet_frames_dead[] = {
    //  0         1         2         3         4         5         6         7         8         9            flags            hold
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C | SEG_G, SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_A | SEG_D}, 0, 4},
};

static const pet_frame_t _pet_frames_resurrect[] = {
    //  0         1         2         3         4         5         6         7         8         9            flags            hold
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C | SEG_G, SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_A | SEG_D}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C | SEG_G, SEG_G, SEG_A | SEG_D | SEG_E | SEG_F}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C | SEG_G, SEG_G}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C | SEG_G}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_G}, 0, 3},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E | SEG_G}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_D, SEG_E | SEG_G}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_D | SEG_E, SEG_E}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_D | SEG_E | SEG_F, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B, SEG_E | SEG_F, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B, SEG_F, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_F, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_E | SEG_F, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D, SEG_E | SEG_F, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_E, SEG_E, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_C | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 6},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
};

static const pet_frame_t _pet_frames_play_small[] = {
    //  0         1         2         3         4         5         6         7         8         9            flags            hold
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 3},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_F, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_D, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 2},
};

static const pet_frame_t _pet_frames_play_big[] = {
    //  0         1         2         3         4         5         6         7         8         9            flags            hold
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 2},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A, SEG_NONE, SEG_B, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_F, SEG_NONE, SEG_A, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_E, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_D | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E, SEG_NONE, SEG_D, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_D, SEG_NONE, SEG_C, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_D, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
};

static const pet_frame_t _pet_frames_eat[] = {
    //  0         1         2         3         4         5         6         7         8         9            flags            hold
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 4},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 2},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_E | SEG_F, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D | SEG_E | SEG_F, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 2},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C | SEG_E | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C | SEG_E | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C | SEG_E | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 5},
};

static const pet_frame_t _pet_frames_kiss[] = {
    //  0         1         2         3         4         5         6         7         8         9            flags            hold
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 4},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 2},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_D, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_D | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_A | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_A, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 4},
};

static const pet_frame_t _pet_frames_snore[] = {
    //  0         1         2         3         4         5         6         7         8         9            flags            hold
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_F | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 8},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_D, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_D | SEG_G, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_A | SEG_G, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_A, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 3},
};

static const pet_frame_t _pet_frames_wake[] = {
    //  0         1         2         3         4         5         6         7         8         9            flags            hold
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 3},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_C, SEG_E | SEG_F, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_A | SEG_D | SEG_E | SEG_F, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_D, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_A | SEG_D | SEG_E | SEG_F, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_D | SEG_G, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_A | SEG_D | SEG_E | SEG_F, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_G, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_A | SEG_D | SEG_E | SEG_F, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_A | SEG_D | SEG_E | SEG_F, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_C, SEG_E | SEG_F, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 3},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_G, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D | SEG_E | SEG_F, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
};

static const pet_frame_t _pet_frames_poo[] = {
    //  0         1         2         3         4         5         6         7         8         9            flags            hold
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 2},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 3},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_G, SEG_NONE, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_G, SEG_NONE}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_G}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_G, SEG_NONE, SEG_B | SEG_C}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_G, SEG_B | SEG_C}, 0, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_B | SEG_C | SEG_G}, PET_FRAME_COLON, 3},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_C | SEG_D, SEG_NONE, SEG_NONE, SEG_B | SEG_C | SEG_G}, PET_FRAME_COLON, 2},
};

static const pet_frame_t _pet_frames_barf[] = {
    //  0         1         2         3         4         5         6         7         8         9            flags            hold
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_D | SEG_E | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_C | SEG_D | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_D | SEG_E | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_C | SEG_D | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_B | SEG_D | SEG_E | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_C | SEG_D | SEG_F | SEG_G, SEG_NONE, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_E | SEG_F, SEG_NONE, SEG_NONE, SEG_NONE}, 0, 3},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D | SEG_E | SEG_F, SEG_D, SEG_NONE, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D | SEG_E | SEG_F, SEG_D | SEG_G, SEG_D, SEG_NONE}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D | SEG_E | SEG_F, SEG_G, SEG_D | SEG_G, SEG_D}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D | SEG_E | SEG_F, SEG_D, SEG_NONE, SEG_C | SEG_D | SEG_G}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D | SEG_E | SEG_F, SEG_D, SEG_NONE, SEG_B | SEG_C | SEG_G}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D | SEG_E | SEG_F, SEG_D | SEG_G, SEG_D, SEG_B | SEG_C}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_A | SEG_D | SEG_E | SEG_F, SEG_G, SEG_D | SEG_G, SEG_B | SEG_C | SEG_D}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_G, SEG_B | SEG_C | SEG_D | SEG_G}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_B | SEG_C | SEG_G}, PET_FRAME_COLON, 1},
    {{SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_NONE, SEG_B | SEG_C, SEG_NONE, SEG_NONE, SEG_B | SEG_C}, PET_FRAME_COLON, 2},
};

static const pet_frame_t _pet_frames_pile[] = {
    //  0  1  2  3  4  5  6  7  8  9                                flags  hold
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, SEG_G | SEG_B | SEG_C}, 0, PET_ANIM_HZ},
};

static const pet_frame_t _pet_frames_puddle[] = {
    //  0  1  2  3  4  5  6  7  8  9                                flags  hold
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, SEG_B | SEG_C}, 0, PET_ANIM_HZ},
};

// ----------
// SOUND CUES
// ----------

static const pet_cue_t _pet_cues_snore[] = {
    {PET_SOUND_SNORE_IN, 0, 0, false, true},
    {PET_SOUND_SNORE_OUT, 1, SEG_D, false, true},
};

static const pet_cue_t _pet_cues_kiss[] = {
    {PET_SOUND_KISS, 6, SEG_G, false, true},
};

static const pet_cue_t _pet_cues_eat[] = {
    {PET_SOUND_EAT_GULP, 7, 0xFF, true, true},
    {PET_SOUND_EAT_CHEW, 6, SEG_G, false, false},
};

static const pet_cue_t _pet_cues_barf[] = {
    {PET_SOUND_BARF_UHOH, 0, 0, false, true},
    {PET_SOUND_BARF_SLIDE, 7, SEG_D, false, true},
};

static const pet_cue_t _pet_cues_poo[] = {
    {PET_SOUND_POO, 9, SEG_B | SEG_C, false, true},
};

static const pet_cue_t _pet_cues_play_small[] = {
    {PET_SOUND_PLAY_SMALL, 0, 0, false, true},
};

static const pet_cue_t _pet_cues_play_big[] = {
    {PET_SOUND_PLAY_BIG, 0, 0, false, true},
};

static const pet_cue_t _pet_cues_wake[] = {
    {PET_SOUND_WAKE, 7, SEG_A | SEG_B | SEG_C | SEG_D, false, true},
};

static const pet_cue_t _pet_cues_resurrect[] = {
    {PET_SOUND_RESURRECT_FADE, 0, 0, false, true},
    {PET_SOUND_RESURRECT_RISE, 6, SEG_A, false, true},
};

// ---------
// CUE SHEET
// ---------

static const pet_anim_t _pet_anims[PET_ANIM_COUNT] = {
    //                        frames and count                      loop   layer                cues and count
    [PET_ANIM_NONE] = {PET_NO_FRAMES, false, PET_LAYER_CHARACTER, PET_NO_CUES},
    [PET_ANIM_HAPPY] = {PET_FRAMES(_pet_frames_happy), true, PET_LAYER_CHARACTER, PET_NO_CUES},
    [PET_ANIM_CONFUSED] = {PET_FRAMES(_pet_frames_confused), true, PET_LAYER_CHARACTER, PET_NO_CUES},
    [PET_ANIM_UPSET] = {PET_FRAMES(_pet_frames_upset), true, PET_LAYER_CHARACTER, PET_NO_CUES},
    [PET_ANIM_ANGRY] = {PET_FRAMES(_pet_frames_angry), true, PET_LAYER_CHARACTER, PET_NO_CUES},
    [PET_ANIM_DEAD] = {PET_FRAMES(_pet_frames_dead), true, PET_LAYER_CHARACTER, PET_NO_CUES},
    [PET_ANIM_RESURRECT] = {PET_FRAMES(_pet_frames_resurrect), false, PET_LAYER_CHARACTER, PET_CUES(_pet_cues_resurrect)},
    [PET_ANIM_PLAY_SMALL] = {PET_FRAMES(_pet_frames_play_small), false, PET_LAYER_CHARACTER, PET_CUES(_pet_cues_play_small)},
    [PET_ANIM_PLAY_BIG] = {PET_FRAMES(_pet_frames_play_big), false, PET_LAYER_CHARACTER, PET_CUES(_pet_cues_play_big)},
    [PET_ANIM_EAT] = {PET_FRAMES(_pet_frames_eat), false, PET_LAYER_CHARACTER, PET_CUES(_pet_cues_eat)},
    [PET_ANIM_KISS] = {PET_FRAMES(_pet_frames_kiss), false, PET_LAYER_CHARACTER, PET_CUES(_pet_cues_kiss)},
    [PET_ANIM_SNORE] = {PET_FRAMES(_pet_frames_snore), true, PET_LAYER_CHARACTER, PET_CUES(_pet_cues_snore)},
    [PET_ANIM_WAKE] = {PET_FRAMES(_pet_frames_wake), false, PET_LAYER_CHARACTER, PET_CUES(_pet_cues_wake)},
    [PET_ANIM_POO] = {PET_FRAMES(_pet_frames_poo), false, PET_LAYER_CHARACTER, PET_CUES(_pet_cues_poo)},
    [PET_ANIM_BARF] = {PET_FRAMES(_pet_frames_barf), false, PET_LAYER_CHARACTER, PET_CUES(_pet_cues_barf)},
    [PET_ANIM_PILE] = {PET_FRAMES(_pet_frames_pile), true, PET_LAYER_STATUS, PET_NO_CUES},
    [PET_ANIM_PUDDLE] = {PET_FRAMES(_pet_frames_puddle), true, PET_LAYER_STATUS, PET_NO_CUES},
};

// ------
// SOUNDS
// ------

static int8_t _pet_sound_snore_in[] = {BUZZER_NOTE_C6, 32, 0};
static int8_t _pet_sound_snore_out[] = {BUZZER_NOTE_C5, 32, 0};

static int8_t _pet_sound_kiss[] = {
    BUZZER_NOTE_D6, 2,
    BUZZER_NOTE_D6SHARP_E6FLAT, 2,
    BUZZER_NOTE_E6, 2,
    BUZZER_NOTE_F6, 2,
    0};

static int8_t _pet_sound_eat_gulp[] = {BUZZER_NOTE_E6, 6, 0};
static int8_t _pet_sound_eat_chew[] = {BUZZER_NOTE_G5, 6, 0};

static int8_t _pet_sound_barf_uhoh[] = {
    BUZZER_NOTE_A5, 24,
    BUZZER_NOTE_F5, 24,
    0};

static int8_t _pet_sound_barf_slide[] = {
    BUZZER_NOTE_C6, 6,
    BUZZER_NOTE_B5, 6,
    BUZZER_NOTE_A5SHARP_B5FLAT, 6,
    BUZZER_NOTE_A5, 6,
    BUZZER_NOTE_G5SHARP_A5FLAT, 6,
    BUZZER_NOTE_G5, 6,
    BUZZER_NOTE_F5SHARP_G5FLAT, 6,
    BUZZER_NOTE_F5, 6,
    BUZZER_NOTE_E5, 6,
    BUZZER_NOTE_D5SHARP_E5FLAT, 6,
    BUZZER_NOTE_D5, 6,
    BUZZER_NOTE_C5SHARP_D5FLAT, 6,
    0};

static int8_t _pet_sound_poo[] = {BUZZER_NOTE_C4, 5, 0};

static int8_t _pet_sound_play_small[] = {
    BUZZER_NOTE_C5, 5, BUZZER_NOTE_E5, 5, BUZZER_NOTE_G5, 5, BUZZER_NOTE_C6, 5,
    BUZZER_NOTE_G5, 5, BUZZER_NOTE_E5, 5, BUZZER_NOTE_C5, 5,
    0};

static int8_t _pet_sound_play_big[] = {
    BUZZER_NOTE_D5, 5, BUZZER_NOTE_F5SHARP_G5FLAT, 5, BUZZER_NOTE_A5, 5, BUZZER_NOTE_D6, 5,
    BUZZER_NOTE_A5, 5, BUZZER_NOTE_F5SHARP_G5FLAT, 5, BUZZER_NOTE_D5, 5,
    0};

static int8_t _pet_sound_wake[] = {
    BUZZER_NOTE_E5, 12,
    BUZZER_NOTE_G5, 12,
    BUZZER_NOTE_C6, 16,
    BUZZER_NOTE_A5, 20,
    0};

static int8_t _pet_sound_resurrect_fade[] = {
    BUZZER_NOTE_G4, 12,
    BUZZER_NOTE_C4, 16,
    0};

static int8_t _pet_sound_resurrect_rise[] = {
    BUZZER_NOTE_C5, 5,
    BUZZER_NOTE_E5, 5,
    BUZZER_NOTE_G5, 5,
    BUZZER_NOTE_C6, 5,
    BUZZER_NOTE_E6, 5,
    BUZZER_NOTE_G6, 14,
    0};

static int8_t _pet_sound_feed[] = {BUZZER_NOTE_D6, 3, 0};

static int8_t _pet_sound_sweep[] = {
    BUZZER_NOTE_G5, 2,
    BUZZER_NOTE_E5, 2,
    BUZZER_NOTE_C5, 3,
    0};

static int8_t _pet_sound_grumble[] = {
    BUZZER_NOTE_E4, 10,
    BUZZER_NOTE_C4, 14,
    0};

static int8_t _pet_sound_death[] = {
    BUZZER_NOTE_C5, 16,
    BUZZER_NOTE_A4, 16,
    BUZZER_NOTE_F4, 16,
    BUZZER_NOTE_D4, 28,
    0};

static int8_t _pet_sound_mood_up[] = {BUZZER_NOTE_G5, 4, BUZZER_NOTE_C6, 7, 0};
static int8_t _pet_sound_mood_down[] = {BUZZER_NOTE_C6, 4, BUZZER_NOTE_G5, 7, 0};

static int8_t *_pet_sounds[PET_SOUND_COUNT] = {
    [PET_SOUND_SNORE_IN] = _pet_sound_snore_in,
    [PET_SOUND_SNORE_OUT] = _pet_sound_snore_out,
    [PET_SOUND_KISS] = _pet_sound_kiss,
    [PET_SOUND_BARF_UHOH] = _pet_sound_barf_uhoh,
    [PET_SOUND_BARF_SLIDE] = _pet_sound_barf_slide,
    [PET_SOUND_EAT_GULP] = _pet_sound_eat_gulp,
    [PET_SOUND_EAT_CHEW] = _pet_sound_eat_chew,
    [PET_SOUND_POO] = _pet_sound_poo,
    [PET_SOUND_PLAY_SMALL] = _pet_sound_play_small,
    [PET_SOUND_PLAY_BIG] = _pet_sound_play_big,
    [PET_SOUND_WAKE] = _pet_sound_wake,
    [PET_SOUND_RESURRECT_FADE] = _pet_sound_resurrect_fade,
    [PET_SOUND_RESURRECT_RISE] = _pet_sound_resurrect_rise,
    [PET_SOUND_FEED] = _pet_sound_feed,
    [PET_SOUND_SWEEP] = _pet_sound_sweep,
    [PET_SOUND_GRUMBLE] = _pet_sound_grumble,
    [PET_SOUND_DEATH] = _pet_sound_death,
    [PET_SOUND_MOOD_UP] = _pet_sound_mood_up,
    [PET_SOUND_MOOD_DOWN] = _pet_sound_mood_down,
};

// ----------------
// VISUAL SOUND CUE
// ----------------

static void _pet_play_sound(pet_state_t *s, pet_sound_id_t id)
{
    if (movement_button_should_sound())
    {
        movement_play_sequence(_pet_sounds[id], BUZZER_PRIORITY_BUTTON);
    }
    s->signal_ticks = PET_FLASH_TICKS;
}

// -------
// HELPERS
// -------

static uint32_t _pet_now(void)
{
    return movement_get_utc_timestamp();
}

static pet_daypart_t _pet_daypart(uint8_t hour)
{
    if (hour >= PET_HOUR_SLEEP || hour < PET_HOUR_WAKE)
        return PET_DAYPART_NIGHT;
    if (hour < PET_HOUR_AFTERNOON)
        return PET_DAYPART_MORNING;
    return PET_DAYPART_AFTERNOON;
}

static uint8_t _pet_meal_segment(uint8_t hour)
{
    if (hour < PET_HOUR_WAKE)
        return 0;
    uint8_t seg = (uint8_t)((hour - PET_HOUR_WAKE) / PET_FEED_SEGMENT_HOURS);
    return seg < PET_FEED_SEGMENTS ? seg : PET_FEED_SEGMENTS - 1;
}

static uint16_t _pet_sitting_now(void)
{
    watch_date_time_t local = movement_get_local_date_time();
    return (uint16_t)((local.unit.day << 8) | _pet_meal_segment(local.unit.hour));
}

static pet_mood_t _pet_mood(const pet_state_t *s)
{
    if (s->tics >= PET_TIC_DEAD)
        return PET_MOOD_DEAD;
    if (s->tics >= PET_TIC_ANGRY)
        return PET_MOOD_ANGRY;
    if (s->tics >= PET_TIC_UPSET)
        return PET_MOOD_UPSET;
    if (s->tics >= PET_TIC_CONFUSED)
        return PET_MOOD_CONFUSED;
    return PET_MOOD_HAPPY;
}

static pet_anim_id_t _pet_mood_anim(pet_mood_t mood)
{
    switch (mood)
    {
    case PET_MOOD_CONFUSED:
        return PET_ANIM_CONFUSED;
    case PET_MOOD_UPSET:
        return PET_ANIM_UPSET;
    case PET_MOOD_ANGRY:
        return PET_ANIM_ANGRY;
    case PET_MOOD_DEAD:
        return PET_ANIM_DEAD;
    default:
        return PET_ANIM_HAPPY;
    }
}

static void _pet_set_tap_detection(pet_state_t *s, bool want)
{
    if (want == s->tap_enabled)
        return;
    if (want)
    {
        if (movement_enable_tap_detection_if_available(false))
        {
            lis2dw_configure_tap_threshold(0, 0, PET_SHAKE_THRESHOLD,
                                           LIS2DW_REG_TAP_THS_Z_Z_AXIS_ENABLE);
        }
    }
    else
    {
        movement_disable_tap_detection_if_available();
    }
    s->tap_enabled = want;
    s->shake_taps = s->shake_ticks = s->shake_gap = 0;
}

static void _pet_add_tics(pet_state_t *s, int16_t delta)
{
    int16_t v = (int16_t)s->tics + delta;
    if (v < 0)
        v = 0;
    if (v > PET_TIC_DEAD)
        v = PET_TIC_DEAD;
    s->tics = (uint8_t)v;
}

static uint32_t _pet_local_ts(uint32_t utc_ts)
{
    int32_t offset = movement_get_current_timezone_offset();
    if (offset < 0)
    {
        uint32_t behind = (uint32_t)-offset;
        return (utc_ts > behind) ? utc_ts - behind : 0;
    }
    return utc_ts + (uint32_t)offset;
}

static uint32_t _pet_awake_seconds(uint32_t local_ts)
{
    uint32_t days = local_ts / 86400;
    uint32_t rem = local_ts % 86400;
    uint32_t partial;

    if (rem < (uint32_t)PET_HOUR_WAKE * 3600)
    {
        partial = 0; // still in last night
    }
    else if (rem < (uint32_t)PET_HOUR_SLEEP * 3600)
    {
        partial = rem - (uint32_t)PET_HOUR_WAKE * 3600;
    }
    else
    {
        partial = PET_AWAKE_SECONDS_PER_DAY; // already gone to bed
    }
    return days * (uint32_t)PET_AWAKE_SECONDS_PER_DAY + partial;
}

static uint32_t _pet_awake_between(uint32_t from_ts, uint32_t to_ts)
{
    if (to_ts <= from_ts)
        return 0;
    return _pet_awake_seconds(_pet_local_ts(to_ts)) - _pet_awake_seconds(_pet_local_ts(from_ts));
}

// ----------
// COMPOSITOR
// ----------

static void _pet_draw_position(const digit_mapping_t *maps, uint8_t position, uint8_t mask)
{
    const digit_mapping_t *map = &maps[position];

    for (uint8_t i = 0; i < 8; i++)
    {
        if (map->segment[i].value == segment_does_not_exist)
            continue;
        watch_clear_pixel(map->segment[i].address.com, map->segment[i].address.seg);
    }
    for (uint8_t i = 0; i < 8; i++)
    {
        if (!(mask & (1 << i)))
            continue;
        if (map->segment[i].value == segment_does_not_exist)
            continue;
        watch_set_pixel(map->segment[i].address.com, map->segment[i].address.seg);
    }
}

static void _pet_draw_flags(uint8_t flags)
{
    if (flags & PET_FRAME_COLON)
        watch_set_colon();
    else
        watch_clear_colon();
    if (flags & PET_FRAME_SIGNAL)
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    else
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    if (flags & PET_FRAME_BELL)
        watch_set_indicator(WATCH_INDICATOR_BELL);
    else
        watch_clear_indicator(WATCH_INDICATOR_BELL);
}

static const digit_mapping_t *_pet_display_mapping(void)
{
    // UNKNOWN (an autodetect build sitting on USB) falls back to classic, which is
    // what watch_display_character() does with the same question.
    return watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM ? Custom_LCD_Display_Mapping
                                                         : Classic_LCD_Display_Mapping;
}

static void _pet_draw(pet_state_t *s)
{
    uint8_t fb[10] = {0};
    uint8_t flags = 0;

    for (uint8_t l = 0; l < PET_LAYER_COUNT; l++)
    {
        if (s->layer[l].anim == PET_ANIM_NONE)
            continue;
        const pet_anim_t *a = &_pet_anims[s->layer[l].anim];
        const pet_frame_t *f = &a->frames[s->layer[l].frame];
        const pet_layer_def_t *d = &_pet_layers[l];
        for (uint8_t p = 0; p < 10; p++)
            fb[p] |= f->seg[p] & d->seg[p];
        flags |= f->flags & d->flags;
    }

    for (uint8_t i = 0; i < s->food_queue && i < PET_FOOD_MAX; i++)
    {
        fb[PET_FOOD_POSITION] |= _pet_food_pips[i];
    }

    if (s->buff_ticks)
        fb[PET_BUFF_POSITION] |= SEG_G | SEG_H;
    if (s->debuff_ticks)
        fb[PET_BUFF_POSITION] |= SEG_H;
    if (s->bell_ticks)
        flags |= PET_FRAME_BELL;
    if (s->signal_ticks)
        flags |= PET_FRAME_SIGNAL;

    const digit_mapping_t *maps = _pet_display_mapping();

    for (uint8_t p = 0; p < 10; p++)
    {
        if (fb[p] == s->shadow[p] && !s->shadow_stale)
            continue;
        _pet_draw_position(maps, p, fb[p]);
        s->shadow[p] = fb[p];
    }
    if (flags != s->shadow_flags || s->shadow_stale)
    {
        _pet_draw_flags(flags);
        s->shadow_flags = flags;
    }
    s->shadow_stale = false;
}

static void _pet_invalidate(pet_state_t *s)
{
    s->shadow_stale = true;
}

// ------------
// LAYER ENGINE
// ------------

static void _pet_rest(pet_state_t *s);
#if PET_SHOWCASE
static void _pet_showcase_advance(pet_state_t *s);
#endif

static uint8_t _pet_frame_hold(const pet_anim_t *a, uint8_t frame)
{
    return a->frames ? a->frames[frame].hold : PET_ANIM_HZ;
}

static bool _pet_anim_busy(const pet_state_t *s)
{
    uint8_t id = s->layer[PET_LAYER_CHARACTER].anim;
    return id != PET_ANIM_NONE && !_pet_anims[id].loop;
}

static bool _pet_cue_audible(const pet_state_t *s, pet_sound_id_t id)
{
    if (id == PET_SOUND_SNORE_IN || id == PET_SOUND_SNORE_OUT)
    {
        return s->breath < PET_SNORE_AUDIBLE_BREATHS;
    }
    if (id == PET_SOUND_WAKE)
        return s->scene != PET_SCENE_NIGHT_AWAKE;
    return true;
}

static void _pet_fire_cues(pet_state_t *s, pet_layer_t *L, const pet_anim_t *a,
                           const uint8_t *prev, const uint8_t *cur, bool begun)
{
    for (uint8_t i = 0; i < a->cue_count && i < 8; i++)
    {
        const pet_cue_t *c = &a->cues[i];
        bool fire;
        if (c->mask == 0)
        {
            fire = begun;
        }
        else if (c->on_clear)
        {
            fire = (prev[c->position] & c->mask) && !(cur[c->position] & c->mask);
        }
        else
        {
            fire = !(prev[c->position] & c->mask) && (cur[c->position] & c->mask);
        }
        if (!fire)
            continue;
        if (c->once)
        {
            if (L->cues_fired & (1 << i))
                continue;
            L->cues_fired |= (uint8_t)(1 << i);
        }
        if (_pet_cue_audible(s, (pet_sound_id_t)c->sound))
        {
            _pet_play_sound(s, (pet_sound_id_t)c->sound);
        }
    }
}

static const uint8_t _pet_no_segments[10] = {0};

static const uint8_t *_pet_frame_segs(const pet_anim_t *a, uint8_t frame)
{
    return a->frames ? a->frames[frame].seg : _pet_no_segments;
}

static void _pet_layer_play(pet_state_t *s, pet_layer_id_t l, pet_anim_id_t id)
{
    pet_layer_t *L = &s->layer[l];
    const pet_anim_t *a = &_pet_anims[id];
    L->anim = (uint8_t)id;
    L->frame = 0;
    L->hold_left = _pet_frame_hold(a, 0);
    L->cues_fired = 0;
    _pet_fire_cues(s, L, a, _pet_no_segments, _pet_frame_segs(a, 0), true);
}

static void _pet_start_anim(pet_state_t *s, pet_anim_id_t id)
{
    _pet_layer_play(s, _pet_anims[id].layer, id);
    _pet_draw(s);
}

static void _pet_queue_anim(pet_state_t *s, pet_anim_id_t id)
{
    if (s->queue_len < PET_QUEUE_LEN)
        s->queue[s->queue_len++] = id;
}

static bool _pet_start_next_queued(pet_state_t *s)
{
    if (s->queue_len == 0)
        return false;
    pet_anim_id_t id = s->queue[0];
    s->queue_len--;
    memmove(&s->queue[0], &s->queue[1], s->queue_len * sizeof(s->queue[0]));
    _pet_start_anim(s, id);
    return true;
}

static void _pet_set_status(pet_state_t *s)
{
    pet_anim_id_t want = PET_ANIM_NONE;
    if (_pet_mood(s) != PET_MOOD_DEAD)
    {
        if (s->has_poo && !s->poo_pending)
            want = PET_ANIM_PILE;
        else if (s->has_barf)
            want = PET_ANIM_PUDDLE;
    }
    pet_layer_t *L = &s->layer[PET_LAYER_STATUS];
    L->idle = (uint8_t)want;
    _pet_layer_play(s, PET_LAYER_STATUS, want);
}

static void _pet_layer_tick(pet_state_t *s, pet_layer_id_t l)
{
    pet_layer_t *L = &s->layer[l];
    if (L->anim == PET_ANIM_NONE)
        return;

    const pet_anim_t *a = &_pet_anims[L->anim];
    uint8_t count = a->count;
    bool loop_here = a->loop;
    bool queued = (l == PET_LAYER_CHARACTER) && (s->queue_len > 0);

#if PET_SHOWCASE
    bool reeling = s->showcase_on && l == PET_LAYER_CHARACTER;
    if (reeling)
    {
        loop_here = true;
        queued = false;
    }
#endif

    if (L->hold_left > 1)
    {
        L->hold_left--;
        return;
    }

    uint8_t leaving = L->frame;
    L->frame++;
    if (L->frame < count)
    {
        L->hold_left = _pet_frame_hold(a, L->frame);
        _pet_fire_cues(s, L, a, _pet_frame_segs(a, leaving), _pet_frame_segs(a, L->frame), false);
        _pet_draw(s);
        return;
    }

#if PET_SHOWCASE
    if (reeling && ++s->showcase_plays >= PET_SHOWCASE_PLAYS)
    {
        _pet_showcase_advance(s);
        return;
    }
#endif
    if (loop_here && !queued)
    {
        L->frame = 0;
        L->hold_left = _pet_frame_hold(a, 0);
        if (L->anim == PET_ANIM_SNORE)
        {
            s->breath = (uint8_t)((s->breath + 1) % PET_SNORE_BREATH_CYCLE);
        }
        L->cues_fired = 0; // a fresh turn of the loop cues afresh
        _pet_fire_cues(s, L, a, _pet_frame_segs(a, leaving), _pet_frame_segs(a, 0), true);
        _pet_draw(s);
        return;
    }
    if (queued)
    {
        _pet_start_next_queued(s);
        return;
    }
    if (l == PET_LAYER_CHARACTER)
    {
        _pet_rest(s);
        return;
    }
    _pet_layer_play(s, l, (pet_anim_id_t)L->idle);
    _pet_draw(s);
}

static void _pet_anim_tick(pet_state_t *s)
{
    for (uint8_t l = 0; l < PET_LAYER_COUNT; l++)
        _pet_layer_tick(s, (pet_layer_id_t)l);
}

static void _pet_flash_tick(pet_state_t *s)
{
    bool expired = false;
    if (s->buff_ticks && --s->buff_ticks == 0)
        expired = true;
    if (s->debuff_ticks && --s->debuff_ticks == 0)
        expired = true;
    if (s->bell_ticks && --s->bell_ticks == 0)
        expired = true;
    if (s->signal_ticks && --s->signal_ticks == 0)
        expired = true;
    if (expired)
        _pet_draw(s);
}

static void _pet_flash_buff(pet_state_t *s, bool gained)
{
    s->buff_ticks = gained ? PET_FLASH_TICKS : 0;
    s->debuff_ticks = gained ? 0 : PET_FLASH_TICKS;
}

static void _pet_rest(pet_state_t *s)
{
    pet_mood_t mood = _pet_mood(s);
    s->queue_len = 0;
    _pet_set_status(s);
    _pet_set_tap_detection(s, mood != PET_MOOD_DEAD);

    if (mood == PET_MOOD_DEAD)
    {
        if (s->scene != PET_SCENE_DEAD)
            _pet_play_sound(s, PET_SOUND_DEATH);
        s->shown_mood = (uint8_t)mood;
        s->scene = PET_SCENE_DEAD;
        _pet_start_anim(s, PET_ANIM_DEAD);
        return;
    }

    switch (s->scene)
    {
    case PET_SCENE_ASLEEP:
        _pet_start_anim(s, PET_ANIM_SNORE);
        return;
    case PET_SCENE_FEEDING:
    case PET_SCENE_PLAYING:
    case PET_SCENE_NIGHT_AWAKE:
        break;
    default:
        s->scene = PET_SCENE_IDLE;
        break;
    }

    if ((pet_mood_t)s->shown_mood != mood)
    {
        _pet_play_sound(s, mood < (pet_mood_t)s->shown_mood ? PET_SOUND_MOOD_UP
                                                            : PET_SOUND_MOOD_DOWN);
        s->shown_mood = (uint8_t)mood;
    }
    _pet_start_anim(s, _pet_mood_anim(mood));
}

#if PET_SHOWCASE

static const uint8_t _pet_showcase_reel[] = {
    PET_ANIM_RESURRECT,
    PET_ANIM_SNORE,
    PET_ANIM_WAKE,
    PET_ANIM_EAT,
    PET_ANIM_KISS,
    PET_ANIM_PLAY_SMALL,
    PET_ANIM_PLAY_BIG,
    PET_ANIM_BARF,
    PET_ANIM_POO,
    PET_ANIM_HAPPY,
    PET_ANIM_CONFUSED,
    PET_ANIM_UPSET,
    PET_ANIM_ANGRY,
};
#define PET_SHOWCASE_REEL_LEN ((uint8_t)(sizeof _pet_showcase_reel))

static void _pet_showcase_play(pet_state_t *s)
{
    _pet_set_tap_detection(s, false);
    s->showcase_on = true;
    s->showcase_plays = 0;
    s->queue_len = 0;
    _pet_layer_play(s, PET_LAYER_CHARACTER, PET_ANIM_NONE);
    _pet_layer_play(s, PET_LAYER_STATUS, PET_ANIM_NONE);
    s->breath = 0;
    _pet_start_anim(s, (pet_anim_id_t)_pet_showcase_reel[s->showcase_step]);
}

static void _pet_showcase_advance(pet_state_t *s)
{
    s->showcase_step = (uint8_t)((s->showcase_step + 1) % PET_SHOWCASE_REEL_LEN);
    _pet_showcase_play(s);
}

static void _pet_showcase_exit(pet_state_t *s)
{
    if (!s->showcase_on)
        return;
    s->showcase_on = false;
    _pet_rest(s);
}

static void _pet_showcase_toggle(pet_state_t *s)
{
    if (s->showcase_on)
    {
        _pet_showcase_exit(s);
        return;
    }
    s->showcase_step = 0;
    _pet_showcase_play(s);
}
#endif

// ----------
// SIMULATION
// ----------

static void _pet_charge_decay(pet_state_t *s, uint32_t *since, uint16_t *residual,
                              uint32_t now, uint32_t seconds_per_tic)
{
    uint32_t awake = _pet_awake_between(*since, now) + *residual;
    uint32_t tics = awake / seconds_per_tic;
    *residual = (uint16_t)(awake % seconds_per_tic);
    *since = now;
    if (tics > PET_TIC_DEAD)
        tics = PET_TIC_DEAD;
    if (tics > 0)
        _pet_add_tics(s, (int16_t)tics);
}

static bool _pet_poo_arrives(pet_state_t *s, uint32_t now)
{
    if (s->tics >= PET_TIC_DEAD)
        return false; // a grave does not digest
    if (s->has_poo || s->poo_due_ts == 0 || now < s->poo_due_ts)
        return false;
    s->has_poo = true;
    s->poo_pending = true;
    s->poo_since_ts = s->poo_due_ts;
    s->poo_residual = 0;
    s->poo_due_ts = 0;
    return true;
}

static void _pet_catch_up(pet_state_t *s)
{
    uint32_t now = _pet_now();
    watch_date_time_t local = movement_get_local_date_time();

    if (now < s->last_update_ts)
        s->last_update_ts = now;
    if (now < s->last_fed_ts)
        s->last_fed_ts = now;
    if (now < s->poo_since_ts)
        s->poo_since_ts = now;
    if (now < s->last_play_buff_ts)
        s->last_play_buff_ts = 0;

    if (s->tics < PET_TIC_DEAD)
    {
        _pet_charge_decay(s, &s->last_update_ts, &s->awake_residual,
                          now, PET_SECONDS_PER_TIC);

        uint32_t days = (now - s->last_fed_ts) / PET_MISSED_FEED_SECONDS;
        if (days > 0)
        {
            s->last_fed_ts += days * PET_MISSED_FEED_SECONDS;
            if (days > 6)
                days = 6;
            _pet_add_tics(s, (int16_t)(PET_MISSED_FEED_TICS * days));
        }

        _pet_poo_arrives(s, now);

        if (s->has_poo)
        {
            _pet_charge_decay(s, &s->poo_since_ts, &s->poo_residual,
                              now, PET_POO_SECONDS_PER_TIC);
        }
    }

    // The hug cap resets with the local calendar day.
    if (s->hug_day != local.unit.day)
    {
        s->hug_day = local.unit.day;
        s->hugs_today = 0;
    }
}

// ------------
// INTERACTIONS
// ------------

static void _pet_disturb(pet_state_t *s)
{
    _pet_add_tics(s, PET_DEBUFF_DISTURB);
    _pet_flash_buff(s, false);
    s->night_awake_ticks = PET_NIGHT_AWAKE_SECONDS * PET_ANIM_HZ;
    bool was_asleep = s->scene == PET_SCENE_ASLEEP;
    s->scene = PET_SCENE_NIGHT_AWAKE;
    _pet_play_sound(s, PET_SOUND_GRUMBLE);
    if (was_asleep)
    {
        _pet_start_anim(s, PET_ANIM_WAKE);
    }
    else
    {
        _pet_draw(s);
    }
}

static void _pet_fall_asleep(pet_state_t *s)
{
    s->scene = PET_SCENE_ASLEEP;
    s->breath = 0;
    _pet_rest(s);
}

static bool _pet_blocked(pet_state_t *s)
{
    switch (s->scene)
    {
    case PET_SCENE_DEAD:
        return true;
    case PET_SCENE_ASLEEP:
    case PET_SCENE_NIGHT_AWAKE:
        _pet_disturb(s);
        return true;
    default:
        return false;
    }
}

static void _pet_barf_scene(pet_state_t *s)
{
    _pet_flash_buff(s, false);
    s->barf_until_ts = _pet_now() + PET_BARF_SETTLE_SECONDS;
    s->barf_sitting = _pet_sitting_now();
    s->food_queue = 0;
    s->feed_ticks = 0;
    s->has_barf = true;
    _pet_start_anim(s, PET_ANIM_BARF);
}

static bool _pet_settling(pet_state_t *s)
{
    if (s->barf_until_ts == 0)
        return false;
    if (s->barf_sitting != _pet_sitting_now())
        return false;
    return _pet_now() < s->barf_until_ts;
}

static void _pet_feed_barf(pet_state_t *s)
{
    _pet_add_tics(s, (int16_t)(s->seg_buff_tics + PET_DEBUFF_BARF));
    s->seg_buff_tics = 0;
    _pet_barf_scene(s);
}

static void _pet_feed_press(pet_state_t *s)
{
    if (_pet_blocked(s))
        return;
    if (_pet_settling(s))
    {
        _pet_play_sound(s, PET_SOUND_GRUMBLE);
        _pet_draw(s);
        return;
    }
    if (s->food_queue < PET_FOOD_MAX)
        s->food_queue++;
    s->feed_ticks = PET_FEED_SETTLE_SECONDS * PET_ANIM_HZ;
    s->scene = PET_SCENE_FEEDING;
    s->bell_ticks = PET_BELL_TICKS;
    _pet_play_sound(s, PET_SOUND_FEED);
    _pet_draw(s);
}

static void _pet_feed_tick(pet_state_t *s)
{
    if (_pet_anim_busy(s))
        return;
    if (s->feed_ticks > 0)
    {
        s->feed_ticks--;
        return;
    }
    if (s->food_queue == 0)
    {
        s->scene = PET_SCENE_IDLE;
        _pet_rest(s);
        return;
    }

    uint32_t now = _pet_now();

    uint16_t sitting = _pet_sitting_now();
    if (s->fed_sitting != sitting)
    {
        s->fed_sitting = sitting;
        s->pips_this_seg = 0;
        s->seg_buff_tics = 0;
    }
    s->food_queue--;
    s->last_fed_ts = now;

    if (s->pips_this_seg >= PET_FEED_SEGMENT_CAP)
    {
        _pet_feed_barf(s);
        return;
    }

    s->pips_this_seg++;
    s->seg_buff_tics = (uint8_t)(s->seg_buff_tics + PET_BUFF_EAT);
    _pet_add_tics(s, -PET_BUFF_EAT);
    if (!s->has_poo && s->poo_due_ts == 0)
    {
        s->poo_due_ts = now + PET_POO_DELAY_SECONDS;
    }
    _pet_flash_buff(s, true);
    _pet_start_anim(s, PET_ANIM_EAT);
    s->feed_ticks = PET_FEED_PIP_SECONDS * PET_ANIM_HZ;
}

static void _pet_hug(pet_state_t *s)
{
    if (_pet_blocked(s))
        return;
    if (s->hugs_today < PET_HUG_CAP)
    {
        s->hugs_today++;
        _pet_add_tics(s, -PET_BUFF_HUG);
        _pet_flash_buff(s, true);
    }
    _pet_start_anim(s, PET_ANIM_KISS);
}

static void _pet_chord(pet_state_t *s)
{
    s->chord_hugged = true;
#if PET_SHOWCASE
    _pet_showcase_exit(s); // the kiss needs the screen
#endif
    _pet_hug(s);
}

static bool _pet_chord_release(pet_state_t *s)
{
    if (!s->chord_hugged)
        return false;
    if (!s->light_down && !s->alarm_down)
        s->chord_hugged = false;
    return true;
}

static void _pet_sweep(pet_state_t *s)
{
    if (s->scene == PET_SCENE_DEAD)
        return;
    bool had_something = s->has_poo || s->has_barf;
    s->has_poo = false;
    s->has_barf = false;
    s->poo_residual = 0;
    s->poo_pending = false;
    _pet_set_status(s);
    if (had_something)
        _pet_play_sound(s, PET_SOUND_SWEEP);
    if (had_something && s->scene != PET_SCENE_ASLEEP)
    {
        _pet_start_anim(s, _pet_mood_anim(_pet_mood(s)));
    }
    else
    {
        _pet_draw(s);
    }
}

static void _pet_resurrect(pet_state_t *s)
{
    if (s->scene != PET_SCENE_DEAD)
        return;
    uint32_t now = _pet_now();
    s->tics = 0;
    s->shown_mood = (uint8_t)PET_MOOD_HAPPY;
    s->last_update_ts = now;
    s->last_fed_ts = now;
    s->awake_residual = 0;
    s->has_poo = false;
    s->has_barf = false;
    s->poo_due_ts = 0;
    s->poo_residual = 0;
    s->poo_pending = false;
    s->hugs_today = 0;
    s->last_play_buff_ts = 0;
    s->pips_this_seg = 0;
    s->seg_buff_tics = 0;
    s->barf_until_ts = 0;
    s->scene = PET_SCENE_IDLE;
    _pet_start_anim(s, PET_ANIM_RESURRECT);
}

#define PET_PLAY_WINDOW_TICKS (PET_PLAY_WINDOW_SECONDS * PET_ANIM_HZ)

static void _pet_barf(pet_state_t *s)
{
    _pet_add_tics(s, (s->play_buffed ? PET_BUFF_PLAY : 0) + PET_DEBUFF_BARF);
    s->play_stage = 0;
    s->play_ticks = 0;
    _pet_barf_scene(s);
}

static void _pet_on_motion(pet_state_t *s)
{
    if (s->scene == PET_SCENE_PLAYING)
    {
        if (s->play_stage == 0 || _pet_anim_busy(s))
            return;
        s->play_stage++;
        if (s->play_stage >= PET_PLAY_STAGE_BARF)
        {
            _pet_barf(s);
        }
        else
        {
            s->play_ticks = PET_PLAY_WINDOW_TICKS;
            _pet_start_anim(s, PET_ANIM_PLAY_BIG);
        }
        return;
    }
    if (_pet_blocked(s))
        return;

    uint32_t now = _pet_now();
    s->scene = PET_SCENE_PLAYING;
    s->play_stage = 1;
    s->play_ticks = PET_PLAY_WINDOW_TICKS;
    s->play_buffed = (now - s->last_play_buff_ts) >= PET_PLAY_COOLDOWN_SECONDS;
    if (s->play_buffed)
    {
        s->last_play_buff_ts = now;
        _pet_add_tics(s, -PET_BUFF_PLAY);
        _pet_flash_buff(s, true);
    }
    _pet_start_anim(s, PET_ANIM_PLAY_SMALL);
}

#define PET_SHAKE_WINDOW_TICKS (PET_SHAKE_WINDOW_SECONDS * PET_ANIM_HZ)

static void _pet_on_tap(pet_state_t *s)
{
    if (s->shake_gap)
        return;
    s->shake_gap = PET_SHAKE_GAP_TICKS;
    if (s->shake_ticks == 0)
    {
        s->shake_taps = 0;
        s->shake_ticks = PET_SHAKE_WINDOW_TICKS;
    }
    if (++s->shake_taps < PET_SHAKE_TAPS)
        return;
    s->shake_taps = 0;
    s->shake_ticks = 0;
    _pet_on_motion(s);
}

static void _pet_shake_tick(pet_state_t *s)
{
    if (s->shake_gap)
        s->shake_gap--;
    if (s->shake_ticks && --s->shake_ticks == 0)
        s->shake_taps = 0;
}

static void _pet_play_tick(pet_state_t *s)
{
    if (_pet_anim_busy(s))
        return;
    if (s->play_ticks > 0)
    {
        s->play_ticks--;
        return;
    }
    s->scene = PET_SCENE_IDLE;
    s->play_stage = 0;
    _pet_rest(s);
}

// -----------
// SCENE ENTRY
// -----------

static void _pet_enter(pet_state_t *s)
{
    _pet_catch_up(s);
    _pet_invalidate(s);

    s->queue_len = 0;
    s->food_queue = 0;
    s->play_stage = 0;
    s->play_buffed = false;
    s->light_down = s->alarm_down = s->chord_hugged = false;
    s->buff_ticks = s->debuff_ticks = s->bell_ticks = s->signal_ticks = 0;
    _pet_layer_play(s, PET_LAYER_CHARACTER, PET_ANIM_NONE);
    _pet_layer_play(s, PET_LAYER_STATUS, PET_ANIM_NONE);
#if PET_SHOWCASE
    s->showcase_on = false;
    s->showcase_step = 0;
#endif
    _pet_set_status(s);

    pet_mood_t mood = _pet_mood(s);
    s->shown_mood = (uint8_t)mood;
    if (mood == PET_MOOD_DEAD)
    {
        _pet_rest(s);
        return;
    }
    _pet_set_tap_detection(s, true);

    watch_date_time_t local = movement_get_local_date_time();
    switch (_pet_daypart(local.unit.hour))
    {
    case PET_DAYPART_MORNING:
        s->scene = PET_SCENE_IDLE;
        if (s->woke_day != local.unit.day)
        {
            s->woke_day = local.unit.day;
            _pet_queue_anim(s, PET_ANIM_WAKE);
        }
        _pet_queue_anim(s, _pet_mood_anim(mood));
        break;
    case PET_DAYPART_AFTERNOON:
        s->scene = PET_SCENE_IDLE;
        break;
    case PET_DAYPART_NIGHT:
        s->scene = PET_SCENE_ASLEEP;
        s->breath = 0;
        break;
    }
    if (!_pet_start_next_queued(s))
        _pet_rest(s);
}

static void _pet_check_daypart(pet_state_t *s)
{
    watch_date_time_t local = movement_get_local_date_time();
    bool night = _pet_daypart(local.unit.hour) == PET_DAYPART_NIGHT;
    if (night && s->scene == PET_SCENE_IDLE)
    {
        _pet_fall_asleep(s);
    }
    else if (!night && (s->scene == PET_SCENE_ASLEEP || s->scene == PET_SCENE_NIGHT_AWAKE))
    {
        s->scene = PET_SCENE_IDLE;
        s->woke_day = local.unit.day;
        _pet_start_anim(s, PET_ANIM_WAKE);
    }
}

static void _pet_tick(pet_state_t *s, uint8_t subsecond)
{
    switch (s->scene)
    {
    case PET_SCENE_FEEDING:
        _pet_feed_tick(s);
        break;
    case PET_SCENE_PLAYING:
        _pet_play_tick(s);
        break;
    case PET_SCENE_NIGHT_AWAKE:
        if (s->night_awake_ticks > 0 && --s->night_awake_ticks == 0)
            _pet_fall_asleep(s);
        break;
    case PET_SCENE_ASLEEP:
        break;
    default:
        break;
    }
    if (subsecond == 0)
    {
        _pet_check_daypart(s);
        _pet_poo_arrives(s, _pet_now());
    }
    if (s->poo_pending && !s->showcase_on)
    {
        if (s->scene == PET_SCENE_IDLE && !_pet_anim_busy(s))
        {
            s->poo_pending = false;
            _pet_start_anim(s, PET_ANIM_POO); // and _pet_rest leaves the pile
        }
        else if (s->scene == PET_SCENE_ASLEEP || s->scene == PET_SCENE_DEAD)
        {
            s->poo_pending = false;
            _pet_set_status(s);
            _pet_draw(s);
        }
    }
    _pet_shake_tick(s);
    _pet_flash_tick(s);
    _pet_anim_tick(s);
}

// ------------------
// MOVEMENT CALLBACKS
// ------------------

void pet_face_setup(uint8_t watch_face_index, void **context_ptr)
{
    (void)watch_face_index;
    if (*context_ptr == NULL)
    {
        *context_ptr = malloc(sizeof(pet_state_t));
        memset(*context_ptr, 0, sizeof(pet_state_t));
        pet_state_t *s = (pet_state_t *)*context_ptr;
        uint32_t now = _pet_now();
        s->last_update_ts = now;
        s->last_fed_ts = now;
    }
}

void pet_face_activate(void *context)
{
    pet_state_t *s = (pet_state_t *)context;
    movement_request_tick_frequency(PET_ANIM_HZ);
    s->tap_enabled = false;
}

bool pet_face_loop(movement_event_t event, void *context)
{
    pet_state_t *s = (pet_state_t *)context;

#if PET_SHOWCASE
    switch (event.event_type)
    {
    case EVENT_LIGHT_BUTTON_UP:
    case EVENT_ALARM_BUTTON_UP:
    case EVENT_ALARM_LONG_PRESS:
        _pet_showcase_exit(s);
        break;
    default:
        break;
    }
#endif

    switch (event.event_type)
    {
    case EVENT_ACTIVATE:
        _pet_enter(s);
        break;
    case EVENT_TICK:
        _pet_tick(s, event.subsecond);
        break;

    case EVENT_LIGHT_BUTTON_DOWN:
        s->light_down = true;
        if (s->alarm_down)
            _pet_chord(s);
        break;
    case EVENT_LIGHT_BUTTON_UP:
        s->light_down = false;
        if (_pet_chord_release(s))
            break;
        _pet_feed_press(s);
        break;
    case EVENT_LIGHT_LONG_PRESS:
        break;
    case EVENT_LIGHT_LONG_UP:
        s->light_down = false;
        _pet_chord_release(s);
        break;
    case EVENT_LIGHT_REALLY_LONG_PRESS:
#if PET_SHOWCASE
        if (!s->chord_hugged)
            _pet_showcase_toggle(s);
#endif
        break;

    case EVENT_ALARM_BUTTON_DOWN:
        s->alarm_down = true;
        if (s->light_down)
            _pet_chord(s);
        break;
    case EVENT_ALARM_BUTTON_UP:
        s->alarm_down = false;
        s->alarm_resurrected = false;
        if (_pet_chord_release(s))
            break;
        _pet_sweep(s);
        break;
    case EVENT_ALARM_LONG_PRESS:
        if (s->chord_hugged)
            break;
        if (s->scene == PET_SCENE_DEAD)
        {
            _pet_resurrect(s);
            // This hold has spent itself; it must not go on to play as well.
            s->alarm_resurrected = true;
        }
        break;
    case EVENT_ALARM_LONG_UP:
        s->alarm_down = false;
        s->alarm_resurrected = false;
        _pet_chord_release(s);
        break;
    case EVENT_ALARM_REALLY_LONG_PRESS:
        // A second way in to the same play. The accelerometer is the gesture
        // this is imitating, and a board without one has only this -- but both
        // land in _pet_on_motion, and the buff's 2 h cooldown is what limits
        // playing either way, so offering both costs the game nothing.
        if (!s->chord_hugged && !s->alarm_resurrected)
            _pet_on_motion(s);
        break;

    case EVENT_SINGLE_TAP:
    case EVENT_DOUBLE_TAP:
    case EVENT_ACCELEROMETER_WAKE:
#if PET_SHOWCASE
        if (s->showcase_on)
            break;
#endif
        _pet_on_tap(s);
        break;

    case EVENT_TIMEOUT:
        movement_move_to_face(0);
        break;
    case EVENT_LOW_ENERGY_UPDATE:
        _pet_invalidate(s);
        _pet_draw(s);
        break;

    default:
        return movement_default_loop_handler(event);
    }

    return true;
}

void pet_face_resign(void *context)
{
    pet_state_t *s = (pet_state_t *)context;
    movement_request_tick_frequency(1);
    _pet_set_tap_detection(s, false);
}
