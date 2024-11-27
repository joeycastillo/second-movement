/*
 * MIT License
 *
 * Copyright (c) 2022 Josh Berson, building on Wesley Ellis’ countdown_face.c
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
#include "wake_face.h"
#include "watch.h"
#include "watch_utility.h"

//
// Private
//

static
void _wake_face_update_display(wake_face_state_t *state) {
    uint8_t hour = state->hour;

    watch_clear_display();
    if ( movement_clock_mode_24h() )
        watch_set_indicator(WATCH_INDICATOR_24H);
    else {
        if ( hour >= 12 )
            watch_set_indicator(WATCH_INDICATOR_PM);
        hour = hour % 12 ? hour % 12 : 12;
    }

    if ( state->mode )
        watch_set_indicator(WATCH_INDICATOR_BELL);

    static char lcdbuf[11];
    sprintf(lcdbuf, "WA  %2d%02d  ", hour, state->minute);

    watch_set_colon();
    watch_display_string(lcdbuf, 0);
}

//
// Exported
//

void wake_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(wake_face_state_t));
        wake_face_state_t *state = (wake_face_state_t *)*context_ptr;
        memset(*context_ptr, 0, sizeof(wake_face_state_t));

        state->hour = 5;
        state->minute = 0;
        state->mode = 0;
    }
}

void wake_face_activate(void *context) {
    (void) context;
}
void wake_face_resign(void *context) {
    (void) context;
}

movement_watch_face_advisory_t wake_face_advise(void *context) {
    wake_face_state_t *state = (wake_face_state_t *)context;
    movement_watch_face_advisory_t retval = { 0 };

    if ( state->mode ) {
        watch_date_time_t now = watch_rtc_get_date_time();
        retval.wants_background_task = state->hour==now.unit.hour && state->minute==now.unit.minute;
        // We’re at the mercy of the advise handler
        // In Safari, the emulator triggers at the ›end‹ of the minute
        // Converting to Unix timestamps and taking a difference between now and wake
        // is not an easy win — because the timestamp for wake has to rely on now
        // for its date. So first we’d have to see if the TOD of wake is after that
        // of now. If it is, take tomorrow’s date, calculating month and year rollover
        // if need be.
    }

    return retval;
}

bool wake_face_loop(movement_event_t event, void *context) {
    wake_face_state_t *state = (wake_face_state_t *)context;

    switch (event.event_type) {
    case EVENT_ACTIVATE:
    case EVENT_TICK:
        _wake_face_update_display(state);
        break;
    case EVENT_LIGHT_BUTTON_UP:
        state->hour = (state->hour + 1) % 24;
        _wake_face_update_display(state);
        break;
    case EVENT_LIGHT_LONG_PRESS:
        state->hour = (state->hour + 6) % 24;
        _wake_face_update_display(state);
        break;
    case EVENT_ALARM_BUTTON_UP:
        state->minute = (state->minute + 10) % 60;
        _wake_face_update_display(state);
        break;
    case EVENT_ALARM_LONG_PRESS:
        state->mode ^= 1;
        _wake_face_update_display(state);
        break;
    case EVENT_BACKGROUND_TASK:
        movement_play_alarm();
            // 2022-07-23: Thx @joeycastillo for the dedicated “alarm” signal
        break;
    case EVENT_TIMEOUT:
        movement_move_to_face(0);
        break;
    case EVENT_LOW_ENERGY_UPDATE:
        break;
    case EVENT_LIGHT_BUTTON_DOWN:
        // don't light up every time light is hit
        break;
    default:
        movement_default_loop_handler(event);
        break;
    }

    return true;
}
