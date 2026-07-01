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
#define METRONOME_BPM_MAX     (209u)

typedef struct {
    bool ticking;
    bool beep;
    bool ticktock;
    bool setting;
    bool setting_scroll;
    bool setting_flasher;
    uint8_t setting_item;
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
    // If we're in setting mode, blank out the item we're setting every other tick.
    if (metronome->setting && !metronome->setting_scroll && !metronome->setting_flasher) {
        if (metronome->setting_item == 0) {
            buf[0] = buf[1] = ' ';
        } else {
            buf[2] = ' ';
        }
    }
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

// Tick handler while metronome->ticking is true.
static void running_tick(metronome_state_t *metronome) {
    if (metronome->ticks_countdown)
        metronome->ticks_countdown--;
    if (metronome->ticks_countdown == 0) {
        // A metronome beat has occurred.
        // Reset the countdown.
        metronome->ticks_countdown = metronome->ticks_calc;
        // Update the remainder counter. If it overflows, increase the next countdown.
        // This provides more accurate BPM on average, at the cost of some jitter in the short term.
        metronome->ticks_count_remainder += metronome->ticks_calc_remainder;
        if (metronome->ticks_count_remainder >= metronome->bpm) {
            metronome->ticks_count_remainder -= metronome->bpm;
            if (metronome->ticks_count_remainder >= metronome->bpm)
                metronome->ticks_count_remainder = 0;
            metronome->ticks_countdown++;
        }
        // Display the tick or tock in the seconds position.
        metronome->ticktock = !metronome->ticktock;
        if (metronome->ticktock)
            watch_display_text(WATCH_POSITION_SECONDS, "# ");
        else
            watch_display_text(WATCH_POSITION_SECONDS, " #");
        // Beep if enabled.
        if (metronome->beep)
            watch_buzzer_play_note(BUZZER_NOTE_C8, 50);
    }
}

// Increment the BPM while running.
static void running_bpm_inc(metronome_state_t *metronome) {
    if (metronome->bpm < METRONOME_BPM_MAX) {
        metronome->bpm++;
        metronome_ticks_calc(metronome);
        metronome_display_bpm(metronome);
    }
}

// Decrement the BPM while running.
static void running_bpm_dec(metronome_state_t *metronome) {
    if (metronome->bpm > METRONOME_BPM_MIN) {
        metronome->bpm--;
        metronome_ticks_calc(metronome);
        metronome_display_bpm(metronome);
    }
}

// Enter setting mode, stop ticking and reset settings.
static void setting_enter(metronome_state_t *metronome) {
    metronome->setting = true;
    metronome->setting_flasher = false;
    metronome->setting_scroll = false;
    metronome->setting_item = 0;
    metronome_stop_ticking(metronome);
    watch_display_text(WATCH_POSITION_SECONDS, "  ");
    movement_request_tick_frequency(4);
}

static void setting_exit(metronome_state_t *metronome) {
    metronome->setting = false;
    metronome_display_bpm(metronome);
    metronome_start_ticking(metronome);
}

static void setting_digit_inc(metronome_state_t *metronome) {
    if (metronome->setting_item == 0) {
        // Increment the tens digit of the BPM, with wrap-around.
        if (metronome->bpm + 10u <= METRONOME_BPM_MAX)
            metronome->bpm += 10u;
        else
            metronome->bpm = (METRONOME_BPM_MIN - (METRONOME_BPM_MIN % 10u)) + (metronome->bpm % 10u);
    } else {
        // Increment the ones digit of the BPM, with wrap-around.
        metronome->bpm = metronome->bpm + (((metronome->bpm + 1u) % 10u) ? 1 : -9);
    }
}

static void setting_button_up_inc(metronome_state_t *metronome) {
    if (metronome->setting_scroll) {
        // Stop scroll.
        metronome->setting_scroll = false;
        movement_request_tick_frequency(4);
    } else {
        // Increment the digit being set, then recalculate the ticks for the new BPM.
        setting_digit_inc(metronome);
        metronome_ticks_calc(metronome);
    }
}

static void setting_button_long_start_scroll(metronome_state_t *metronome) {
    if (!metronome->setting_scroll) {
        // Start to scroll the BPM number.
        metronome->setting_scroll = true;
        metronome->setting_flasher = false;
        movement_request_tick_frequency(8);
    }
}

static void setting_button_long_up_stop_scroll(metronome_state_t *metronome) {
    if (metronome->setting_scroll) {
        // Stop scroll.
        metronome->setting_scroll = false;
        movement_request_tick_frequency(4);
    }
}

static void setting_reset_default(metronome_state_t *metronome) {
    // Reset to default BPM.
    metronome->bpm = METRONOME_BPM_DEFAULT;
    metronome_ticks_calc(metronome);
    metronome->setting_item = 0;
}

static void setting_next(metronome_state_t *metronome) {
    if (metronome->setting_item == 0) {
        // Move to the ones digit of the BPM.
        metronome->setting_item = 1;
    } else {
        // Exit setting mode, start ticking again.
        setting_exit(metronome);
    }
}

static void setting_tick(metronome_state_t *metronome) {
    if (metronome->setting_scroll) {
        if (HAL_GPIO_BTN_ALARM_read()) {
            setting_digit_inc(metronome);
            metronome_ticks_calc(metronome);
        }
    } else {
        // Flash the item being set.
        metronome->setting_flasher = !metronome->setting_flasher;
    }
    metronome_display_bpm(metronome);
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
            if (metronome->setting) {
                setting_button_up_inc(metronome);
            } else {
                running_bpm_inc(metronome);
            }
            break;
        case EVENT_ALARM_LONG_UP:
            if (metronome->setting) {
                setting_button_long_up_stop_scroll(metronome);
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (metronome->setting) {
                setting_button_long_start_scroll(metronome);
            } else {
                // Enter setting mode, stop ticking and reset settings.
                setting_enter(metronome);
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            // Inhibit the LED
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if (metronome->setting) {
                setting_next(metronome);
            } else {
                running_bpm_dec(metronome);
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (metronome->setting) {
                setting_reset_default(metronome);
            } else {
                // Toggle beep on/off.
                metronome->beep = !metronome->beep;
                metronome_indicate_beep(metronome);
            }
            break;
        case EVENT_TICK:
            if (metronome->setting) {
                setting_tick(metronome);
            }
            if (metronome->ticking) {
                running_tick(metronome);
            }
            break;
        case EVENT_TIMEOUT:
            if (metronome->setting) {
                // Exit setting mode, start ticking again.
                setting_exit(metronome);
            }
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
