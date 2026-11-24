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

// The offset from UTC in force in a utz zone at a given moment, daylight
// saving included. No hardware dependencies.

#ifndef ZONE_OFFSET_H_
#define ZONE_OFFSET_H_

#include <stdint.h>
#include "utz.h"

/// Minutes east of UTC in force in `zone` at the UTC date-time `utc`.
int16_t zone_offset_at_utc(const uzone_packed_t *zone, udatetime_t utc);

/// The same, for a date-time on the zone's own standard-time clock (UTC plus
/// the zone's base offset, with no daylight saving shift).
int16_t zone_offset_at_standard(const uzone_packed_t *zone, udatetime_t standard);

#endif // ZONE_OFFSET_H_
