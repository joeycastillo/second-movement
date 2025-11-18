/*
 * MIT License
 *
 * Copyright (c) 2023 Jeremy O'Brien
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

#include "movement_signal_tunes.h"

#include <assert.h>

#include "watch.h"

static const int8_t signal_tune_default[] = {
    BUZZER_NOTE_C8, 5,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_C8, 5,
    0
};

static const int8_t signal_tune_alarm[] = {
    BUZZER_NOTE_C8, 2,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_C8, 2,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_C8, 2,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_C8, 2,
    0
};

static const int8_t signal_tune_beep[] = {
    BUZZER_NOTE_C8, 32,
    0
};

static const int8_t signal_tune_zelda_secret[] = {
    BUZZER_NOTE_G5, 8,
    BUZZER_NOTE_F5SHARP_G5FLAT, 8,
    BUZZER_NOTE_D5SHARP_E5FLAT, 8,
    BUZZER_NOTE_A4, 8,
    BUZZER_NOTE_G4SHARP_A4FLAT, 8,
    BUZZER_NOTE_E5, 8,
    BUZZER_NOTE_G5SHARP_A5FLAT, 8,
    BUZZER_NOTE_C6, 20,
    0
};

static const int8_t signal_tune_mario_theme[] = {
    BUZZER_NOTE_E6, 7,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_E6, 7,
    BUZZER_NOTE_REST, 10,
    BUZZER_NOTE_E6, 7,
    BUZZER_NOTE_REST, 11,
    BUZZER_NOTE_C6, 7,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 7,
    BUZZER_NOTE_REST, 10,
    BUZZER_NOTE_G6, 8,
    BUZZER_NOTE_REST, 30,
    BUZZER_NOTE_G5, 8,
    0
};

static const int8_t signal_tune_mgs_codec[] = {
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    0
};

static const int8_t signal_tune_kim_possible[] = {
    BUZZER_NOTE_G7, 6,
    BUZZER_NOTE_G4, 2,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_G7, 6,
    BUZZER_NOTE_G4, 2,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_A7SHARP_B7FLAT, 6,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_G7, 6,
    BUZZER_NOTE_G4, 2,
    0
};

static const int8_t signal_tune_power_rangers[] = {
    BUZZER_NOTE_D8, 6,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_D8, 6,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_C8, 6,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_D8, 6,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_F8, 6,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_D8, 6,
    0
};

static const int8_t signal_tune_layla[] = {
    BUZZER_NOTE_A6, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_C7, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F7, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_C7, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_D7, 20,
    0
};

static const int8_t signal_tune_harry_potter_short[] = {
    BUZZER_NOTE_B5, 12,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 12,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G6, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6SHARP_G6FLAT, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 16,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_B6, 8,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_A6, 24,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6SHARP_G6FLAT, 24,
    0
};

static const int8_t signal_tune_harry_potter_long[] = {
    BUZZER_NOTE_B5, 12,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 12,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G6, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6SHARP_G6FLAT, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 16,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_B6, 8,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_A6, 24,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6SHARP_G6FLAT, 24,
    BUZZER_NOTE_REST, 1,

    BUZZER_NOTE_E6, 12,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G6, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6SHARP_G6FLAT, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_D6SHARP_E6FLAT, 16,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6, 8,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_B5, 24,

    0
};

static const int8_t signal_tune_jurassic_park[] = {
    BUZZER_NOTE_B5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_A5SHARP_B5FLAT, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_B5, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_F5SHARP_G5FLAT, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_E5, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_B5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_A5SHARP_B5FLAT, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_B5, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_F5SHARP_G5FLAT, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_E5, 13,
    0,
};

static const int8_t signal_tune_evangelion[] = {
    BUZZER_NOTE_C5, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_D5SHARP_E5FLAT, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_F5, 13,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_D5SHARP_E5FLAT, 13,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_F5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_F5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_F5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_A5SHARP_B5FLAT, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_G5SHARP_A5FLAT, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_G5, 3,
    BUZZER_NOTE_REST, 3,
    BUZZER_NOTE_F5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_G5, 13,
    0,
};

const int8_t * const movement_signal_tunes[] = {
    signal_tune_default,
    signal_tune_alarm,
    signal_tune_beep,
    signal_tune_zelda_secret,
    signal_tune_mario_theme,
    signal_tune_mgs_codec,
    signal_tune_kim_possible,
    signal_tune_power_rangers,
    signal_tune_layla,
    signal_tune_harry_potter_short,
    signal_tune_harry_potter_long,
    signal_tune_jurassic_park,
    signal_tune_evangelion,
    0
};

static_assert(sizeof(movement_signal_tunes) / sizeof(movement_signal_tunes[0]) <= 100,
    "Adding so many tunes requres modification of signal_tunes_face");

movement_tune_params_t movement_selected_signal_tunes[MOVEMENT_NUM_TUNE_MODES] = {
    [MOVEMENT_TUNE_MODE_CHIME] = {
        .tune = signal_tune_default,
        .duration_sec = 0,
        .interval_ticks = 0,
    },
    [MOVEMENT_TUNE_MODE_ALARM] = {
        .tune = signal_tune_alarm,
        .duration_sec = 60,
        .interval_ticks = 32,
    },
    [MOVEMENT_TUNE_MODE_TIMER] = {
        .tune = signal_tune_alarm,
        .duration_sec = 15,
        .interval_ticks = 32,
    }
};

