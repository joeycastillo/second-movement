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
    }
}

void pi_face_activate(void *context) {
    (void) context;
    pi_state_t *state = (pi_state_t *)context;
    reset_pi(state);
}

// https://crypto.stanford.edu/pbc/notes/pi/code.html
int pi_calc(pi_state_t *state)
{
    int ret;
    int d = 0;
    int i = state->k;
    state->k -= 14;
    while(true)
    {
        d += state->r[i] * 10000;
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
    ret = state->c + d / 10000;
    state->c = d % 10000;
    return ret;
}

bool pi_face_loop(movement_event_t event, void *context) {
    pi_state_t *state = (pi_state_t *)context;
    switch (event.event_type) {
        case EVENT_ALARM_BUTTON_UP:
        case EVENT_ACTIVATE:
            if (state->k > 0)
            {
                print_pi(state);
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
        case EVENT_LIGHT_BUTTON_UP:
            reset_pi(state);
            print_pi(state);
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void print_pi(pi_state_t *state) {
    char buf[6];
    int num = pi_calc(state);
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "PI", "PI");
    sprintf(buf, "%04d", num);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

void reset_pi(pi_state_t * state)
{
    int i;
    for (i = 0; i < 2800; i++)
    {
        state->r[i] = 2000;
    }
    state->r[i] = 0;
    state->k = 2800;
    state->c = 0;
}

void pi_face_resign(void *context) {
    (void) context;
}
