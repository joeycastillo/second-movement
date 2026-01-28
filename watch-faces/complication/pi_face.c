/*
 * MIT License
 *
 * Copyright (c) 2026 Michael Ciuffo
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
#include "pi_face.h"
#include "watch.h"

void pi_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(pi_state_t));
        memset(*context_ptr, 0, sizeof(pi_state_t));
        pi_state_t *state = (pi_state_t *)*context_ptr;
    }
}

void pi_face_activate(void *context) {
    (void) context;
    pi_state_t *state = (pi_state_t *)context;
    memset(state->r, 2000, sizeof(state->r));
    memset(state->r, 2000, 2800);
    state->r[2800] = 0;
    state->k = 2800;
    pi_calc(state);

}

// https://crypto.stanford.edu/pbc/notes/pi/code.html
void pi_calc(pi_state_t *state)
{
    int d = 0;
    int i = state->k;
    state->k -= 14;
    while(true)
    {
        d += state->r[state->i] * 10000;
        int b = 2 * i - 1;
        state->r[i] = d % b;
        d /= b;
        i -= 1;
        if (i == 0)
        {
            break;
        }
        d *= i;
    }
    int digit = state->c + d / 10000;
    state->c = d * 10000;
}

bool pi_face_loop(movement_event_t event, void *context) {
    pi_state_t *state = (pi_state_t *)context;
    static bool using_led = false;


    switch (event.event_type) {
        case EVENT_ALARM_BUTTON_UP:
            pi_calc(state);
            break;
        case EVENT_ACTIVATE:
            print_pi(state);
            break;
        case EVENT_TIMEOUT:
            // ignore timeout
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

// print tally index at the center of display.
void print_pi(pi_state_t *state) {
    /*char buf[6];
    int display_val = (int)(state->tally_idx);
    
    // Clamp to limits
    if (display_val > TALLY_FACE_MAX) display_val = TALLY_FACE_MAX;
    if (display_val < TALLY_FACE_MIN) display_val = TALLY_FACE_MIN;
    
    if (sound_on)
        watch_set_indicator(WATCH_INDICATOR_BELL);
    else
        watch_clear_indicator(WATCH_INDICATOR_BELL);
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "TALLY", "TA");
    sprintf(buf, "%4d", display_val);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    */
}

void pi_face_resign(void *context) {
    (void) context;
}
