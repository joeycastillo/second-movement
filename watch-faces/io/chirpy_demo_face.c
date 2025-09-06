/*
 * MIT License
 *
 * Copyright (c) 2023 Gabor L Ugray
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
#include "chirpy_demo_face.h"
#include "chirpy_tx.h"
#include "filesystem.h"

typedef enum {
    CDM_CHOOSE = 0,
    CDM_CHIRPING,
} chirpy_demo_mode_t;

typedef enum {
    CDP_CLEAR = 0,
    CDP_INFO_SHORT,
    CDP_INFO_LONG,
    CDP_INFO_NANOSEC,
} chirpy_demo_program_t;

typedef struct {
    // Current mode
    chirpy_demo_mode_t mode;

    // Selected program
    chirpy_demo_program_t program;

    // Used by chirpy encoder during transmission
    chirpy_encoder_state_t encoder_state;

} chirpy_demo_state_t;

static uint8_t long_data_str[] =
    "There once was a ship that put to sea\n"
    "The name of the ship was the Billy of Tea\n"
    "The winds blew up, her bow dipped down\n"
    "O blow, my bully boys, blow (huh)\n"
    "\n"
    "Soon may the Wellerman come\n"
    "To bring us sugar and tea and rum\n"
    "One day, when the tonguin' is done\n"
    "We'll take our leave and go\n";

static uint16_t short_data_len = 20;

static uint8_t short_data[] = {
    0x27,
    0x00,
    0x0c,
    0x42,
    0xa3,
    0xd4,
    0x06,
    0x54,
    0x00,
    0x00,
    0x02,
    0x0c,
    0x6b,
    0x05,
    0x5a,
    0x09,
    0xd8,
    0x00,
    0xf5,
    0x00,
};

#define ACTIVITY_DATA_FILE_NAME "activity.dat"

static uint8_t *activity_buffer = 0;
static uint16_t activity_buffer_size = 0;

void chirpy_demo_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(chirpy_demo_state_t));
        memset(*context_ptr, 0, sizeof(chirpy_demo_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

void chirpy_demo_face_activate(void *context) {
    chirpy_demo_state_t *state = (chirpy_demo_state_t *)context;

    memset(context, 0, sizeof(chirpy_demo_state_t));
    state->mode = CDM_CHOOSE;
    state->program = CDP_INFO_NANOSEC;

    // Do we have activity data? Load it.
    int32_t sz = filesystem_get_file_size(ACTIVITY_DATA_FILE_NAME);
    if (sz > 0) {
        // We will free this in resign.
        // I don't like any kind of dynamic allocation in long-running embedded software...
        // But there's no way around it here; I don't want to hard-wire (and squat) any fixed size structure
        // Nanosec data may change in the future too
        activity_buffer_size = sz + 2;
        activity_buffer = malloc(activity_buffer_size);
        // First two bytes of prefix, so Chirpy RX can recognize this data type
        activity_buffer[0] = 0x41;
        activity_buffer[1] = 0x00;
        // Read file
        filesystem_read_file(ACTIVITY_DATA_FILE_NAME, (char*)&activity_buffer[2], sz);
    }
}

// To create / check test file in emulator:
// echo TestData > activity.ini
// cat activity.ini

static void _cdf_update_lcd(chirpy_demo_state_t *state) {
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "CH", "Chirp");
    if (state->program == CDP_CLEAR) {
        movement_force_led_on(255, 0, 0);
        watch_display_text(WATCH_POSITION_BOTTOM, "CLEAR?");
    } else if (state->program == CDP_INFO_SHORT) {
        watch_display_text(WATCH_POSITION_BOTTOM, "SHORT ");
    } else if (state->program == CDP_INFO_LONG) {
        watch_display_text(WATCH_POSITION_BOTTOM, " LOng ");
    } else if (state->program == CDP_INFO_NANOSEC) {
        watch_display_text(WATCH_POSITION_BOTTOM, " ACtIV");
    } else {
        watch_display_text(WATCH_POSITION_BOTTOM, "----  ");
    }
}

static uint8_t *curr_data_ptr;
static uint16_t curr_data_ix;
static uint16_t curr_data_len;
static chirpy_demo_state_t *curr_state;

static uint8_t _cdf_get_next_byte(uint8_t *next_byte) {
    if (curr_data_ix == curr_data_len)
        return 0;
    *next_byte = curr_data_ptr[curr_data_ix];
    ++curr_data_ix;
    return 1;
}

static void _cdf_on_chirping_done(void) {
    if (curr_state) {
        curr_state->mode = CDM_CHOOSE;
    }
    watch_clear_indicator(WATCH_INDICATOR_BELL);
}

static bool _cdm_raw_source_fn(uint16_t position, void* userdata, uint16_t* period, uint16_t* duration) {
    // Beep countdown
    if (position < 6) {
        if (position % 2) {
            *period = WATCH_BUZZER_PERIOD_REST;
            *duration = 56;
        } else {
            *period = NotePeriods[BUZZER_NOTE_A5];
            *duration = 8;
        }
        return false;
    }

    chirpy_demo_state_t *state = (chirpy_demo_state_t *)userdata;

    uint8_t tone = chirpy_get_next_tone(&state->encoder_state);
    // Transmission over?
    if (tone == 255) {
        return true;
    }

    *period = chirpy_get_tone_period(tone);
    *duration = 3;

    return false;
}

static void _cdm_start_transmission(chirpy_demo_state_t *state) {
    watch_set_indicator(WATCH_INDICATOR_BELL);
    state->mode = CDM_CHIRPING;

    // Set up the data
    curr_state = state;
    curr_data_ix = 0;
    if (state->program == CDP_INFO_SHORT) {
        curr_data_ptr = short_data;
        curr_data_len = short_data_len;
    } else if (state->program == CDP_INFO_LONG) {
        curr_data_ptr = long_data_str;
        curr_data_len = strlen((const char *)long_data_str);
    } else if (state->program == CDP_INFO_NANOSEC) {
        curr_data_ptr = activity_buffer;
        curr_data_len = activity_buffer_size;
    }

    chirpy_init_encoder(&state->encoder_state, _cdf_get_next_byte);
    watch_buzzer_play_raw_source(_cdm_raw_source_fn, state, _cdf_on_chirping_done);
}

bool chirpy_demo_face_loop(movement_event_t event, void *context) {
    chirpy_demo_state_t *state = (chirpy_demo_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _cdf_update_lcd(state);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
        case EVENT_LIGHT_BUTTON_UP:
            // We don't do light.
            break;
        case EVENT_ALARM_BUTTON_UP:
            // If in choose mode: select next program
            if (state->mode == CDM_CHOOSE) {
                if (state->program == CDP_CLEAR)
                    state->program = CDP_INFO_SHORT;
                else if (state->program == CDP_INFO_SHORT)
                    state->program = CDP_INFO_LONG;
                else if (state->program == CDP_INFO_LONG) {
                    if (activity_buffer_size > 0)
                        state->program = CDP_INFO_NANOSEC;
                    else
                        state->program = CDP_CLEAR;
                } else if (state->program == CDP_INFO_NANOSEC)
                    state->program = CDP_CLEAR;
                _cdf_update_lcd(state);
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            // If in choose mode: start chirping
            if (state->mode == CDM_CHOOSE) {
                if (state->program == CDP_CLEAR) {
                    filesystem_write_file("activity.dat", "", 0);
                    movement_force_led_off();
                    movement_move_to_next_face();
                } else {
                    _cdm_start_transmission(state);
                }
            }
            break;
        case EVENT_TIMEOUT:
            // Do not time out while we're chirping
            if (state->mode != CDM_CHIRPING) {
                movement_move_to_face(0);
            }
            // fall through
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void chirpy_demo_face_resign(void *context) {
    (void)context;

    if (activity_buffer != 0) {
        free(activity_buffer);
        activity_buffer = 0;
        activity_buffer_size = 0;
    }
}
