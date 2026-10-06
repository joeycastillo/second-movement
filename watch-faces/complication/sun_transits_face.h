/* SPDX-License-Identifier: MIT */

/*
 * MIT License
 *
 * Copyright © 2021-2022 Joey Castillo <joeycastillo@utexas.edu> <jose.castillo@gmail.com>
 * Copyright © 2022 Alexsander Akers <me@a2.io>
 * Copyright © 2022 TheOnePerson <a.nebinger@web.de>
 * Copyright © 2023 Alex Utter <ooterness@gmail.com>
 * Copyright © 2024 Matheus Afonso Martins Moreira <matheus.a.m.moreira@gmail.com>
 * Copyright © 2026 Chris Gillings <github@ned-ludd.com>
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

#ifndef SUN_TRANSITS_FACE_H_
#define SUN_TRANSITS_FACE_H_

/*
 * SUN TRANSITS FACE
 *
 * Displays the dates of upcoming (and past) sun transits that mark
 * the seasonal Solstices, Equinoxes and Cross-Quarter days,
 * adjusted for hemisphere and local timezone.
 *
 * At activation, the next upcoming transit name and date is shown.
 *
 * The Alarm button steps forward to the next event and the Light
 * button steps back, covering any year between 2020 and 2082.
 *
 * The date is displayed in the Clock Digits as dd.mm.yy.
 *
 * The Event name is displayed in the Weekday Digits:
 *
 *    Spring Equinox ('Spr' / 'Su')
 *    Beltane Cross-Quarter ('BeL' / 'Be')
 *    Summer Solstice ( 'SUM' / 'Su')
 *    Lughnasadh/Lammas Cross-Quarter Day ('LUg' / 'Lu')
 *    Autumn Equinox ('Aut' / 'Au')
 *    Samhain Cross-Quarter Day ('Sam' / 'Sa')
 *    Winter Solstice ('Win' / 'Wi')
 *    Imbolc Cross-Quarter Day ('Imb' / 'Ib')
 *
 * The event type is displayed in the Day Digits position 3:
 *    Equinox ('E')
 *    Cross-Quarter ('q')
 *    Solstice ('S')
 *
 * The complication is signified with a triple '=' in the
 * Day Digits position 2 for all events. The symbol hints at the
 * movement of the sun between high latitudes and low latitudes
 * throughout the course of the year.
 *
 * If today is the date of a displayed transit, the Bell indicator
 * is energised to alert the user to the fact.
 *
 */

#include "movement.h"

#define SUN_TRANSIT_MINIMUM_DAY_GAP 44
#define SUN_TRANSIT_MAXIMUM_DAY_GAP 48

typedef struct {
    struct {
        watch_date_time_t previous;
    } date_time;
    uint8_t watch_face_index;
    bool time_signal_enabled;
} sun_transits_state_t;

void sun_transits_face_setup(uint8_t watch_face_index, void ** context_ptr);
void sun_transits_face_activate(void *context);
bool sun_transits_face_loop(movement_event_t event, void *context);
void sun_transits_face_resign(void *context);
movement_watch_face_advisory_t sun_transits_face_advise(void *context);

#define sun_transits_face ((const watch_face_t) { \
    sun_transits_face_setup, \
    sun_transits_face_activate, \
    sun_transits_face_loop, \
    sun_transits_face_resign, \
    sun_transits_face_advise, \
})

#endif // SUN_TRANSITS_FACE_H_
