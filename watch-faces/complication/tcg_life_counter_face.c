/*
 * MIT License
 *
 * Copyright (c) 2025 Mark Schlosser
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
#include "tcg_life_counter_face.h"

#define TCG_LIFE_COUNTER_NUM_LIFE_VALUES 2

static const int16_t _tcg_life_counter_defaults[] = {
    20,
    40
};
#define TCG_LIFE_COUNTER_DEFAULT_SIZE() (sizeof(_tcg_life_counter_defaults) / sizeof(int16_t))

static const int8_t _tcg_life_counter_increment_amts[] = {
  1,
  5
};
#define TCG_LIFE_COUNTER_INCREMENT_AMTS_SIZE() (sizeof(_tcg_life_counter_increment_amts) / sizeof(int8_t))

void tcg_life_counter_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(tcg_life_counter_state_t));
        memset(*context_ptr, 0, sizeof(tcg_life_counter_state_t));
        tcg_life_counter_state_t *state = (tcg_life_counter_state_t *)*context_ptr;
        state->default_idx = 0;
        for (size_t i = 0; i < TCG_LIFE_COUNTER_NUM_LIFE_VALUES; i++)
          state->life_values[i] = _tcg_life_counter_defaults[state->default_idx];
        state->increment_mode_on = false;
        state->increment_idx = 0;
    }
}

void tcg_life_counter_face_activate(void *context) {
    tcg_life_counter_state_t *state = (tcg_life_counter_state_t *)context;
}

static bool _tcg_life_counter_is_initial_default_values(tcg_life_counter_state_t *state) {
    return (_tcg_life_counter_defaults[state->default_idx] == state->life_values[0] &&
              _tcg_life_counter_defaults[state->default_idx] == state->life_values[1] &&
              0 == state->increment_idx &&
              false == state->increment_mode_on);
}

bool tcg_life_counter_face_loop(movement_event_t event, void *context) {

    tcg_life_counter_state_t *state = (tcg_life_counter_state_t *)context;

    switch (event.event_type) {
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if (!state->increment_mode_on) {
              if (state->life_values[0] > 0) {
                int16_t temp;
                temp = (int16_t)state->life_values[0];
                temp -= _tcg_life_counter_increment_amts[state->increment_idx]; // decrement counter index 0
                if (temp < 0)
                  temp = 0;
                state->life_values[0] = (uint16_t)temp;
              }
            } else {
              if (state->life_values[0] < 999) {
                state->life_values[0] += _tcg_life_counter_increment_amts[state->increment_idx]; // increment counter index 0
                if (state->life_values[0] > 999)
                  state->life_values[0] = 999;
              }
            }
            print_tcg_life_counter(state);
            break;
        case EVENT_LIGHT_LONG_PRESS:
#ifndef TCG_LIFE_COUNTER_FACE_DISABLE_LED
            movement_illuminate_led();
#endif
            state->increment_mode_on = !state->increment_mode_on;
            print_tcg_life_counter(state);
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (!state->increment_mode_on) {
              if (state->life_values[1] > 0) {
                int16_t temp;
                temp = (int16_t)state->life_values[1];
                temp -= _tcg_life_counter_increment_amts[state->increment_idx]; // decrement counter index 0
                if (temp < 0)
                  temp = 0;
                state->life_values[1] = (uint16_t)temp;
              }
            } else {
              if (state->life_values[1] < 999) {
                state->life_values[1] += _tcg_life_counter_increment_amts[state->increment_idx]; // increment counter index 1
                if (state->life_values[1] > 999)
                  state->life_values[1] = 999;
              }
            }
            print_tcg_life_counter(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            // possibly advance to next set of default values
            if (_tcg_life_counter_is_initial_default_values(state)) {
                state->default_idx++;
                if (state->default_idx >= TCG_LIFE_COUNTER_DEFAULT_SIZE())
                  state->default_idx = 0;
              // reset all life counters
              for (size_t i = 0; i < TCG_LIFE_COUNTER_NUM_LIFE_VALUES; i++)
                state->life_values[i] = _tcg_life_counter_defaults[state->default_idx];
              state->increment_mode_on = false;
              state->increment_idx = 0;
              } else {
                state->increment_idx++;
                if (state->increment_idx >= TCG_LIFE_COUNTER_INCREMENT_AMTS_SIZE())
                  state->increment_idx = 0;
              }
              print_tcg_life_counter(state);
            break;
        case EVENT_MODE_LONG_PRESS:
            if (_tcg_life_counter_is_initial_default_values(state)) {
                movement_move_to_face(0);
              } else {
                // reset all life counters
                for (size_t i = 0; i < TCG_LIFE_COUNTER_NUM_LIFE_VALUES; i++)
                  state->life_values[i] = _tcg_life_counter_defaults[state->default_idx];
                state->increment_mode_on = false;
                state->increment_idx = 0;
                print_tcg_life_counter(state);
              }
            break;
        case EVENT_ACTIVATE:
            print_tcg_life_counter(state);
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

void print_tcg_life_counter(tcg_life_counter_state_t *state) {
    char buf[7];
    char buf2[3];
    watch_display_text(WATCH_POSITION_TOP, "TC");
    sprintf(buf2, state->increment_mode_on ? "i%1d" : "d%1d", _tcg_life_counter_increment_amts[state->increment_idx]);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf2);
    sprintf(buf, "%3d%3d", state->life_values[0], state->life_values[1]);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

void tcg_life_counter_face_resign(void *context) {
    (void) context;
}
