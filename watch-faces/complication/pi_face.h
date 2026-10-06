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

#ifndef PI_FACE_H_
#define PI_FACE_H_

/*
 * PI face
 *
 * Pi face prints out the first 800 digits of pi four digits at a time.
 * Quiz yourself to see how many you can memorize!
 *
 * ALARM - advance to next 4 digits
 *
 * LIGHT - reset back to beginning
 * 
 * This is based off of the Beeler et al 1972, Item 120 algorithm
 * converted into C by Dik T. Winter and further expanded here
 * https://crypto.stanford.edu/pbc/notes/pi/code.html
 */

#include "movement.h"

typedef struct {
    int r[2800 + 1];
    int c;
    int k;
} pi_state_t;


void pi_face_setup(uint8_t watch_face_index, void ** context_ptr);
void pi_face_activate(void *context);
bool pi_face_loop(movement_event_t event, void *context);
void pi_face_resign(void *context);

void print_pi(pi_state_t *state);
int pi_calc(pi_state_t *state);
void reset_pi(pi_state_t * state);

#define pi_face ((const watch_face_t){ \
    pi_face_setup, \
    pi_face_activate, \
    pi_face_loop, \
    pi_face_resign, \
    NULL, \
})

#endif // PI_FACE_H_
