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

#pragma once

static const int8_t alarm_tune_default[] = {
    BUZZER_NOTE_C8, 3,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_C8, 3,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_C8, 3,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_C8, 5,
    BUZZER_NOTE_REST, 38,
    -8, 9,
    0
};

static const int8_t signal_tune_default[] =
{
    BUZZER_NOTE_C8, 5,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_C8, 5,
    0
};

static const int8_t tune_zelda_secret[] =
{
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

static const int8_t tune_mario_theme[] =
{
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

static const int8_t tune_mgs_codec[] =
{
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

static const int8_t tune_kim_possible[] =
{
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

static const int8_t tune_power_rangers[] =
{
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

static const int8_t tune_layla[] =
{
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

static const int8_t tune_harry_potter_short[] =
{
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

static const int8_t tune_harry_potter_long[] =
{
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

static const int8_t tune_jurassic_park[] =
{
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
    0
};

static const int8_t tune_evangelion[] =
{
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
    0
};

/* -------------------- MISSING ADDED TUNES -------------------- */

static const int8_t tune_mario_1up[] =
{
    BUZZER_NOTE_E6, 4,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_G6, 4,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_E7, 4,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_C7, 4,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_D7, 4,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_G7, 4,
    0
};

static const int8_t tune_mario_pup[] =
{
    BUZZER_NOTE_C5, 2,
    BUZZER_NOTE_C5, 2,
    BUZZER_NOTE_E5, 2,
    BUZZER_NOTE_G5, 2,
    BUZZER_NOTE_C6, 2,
    BUZZER_NOTE_G5, 2,
    BUZZER_NOTE_C6, 2,
    BUZZER_NOTE_E6, 2,
    BUZZER_NOTE_G6, 2,
    BUZZER_NOTE_C7, 2,
    0
};

static const int8_t tune_whatsapp[] =
{
    BUZZER_NOTE_B5, 4,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_F6SHARP_G6FLAT, 4,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_B6, 4,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_G6, 2,
    BUZZER_NOTE_A6, 4,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_F6SHARP_G6FLAT, 4,
    0
};

static const int8_t tune_ff_victory[] =
{
    BUZZER_NOTE_B5, 4,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_B5, 4,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_B5, 4,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_B5, 8,
    BUZZER_NOTE_REST, 19,
    BUZZER_NOTE_G5, 8,
    BUZZER_NOTE_REST, 19,
    BUZZER_NOTE_A5, 8,
    BUZZER_NOTE_REST, 19,
    BUZZER_NOTE_B5, 8,
    0
};

static const int8_t tune_game_boy[] =
{
    BUZZER_NOTE_C6, 4,
    BUZZER_NOTE_C7, 8,
    BUZZER_NOTE_REST, 4,
    0
};

static const int8_t tune_game_boy_pause[] =
{
    BUZZER_NOTE_C6, 4,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_C7, 4,
    BUZZER_NOTE_REST, 5,
    -4, 1,
    0
};

static const int8_t tune_westminster[] =
{
    BUZZER_NOTE_G7SHARP_A7FLAT, 1,
    BUZZER_NOTE_REST, 1,
    -2, 2,
    BUZZER_NOTE_REST, 34,
    BUZZER_NOTE_E7, 1,
    BUZZER_NOTE_REST, 1,
    -2, 2,
    BUZZER_NOTE_REST, 34,
    BUZZER_NOTE_F7SHARP_G7FLAT, 1,
    BUZZER_NOTE_REST, 1,
    -2, 2,
    BUZZER_NOTE_REST, 34,
    BUZZER_NOTE_B6, 1,
    BUZZER_NOTE_REST, 1,
    -2, 2,
    BUZZER_NOTE_REST, 74,
    0
};

static const int8_t tune_ocean[] =
{
    BUZZER_NOTE_B4, 1,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G5, 1,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_B4, 1,
    BUZZER_NOTE_REST, 3,
    BUZZER_NOTE_B4, 1,
    BUZZER_NOTE_G5, 1,
    BUZZER_NOTE_D6, 1,
    0
};

static const int8_t tune_kirby_victory[] =
{
    BUZZER_NOTE_F6, 4,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_G6, 4,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_A6, 4,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_G6, 4,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_B6, 4,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_C7, 4,
    BUZZER_NOTE_REST, 20,
    0
};

static const int8_t tune_third_sanctuary[] =
{
    BUZZER_NOTE_C7, 3,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_G6, 4,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_F6, 4,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_G6, 4,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_C7, 4,
    0
};

static const int8_t tune_minecraft[] =
{
    BUZZER_NOTE_B6, 4,
    BUZZER_NOTE_REST, 68,
    BUZZER_NOTE_A6, 4,
    BUZZER_NOTE_REST, 33,
    BUZZER_NOTE_E6, 4,
    BUZZER_NOTE_REST, 68,
    BUZZER_NOTE_G6, 4,
    0
};

static const int8_t tune_sonic_ring[] =
{
    BUZZER_NOTE_E6, 4,
    BUZZER_NOTE_G6, 4,
    BUZZER_NOTE_C7, 5,
    0
};

static const int8_t tune_neo_geo[] =
{
    BUZZER_NOTE_D5, 4,
    BUZZER_NOTE_F5, 4,
    BUZZER_NOTE_A5, 4,
    BUZZER_NOTE_C6, 4,
    BUZZER_NOTE_B6, 4,
    BUZZER_NOTE_D7, 4,
    BUZZER_NOTE_E7, 4,
    0
};

static const int8_t tune_bosun_whistle[] =
{
    BUZZER_NOTE_B6, 4,
    BUZZER_NOTE_C7, 4,
    BUZZER_NOTE_C7SHARP_D7FLAT, 4,
    BUZZER_NOTE_D7SHARP_E7FLAT, 52,
    BUZZER_NOTE_C7, 2,
    BUZZER_NOTE_B6, 6,
    0
};

static const int8_t tune_among_us[] =
{
    BUZZER_NOTE_C6, 4,
    BUZZER_NOTE_REST, 20,
    BUZZER_NOTE_D6SHARP_E6FLAT, 4,
    BUZZER_NOTE_REST, 20,
    BUZZER_NOTE_F6, 4,
    BUZZER_NOTE_REST, 20,
    BUZZER_NOTE_F6SHARP_G6FLAT, 4,
    BUZZER_NOTE_REST, 20,
    BUZZER_NOTE_F6, 4,
    BUZZER_NOTE_D6SHARP_E6FLAT, 4,
    0
};


static const int8_t tune_gigi_dag[] =
{
    BUZZER_NOTE_F6SHARP_G6FLAT, 5,
    BUZZER_NOTE_REST, ((12 * 2) - 5),
    BUZZER_NOTE_F6SHARP_G6FLAT, 5,
    BUZZER_NOTE_REST, (12 - 5),
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, (12 - 5),
    BUZZER_NOTE_C7SHARP_D7FLAT, 5,
    BUZZER_NOTE_REST, ((2 * 12 * 2) - 5),

    BUZZER_NOTE_C7SHARP_D7FLAT, 5,
    BUZZER_NOTE_REST, ((12 * 2) - 5),
    BUZZER_NOTE_C7SHARP_D7FLAT, 5,
    BUZZER_NOTE_REST, (12 - 5),
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, (12 - 5),
    BUZZER_NOTE_B6, 5,
    BUZZER_NOTE_REST, ((2 * 12 * 2) - 5),

    BUZZER_NOTE_B6, 5,
    BUZZER_NOTE_REST, ((12 * 2) - 5),
    BUZZER_NOTE_B6, 5,
    BUZZER_NOTE_REST, (12 - 5),
    BUZZER_NOTE_A6, 5,
    BUZZER_NOTE_REST, (12 - 5),
    BUZZER_NOTE_B6, 5,
    BUZZER_NOTE_REST, ((12 * 2) - 5),
    BUZZER_NOTE_B6, 5,
    BUZZER_NOTE_REST, ((12 * 2) - 5),
    BUZZER_NOTE_B6, 5,
    BUZZER_NOTE_REST, (12 - 5),
    BUZZER_NOTE_A6, 5,
    BUZZER_NOTE_REST, (12 - 5),
    BUZZER_NOTE_B6, 5,
    BUZZER_NOTE_REST, (12 - 5),
    BUZZER_NOTE_A6, 5,
    BUZZER_NOTE_REST, (12 - 5),
    BUZZER_NOTE_F6SHARP_G6FLAT, 5,
    BUZZER_NOTE_REST, ((2 * 12 * 2) - 5),

    -36, 1,
    0
};

/* -------------------- TABLE -------------------- */

static const int8_t* tunes_table[] =
{
    alarm_tune_default, //rename to tune_alarm_default
    tune_zelda_secret,
    tune_gigi_dag,
    tune_mario_theme,
    tune_mgs_codec,
    tune_kim_possible,
    tune_power_rangers,
    tune_layla,
    tune_harry_potter_short,
    tune_harry_potter_long,
    tune_jurassic_park,
    tune_evangelion,
    tune_mario_1up,
    tune_mario_pup,
    tune_whatsapp,
    tune_ff_victory,
    tune_game_boy,
    tune_game_boy_pause,
    tune_westminster,
    tune_ocean,
    tune_kirby_victory,
    tune_third_sanctuary,
    tune_minecraft,
    tune_sonic_ring,
    tune_neo_geo,
    tune_bosun_whistle,
    tune_among_us,
};

static const int8_t MELODY_CNT = 27;

static int8_t* signal_tune = signal_tune_default;
