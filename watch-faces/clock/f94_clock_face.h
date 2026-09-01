/*
 * MIT License
 *
 * Copyright © 2026 Konrad Rieck <konrad@mlsec.org>
 * Copyright © 2021-2023 Joey Castillo <joeycastillo@utexas.edu> <jose.castillo@gmail.com>
 * Copyright © 2022 David Keck <davidskeck@users.noreply.github.com>
 * Copyright © 2022 TheOnePerson <a.nebinger@web.de>
 * Copyright © 2023 Jeremy O'Brien <neutral@fastmail.com>
 * Copyright © 2023 Mikhail Svarichevsky <3@14.by>
 * Copyright © 2023 Wesley Aptekar-Cassels <me@wesleyac.com>
 * Copyright © 2024 Matheus Afonso Martins Moreira <matheus.a.m.moreira@gmail.com>
 *
 * The clock display is derived from clock_face.c.
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

#ifndef F94_CLOCK_FACE_H_
#define F94_CLOCK_FACE_H_

/*
 * F-94 CLOCK FACE
 *
 * The standard clock face for a Sensor Watch in a Casio F-94 case. The F-94
 * shares the F-91W segment map. However, its five indicators form a ring at
 * the top right of the display. This watch face lets you use this ring as a
 * gauge for a single quantity instead of as separate indicators. For
 * example, it can be used to display the progress towards a daily step
 * goal, or the time remaining until a scheduled alarm.
 *
 * A watch face can register a source for the ring; see f94_ring.h for
 * details. Two sources are built-in: 60SEC fills the ring over a minute,
 * one segment every ten seconds (mode F94_RING_FILL). 5SEC fills the ring
 * over 5 seconds and clears it again over the next 5 seconds (mode
 * F94_RING_DOUBLE).
 *
 * The default source is OFF. The segments then carry their original meaning
 * (alarm, hourly chime, PM, 24H and low battery) and the face behaves as the
 * standard clock face. Any other source takes all five segments, so none of
 * those indications are available while it is selected. The exception is the
 * low battery warning, which preempts the ring on the SPL segment.
 *
 * Long-press LIGHT for settings, where ALARM selects the source and MODE
 * leaves.
 */

#include "movement.h"

typedef enum {
    PAGE_F94_CLOCK = 0,
    PAGE_F94_SETTINGS,
} f94_page_t;

#define F94_LEVEL_UNKNOWN 0xFF
#define F94_LEVEL_BATTERY 0xFE

typedef struct {
    watch_date_time_t previous; /* Time shown on the last tick */
    uint8_t last_battery_check; /* Day of the last battery check */
    bool time_signal;           /* Hourly time signal enabled */
    bool battery_low;           /* Battery voltage is low */
    f94_page_t page;            /* Current page */

    /* Ring */
    uint8_t ring_source;        /* Registry id of the source on show */
    uint8_t ring_shown;         /* Level last painted, or F94_LEVEL_UNKNOWN */
} f94_state_t;

void f94_clock_face_setup(uint8_t watch_face_index, void **context_ptr);
void f94_clock_face_activate(void *context);
bool f94_clock_face_loop(movement_event_t event, void *context);
void f94_clock_face_resign(void *context);
movement_watch_face_advisory_t f94_clock_face_advise(void *context);

#define f94_clock_face ((const watch_face_t) { \
    f94_clock_face_setup, \
    f94_clock_face_activate, \
    f94_clock_face_loop, \
    f94_clock_face_resign, \
    f94_clock_face_advise, \
})

#endif                          /* F94_CLOCK_FACE_H_ */
