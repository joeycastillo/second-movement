/*
 * MIT License
 *
 * Copyright (c) 2023 Austin Teets
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
#include "watch_utility.h"

static const int8_t _sound_seq_start[] = {BUZZER_NOTE_C8, 2, 0};
static const int8_t _sound_seq_beat[] = {BUZZER_NOTE_C6, 2, 0};

#define TAP_DETECTION_SECONDS 3


static void _metronome_face_update_lcd(metronome_state_t *state) {
    char buf[11];
    if (state->soundOn) {
        watch_set_indicator(WATCH_INDICATOR_BELL);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_BELL);
    }

    if (state->tap_tempo.detection_ticks > 0) {
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    }

    if (state->mode == tapTempo) {
        // Alternate "b " and " p" on each tap for visual feedback
        const char *tap_indicator = (state->tap_tempo.counter % 2 == 0) ? " b" : " p";
        sprintf(buf, "MN %d %03d%s", state->count, state->bpm, tap_indicator);
    } else {
        sprintf(buf, "MN %d %03d%s", state->count, state->bpm, "bp");
    }
    watch_display_string(buf, 0);
}

static void _metronome_start_tap_tempo(metronome_state_t *state) {
    printf("starting tap tempo mode\n");

    state->mode = tapTempo;
    state->tap_tempo.counter = 0;
    state->tap_tempo.ground_zero = 0;
    state->tap_tempo.last_tap = 0;
    state->tap_tempo.previous_tap = 0;

    movement_request_tick_frequency(64);
    printf("requesting 64Hz tick frequency for tap timing\n");

    state->tap_tempo.detection_ticks = TAP_DETECTION_SECONDS * 64;
    printf("setting timeout to %d ticks for 64Hz frequency\n", state->tap_tempo.detection_ticks);

    watch_set_indicator(WATCH_INDICATOR_SIGNAL);

    // try to enable accelerometer detection if available (optional)
    if (movement_enable_tap_detection_if_available()) {
        printf("accelerometer tap detection enabled\n");
    } else {
        printf("accelerometer not available, using light button only\n");
    }
}

static void _metronome_abort_tap_detection(metronome_state_t *state) {
    printf("aborting tap detection (timeout or manual)\n");
    state->tap_tempo.detection_ticks = 0;
    movement_disable_tap_detection_if_available();
    state->mode = metWait;

    movement_request_tick_frequency(2);
    printf("returning to 2Hz tick frequency\n");

    watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
}

static void _metronome_handle_tap(metronome_state_t *state) {
    watch_date_time_t dt = movement_get_utc_date_time();
    uint32_t current_time = dt.unit.second * 1000 + (state->tap_tempo.subsecond * 1000) / 64;

    printf("tap detected! Current time: %u ms (second: %d, subsec: %d)\n",
           current_time, dt.unit.second, state->tap_tempo.subsecond);

    // If first tap, record ground zero
    if (state->tap_tempo.last_tap == 0) {
        state->tap_tempo.ground_zero = current_time;
        state->tap_tempo.counter = 0;
        printf("first tap, establishing ground zero at %u ms\n", current_time);
    }

    state->tap_tempo.last_tap = current_time;

    // Calculate elapsed time since previous tap, handling minute wrap-around
    uint32_t elapsed;
    if (current_time >= state->tap_tempo.previous_tap) {
        elapsed = current_time - state->tap_tempo.previous_tap;
    } else {
        // Wrapped around the minute boundary
        elapsed = (60000 - state->tap_tempo.previous_tap) + current_time;
    }

    state->tap_tempo.previous_tap = state->tap_tempo.last_tap;

    // Calculate total time since ground zero, handling minute wrap-around
    uint32_t tap_diff;
    if (state->tap_tempo.last_tap >= state->tap_tempo.ground_zero) {
        tap_diff = state->tap_tempo.last_tap - state->tap_tempo.ground_zero;
    } else {
        // Wrapped around the minute boundary
        tap_diff = (60000 - state->tap_tempo.ground_zero) + state->tap_tempo.last_tap;
    }

    state->tap_tempo.counter++;

    printf("tap %d: elapsed since last = %u ms, total time = %u ms\n",
           state->tap_tempo.counter, elapsed, tap_diff);

    if (tap_diff != 0 && state->tap_tempo.counter > 1) {
        uint16_t avg_bpm = (uint16_t)((60000 * (state->tap_tempo.counter - 1)) / tap_diff);
        printf("calculated BPM: %d\n", avg_bpm);

        if (avg_bpm >= 30 && avg_bpm <= 255) {
            printf("setting BPM to %d (was %d)\n", avg_bpm, state->bpm);
            state->bpm = avg_bpm;
            _metronome_face_update_lcd(state);
        } else {
            printf("BPM %d out of range, ignoring\n", avg_bpm);
        }
    }

    state->tap_tempo.detection_ticks = TAP_DETECTION_SECONDS * 64; // Reset timeout for 64Hz
}

void metronome_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(metronome_state_t));
        memset(*context_ptr, 0, sizeof(metronome_state_t));
    }
}

void metronome_face_activate(void *context) {
    metronome_state_t *state = (metronome_state_t *)context;
    movement_request_tick_frequency(2);
    if (state->bpm == 0) {
        state->count = 4;
        state->bpm = 120;
        state->soundOn = true;
    }
    state->mode = metWait;
    state->correction = 0;
    state->setCur = hundred;
}

static void _metronome_start_stop(metronome_state_t *state) {
    if (state->mode != metRun) {
        // Safety check: ensure BPM is valid
        if (state->bpm == 0) {
            state->bpm = 120;
        }
        movement_request_tick_frequency(64);
        state->mode = metRun;
        watch_clear_display();
        double ticks = 3840.0 / (double)state->bpm;
        state->tick = (int) ticks;
        state->curTick = 0;
        state->halfBeat = (int)(state->tick/2);
        state->curCorrection = 0;
        state->correction = ticks - state->tick;
        state->curBeat = 1;
    } else {
        state->mode = metWait;
        movement_request_tick_frequency(2);
        _metronome_face_update_lcd(state);
    }
}

static void _metronome_tick_beat(metronome_state_t *state) {
    char buf[11];
    if (state->soundOn) {
        if (state->curBeat == 1) {
            watch_buzzer_play_sequence((int8_t *)_sound_seq_start, NULL);
        } else {
            watch_buzzer_play_sequence((int8_t *)_sound_seq_beat, NULL);
        }
    }
    sprintf(buf, "MN %d %03d%s", state->count, state->bpm, "bp");
    watch_display_string(buf, 0);
}

static void _metronome_event_tick(uint8_t subsecond, metronome_state_t *state) {
    (void) subsecond;

    state->curTick++;

    // Determine target: base tick count plus extension if error accumulated
    // Use epsilon to handle floating-point rounding errors
    int target = state->tick;
    if (state->curCorrection >= 0.99) {
        target++;
    }

    if (state->curTick >= target) {
        _metronome_tick_beat(state);
        state->curTick = 0;

        // After beat fires: subtract if we used the extension, then accumulate for next beat
        if (state->curCorrection >= 0.99) {
            state->curCorrection -= 1.0;
        }
        state->curCorrection += state->correction;

        if (state->curBeat < state->count) {
            state->curBeat += 1;
        } else {
            state->curBeat = 1;
        }
    } else if (state->curTick == state->halfBeat) {
        watch_clear_display();
    }
}

static void _metronome_setting_tick(uint8_t subsecond, metronome_state_t *state) {
    char buf[13];
    sprintf(buf, "MN %d %03d%s", state->count, state->bpm, "bp"); 
    if (subsecond%2 == 0) {
        switch (state->setCur) {
            case hundred:
                buf[5] = ' ';
                break;
            case ten:
                buf[6] = ' ';
                break;
            case one:
                buf[7] = ' ';
                break;
            case count:
                buf[3] = ' ';
                break;
            case alarm:
                break;
        }
    }
    if (state->setCur == alarm) {
        sprintf(buf, "MN  8eep%s", state->soundOn ? "On" : " -");
    }
    if (state->soundOn) {
        watch_set_indicator(WATCH_INDICATOR_BELL);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_BELL);
    }
    watch_display_string(buf, 0);
}

static void _metronome_update_setting(metronome_state_t *state) {
    char buf[13];
    switch (state->setCur) {
        case hundred:
            if (state->bpm < 100) {
                state->bpm += 100;
            } else {
                state->bpm -= 100;
            }
            break;
        case ten:
            if ((state->bpm / 10) % 10 < 9) {
                state->bpm += 10;
            } else {
                state->bpm -= 90;
            }
            break;
        case one:
            if (state->bpm%10 < 9) {
                state->bpm += 1;
            } else {
                state->bpm -= 9;
            }
            break;
        case count:
            if (state->count < 9) {
                state->count += 1;
            } else {
                state->count = 2;
            }
            break;
        case alarm:
            state->soundOn = !state->soundOn;
            break;
    }
    // Ensure BPM never goes below 1 to prevent division by zero
    if (state->bpm == 0) {
        state->bpm = 1;
    }
    sprintf(buf, "MN %d %03d%s", state->count % 10, state->bpm, "bp"); 
    if (state->setCur == alarm) {
        sprintf(buf, "MN  8eep%s", state->soundOn ? "On" : " -");
    }
    if (state->soundOn) {
        watch_set_indicator(WATCH_INDICATOR_BELL);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_BELL);
    }
    watch_display_string(buf, 0);
}

bool metronome_face_loop(movement_event_t event, void *context) {
    metronome_state_t *state = (metronome_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _metronome_face_update_lcd(state);
            break;
        case EVENT_TICK:
            if (state->mode == metRun){
                _metronome_event_tick(event.subsecond, state);
            } else if (state->mode == setMenu) {
                _metronome_setting_tick(event.subsecond, state);
            } else if (state->tap_tempo.detection_ticks > 0) {
                state->tap_tempo.detection_ticks--;
                if (state->tap_tempo.detection_ticks == 0) {
                    printf("tap detection timeout reached\n");
                    _metronome_abort_tap_detection(state);
                    _metronome_face_update_lcd(state);
                }
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->mode == setMenu) {
                _metronome_update_setting(state);
            } else {
                if (state->tap_tempo.detection_ticks > 0) {
                    _metronome_abort_tap_detection(state);
                }
                _metronome_start_stop(state);
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            printf("light button pressed, mode: %d, subsecond: %d\n", state->mode, event.subsecond);
            if (state->mode == setMenu) {
                if (state->setCur < alarm) {
                    state->setCur += 1;
                } else {
                    state->setCur = hundred;
                }
            } else if (state->mode == tapTempo) {
                printf("in tap tempo mode, handling light button tap\n");
                state->tap_tempo.subsecond = event.subsecond;
                _metronome_handle_tap(state);
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if (state->mode == metWait) {
                _metronome_start_tap_tempo(state);
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (state->mode != metRun && state->mode != setMenu) {
                if (state->tap_tempo.detection_ticks > 0) {
                    _metronome_abort_tap_detection(state);
                }
                movement_request_tick_frequency(2);
                state->mode = setMenu;
                _metronome_face_update_lcd(state);
            } else if (state->mode == setMenu) {
                state->mode = metWait;
                _metronome_face_update_lcd(state);
            }
            break;
        case EVENT_MODE_BUTTON_UP:
            movement_move_to_next_face();
            break;
        case EVENT_TIMEOUT:
            if (state->mode != metRun) {
                movement_move_to_face(0);
            }
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            break;
        case EVENT_SINGLE_TAP:
            if (state->mode == tapTempo) {
                state->tap_tempo.subsecond = event.subsecond;
                _metronome_handle_tap(state);
            }
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void metronome_face_resign(void *context) {
    metronome_state_t *state = (metronome_state_t *)context;
    if (state->tap_tempo.detection_ticks > 0) {
        _metronome_abort_tap_detection(state);
    }
}
