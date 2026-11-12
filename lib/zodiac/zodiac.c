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

#include <math.h>

#include "zodiac.h"
#include "sunriset.h"

const zodiac_sign_t zodiac_signs[ZODIAC_SIGN_COUNT] = {
    {"Aquar", "AQ"},
    {"Pisce", "PI"},
    {"Aries", "AR"},
    {"Tauru", "TA"},
    {"Gemin", "GE"},
    {"Cance", "CN"},
    {"Leo  ", "LE"},
    {"Virgo", "VI"},
    {"Libra", "LI"},
    {"Scorp", "SC"},
    {"Sagit", "SA"},
    {"Capri", "CR"},
};

#define SECONDS_PER_DAY 86400
// 2000 Jan 0.0 UT (1999-12-31 00:00), the epoch of sunpos().
#define SUNRISET_EPOCH 946598400
// The Sun's mean motion along the ecliptic, degrees per day.
#define SUN_DEGREES_PER_DAY 0.9856474

// Month and day of a Unix time on or after 1970 (Hinnant's civil_from_days).
static void _civil_date(int64_t t, uint8_t *month, uint8_t *day) {
    int64_t z = t / SECONDS_PER_DAY + 719468;
    int64_t era = z / 146097;
    int64_t doe = z - era * 146097;                                        // day of era
    int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;   // year of era
    int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);                 // day of year, from 1 March
    int64_t mp = (5 * doy + 2) / 153;                                      // month, from March
    *day = (uint8_t)(doy - (153 * mp + 2) / 5 + 1);
    *month = (uint8_t)(mp < 10 ? mp + 3 : mp - 9);
}

static double _sun_longitude(double days) {
    double lon, r;
    sunpos(days, &lon, &r);
    return lon;
}

// Aries starts at 0 degrees and is third in zodiac_signs[].
uint8_t zodiac_sign_at(int64_t now) {
    double days = (double)(now - SUNRISET_EPOCH) / SECONDS_PER_DAY;
    uint8_t thirty = (uint8_t)(_sun_longitude(days) / 30.0) % ZODIAC_SIGN_COUNT;
    return (thirty + 2) % ZODIAC_SIGN_COUNT;
}

void zodiac_sign_start_date(uint8_t sign, int64_t now, int32_t utc_offset,
                            uint8_t *month, uint8_t *day) {
    double target = ((sign + 10) % ZODIAC_SIGN_COUNT) * 30.0;
    double days = (double)(now - SUNRISET_EPOCH) / SECONDS_PER_DAY;
    // Newton steps on the longitude, wrapped to -180..180 so they head for the
    // nearest entry. The Sun's speed varies by 3%, so four steps land within
    // seconds.
    for (int i = 0; i < 4; i++) {
        days += rev180(target - _sun_longitude(days)) / SUN_DEGREES_PER_DAY;
    }

    _civil_date(SUNRISET_EPOCH + (int64_t)floor(days * SECONDS_PER_DAY) + utc_offset, month, day);
}
