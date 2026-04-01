/*
 * MIT License
 *
 * Copyright (c) 2025 kbc-yam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef WAREKI_FACE_H_
#define WAREKI_FACE_H_

/*
Display Japanese era names (Wareki)

The displayed Japanese Era can be changed by the buttons on the watch, making it also usable as a converter between the Gregorian calendar and the Japanese Era.

Light button: Subtract one year from the Japanese Era.
Start/Stop button: Add one year to the Japanese Era.
Button operations support long-press functionality.

Japanese Era Notations:

r : REIWA (令和)
h : HEISEI (平成)
s : SHOWA(昭和)
*/

#include "movement.h"

#define REIWA_LIMIT 2018 + 99
#define REIWA_GANNEN  2019
#define HEISEI_GANNEN 1989
#define SHOWA_GANNEN 1926

typedef struct {
    bool active;
    uint32_t disp_year;     //Current displayed year 
    uint32_t start_year;    //Year when this screen was launched
    uint32_t real_year;     //The actual current year     
} wareki_state_t;

void wareki_setup(uint8_t watch_face_index, void ** context_ptr);
void wareki_activate(void *context);
bool wareki_loop(movement_event_t event, void *context);
void wareki_resign(void *context);

void addYear(wareki_state_t* state,int count);
void subYear(wareki_state_t* state,int count);

#define wareki_face ((const watch_face_t){ \
    wareki_setup, \
    wareki_activate, \
    wareki_loop, \
    wareki_resign, \
    NULL, \
})

#endif // WAREKI_FACE_H_

