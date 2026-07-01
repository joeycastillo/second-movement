/* SPDX-License-Identifier: MIT */

/*
 * MIT License
 *
 * Copyright © 2026 Craig McQueen <craig@mcqueen.au>
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

#include "metronome_face.h"
#include "watch.h"
#include "watch_common_display.h"

// tick frequency must be a power of two, between 1 and 128 Hz.
#define METRONOME_FACE_FREQUENCY (64u)

#define METRONOME_BPM_DEFAULT (120u)
#define METRONOME_BPM_MIN     (40u)
#define METRONOME_BPM_MAX     (180u)

typedef struct {
    bool ticking;
    bool beep;
    bool ticktock;
    uint8_t bpm;
    uint16_t ticks_calc;
    uint16_t ticks_calc_remainder;
    uint16_t ticks_countdown;
    uint16_t ticks_count_remainder;
} metronome_state_t;

static void metronome_display_title(metronome_state_t *metronome) {
    (void) metronome;
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "METRO", "ME");
}

static void metronome_display_bpm(metronome_state_t *metronome) {
    char buf[7];
    snprintf(buf, sizeof(buf), "%3u", metronome->bpm);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static void metronome_indicate_beep(metronome_state_t *metronome) {
    if (metronome->beep) {
        watch_set_indicator(WATCH_INDICATOR_BELL);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_BELL);
    }
}

static void metronome_ticks_calc(metronome_state_t *metronome) {
    uint16_t numerator = METRONOME_FACE_FREQUENCY * 60u;
    metronome->ticks_calc = numerator / metronome->bpm;
    metronome->ticks_calc_remainder = numerator % metronome->bpm;
}

static void metronome_start_ticking(metronome_state_t *metronome) {
    metronome->ticking = true;
    metronome->ticks_countdown = 1;
    metronome->ticks_count_remainder = 0;
    movement_request_tick_frequency(METRONOME_FACE_FREQUENCY);
}

static void metronome_stop_ticking(metronome_state_t *metronome) {
    movement_request_tick_frequency(1);
    metronome->ticking = false;
}

static void metronome_tick(metronome_state_t *metronome) {
    if (!metronome->ticking) { return; }

    if (metronome->ticks_countdown)
        metronome->ticks_countdown--;
    if (metronome->ticks_countdown == 0) {
        metronome->ticks_countdown = metronome->ticks_calc;
        metronome->ticks_count_remainder += metronome->ticks_calc_remainder;
        if (metronome->ticks_count_remainder >= metronome->bpm) {
            metronome->ticks_count_remainder -= metronome->bpm;
            if (metronome->ticks_count_remainder >= metronome->bpm) {
                metronome->ticks_count_remainder = 0;
            }
            metronome->ticks_countdown++;
        }
        metronome->ticktock = !metronome->ticktock;
        if (metronome->ticktock)
            watch_display_text(WATCH_POSITION_SECONDS, "# ");
        else
            watch_display_text(WATCH_POSITION_SECONDS, " #");
        if (metronome->beep)
            watch_buzzer_play_note(BUZZER_NOTE_C8, 50);
    }
}

void metronome_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        metronome_state_t *metronome = malloc(sizeof(metronome_state_t));

        memset(metronome, 0, sizeof(metronome_state_t));

        metronome->bpm = METRONOME_BPM_DEFAULT;
        metronome_ticks_calc(metronome);

        *context_ptr = metronome;
    }
}

void metronome_face_activate(void *context) {
    metronome_state_t *metronome = context;

    metronome->ticking = false;
    metronome->beep = false;

    if (metronome->bpm < METRONOME_BPM_MIN || metronome->bpm > METRONOME_BPM_MAX) {
        metronome->bpm = METRONOME_BPM_DEFAULT;
    }
    metronome_ticks_calc(metronome);

    metronome_start_ticking(metronome);

    metronome_display_title(metronome);
    metronome_display_bpm(metronome);
}

bool metronome_face_loop(movement_event_t event, void *context) {
    metronome_state_t *metronome = (metronome_state_t *) context;

    switch (event.event_type) {
        case EVENT_ALARM_BUTTON_UP:
            if (metronome->bpm < METRONOME_BPM_MAX) {
                metronome->bpm++;
                metronome_ticks_calc(metronome);
                metronome_display_bpm(metronome);
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            // Inhibit the LED
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if (metronome->bpm > METRONOME_BPM_MIN) {
                metronome->bpm--;
                metronome_ticks_calc(metronome);
                metronome_display_bpm(metronome);
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            metronome->beep = !metronome->beep;
            metronome_indicate_beep(metronome);
            break;
        case EVENT_TICK:
            metronome_tick(metronome);
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void metronome_face_resign(void *context) {
    metronome_state_t *metronome = (metronome_state_t *) context;

    metronome_stop_ticking(metronome);
}
