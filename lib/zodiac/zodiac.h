/*
 * MIT License
 *
 * Copyright (c) 2026 second-movement contributors
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

// Zodiac signs from the Sun's ecliptic longitude (lib/sunriset). No hardware
// dependencies.
//
// sunpos() is a low-precision model: ingress times come out within about 15
// minutes, so an ingress that close to local midnight can show the
// neighbouring date.

#ifndef ZODIAC_H_
#define ZODIAC_H_

#include <stdint.h>

typedef struct {
    const char *name;          // 5 characters, for the custom LCD
    const char *abbreviation;  // 2 characters, for the classic LCD
} zodiac_sign_t;

#define ZODIAC_SIGN_COUNT 12

// In zodiac order, Aquarius (ecliptic longitude 300-330 degrees) first.
extern const zodiac_sign_t zodiac_signs[ZODIAC_SIGN_COUNT];

// Index into zodiac_signs[] of the sign the Sun is in at `now` (Unix time, UTC).
uint8_t zodiac_sign_at(int64_t now);

// Local month (1-12) and day (1-31) on which the Sun enters `sign`, for the
// entry nearest to `now` (Unix time, UTC): the current sign's start, a coming
// sign's next start, or a recent sign's last one. For the sign opposite the
// Sun, half a year either way, which entry is nearest flips around that day.
// `utc_offset` (seconds) is today's, so in a zone that changes daylight saving
// before or after the ingress, an ingress within an hour of local midnight
// can show the neighbouring date.
void zodiac_sign_start_date(uint8_t sign, int64_t now, int32_t utc_offset,
                            uint8_t *month, uint8_t *day);

#endif // ZODIAC_H_
