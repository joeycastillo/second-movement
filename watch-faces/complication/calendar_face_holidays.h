/*
 * MIT License
 *
 * Copyright (c) 2025 Moritz Glöckl
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

#ifndef CALENDAR_FACE_HOLIDAYS_H
#define CALENDAR_FACE_HOLIDAYS_H

#include <stdint.h>
#include <stdbool.h>

// --- Pan-European Fixed-Date Holidays ---
#define HOLIDAY_NEW_YEARS_DAY 101  // January 1
#define HOLIDAY_MAY_DAY 501        // May 1 (Labour Day)
#define HOLIDAY_CHRISTMAS_DAY 1225 // December 25
#define HOLIDAY_BOXING_DAY 1226    // December 26

// --- Austria (AT) ---
#define HOLIDAY_AT_NATIONAL_DAY 1026          // October 26
#define HOLIDAY_AT_ASSUMPTION_DAY 815         // August 15
#define HOLIDAY_AT_IMMACULATE_CONCEPTION 1208 // December 8
#define HOLIDAY_AT_EPIPHANY 106               // January 6

// --- Germany (DE) ---
#define HOLIDAY_DE_UNITY_DAY 1003       // October 3
#define HOLIDAY_DE_REFORMATION_DAY 1031 // October 31
#define HOLIDAY_DE_ASSUMPTION_DAY 815   // August 15
#define HOLIDAY_DE_EPIPHANY 106         // January 6

// --- United Kingdom (UK) ---
#define HOLIDAY_UK_NEW_YEARS_DAY 101  // January 1
#define HOLIDAY_UK_MAY_DAY 501        // May 1
#define HOLIDAY_UK_CHRISTMAS_DAY 1225 // December 25
#define HOLIDAY_UK_BOXING_DAY 1226    // December 26

// --- France (FR) ---
#define HOLIDAY_FR_BASTILLE_DAY 714   // July 14
#define HOLIDAY_FR_ARMISTICE_DAY 1111 // November 11
#define HOLIDAY_FR_ASSUMPTION_DAY 815 // August 15
#define HOLIDAY_FR_EPIPHANY 106       // January 6

// --- Italy (IT) ---
#define HOLIDAY_IT_REPUBLIC_DAY 602           // June 2
#define HOLIDAY_IT_ASSUMPTION_DAY 815         // August 15
#define HOLIDAY_IT_EPIPHANY 106               // January 6
#define HOLIDAY_IT_IMMACULATE_CONCEPTION 1208 // December 8

// --- Spain (ES) ---
#define HOLIDAY_ES_NATIONAL_DAY 1012          // October 12
#define HOLIDAY_ES_ASSUMPTION_DAY 815         // August 15
#define HOLIDAY_ES_EPIPHANY 106               // January 6
#define HOLIDAY_ES_IMMACULATE_CONCEPTION 1208 // December 8

// --- Dynamic Holidays ---
#define HOLIDAY_EASTER_SUNDAY 0                               // Base: Easter
#define HOLIDAY_EASTER_MONDAY (HOLIDAY_EASTER_SUNDAY + 1)     // Easter Monday
#define HOLIDAY_GOOD_FRIDAY (HOLIDAY_EASTER_SUNDAY - 2)       // Good Friday
#define HOLIDAY_ASCENSION_DAY (HOLIDAY_EASTER_SUNDAY + 39)    // Ascension Day
#define HOLIDAY_PENTECOST_SUNDAY (HOLIDAY_EASTER_SUNDAY + 49) // Pentecost Sunday
#define HOLIDAY_PENTECOST_MONDAY (HOLIDAY_EASTER_SUNDAY + 50) // Pentecost Monday
#define HOLIDAY_CORPUS_CHRISTI (HOLIDAY_EASTER_SUNDAY + 60)   // Corpus Christi

static inline void calculate_easter_date(uint16_t year, uint8_t *month, uint8_t *day)
{
    // Meeus/Jones/Butcher algorithm
    uint16_t a = year % 19;
    uint16_t b = year / 100;
    uint16_t c = year - b * 100; // year % 100
    uint16_t d = b >> 2;         // b / 4
    uint16_t e = b & 3;          // b % 4
    uint16_t f = (b + 8) / 25;
    uint16_t g = (b - f + 1) / 3;
    uint16_t h = (19 * a + b - d - g + 15);
    h = h - ((h / 30) * 30);     // h % 30
    uint16_t i = c >> 2;         // c / 4
    uint16_t k = c & 3;          // c % 4
    uint16_t l = (32 + (e << 1) + (i << 1) - h - k);
    l = l - ((l / 7) * 7);       // l % 7
    uint16_t m = (a + 11 * h + 22 * l) / 451;
    uint16_t temp = h + l - 7 * m + 114;
    uint16_t month_calc = temp / 31;
    uint16_t day_calc = (temp - month_calc * 31) + 1;
    *month = (uint8_t)month_calc;
    *day = (uint8_t)day_calc;
}

static inline void calculate_dynamic_holiday(uint16_t year, int offset_days, uint8_t *month, uint8_t *day)
{
    uint8_t easter_month, easter_day;
    calculate_easter_date(year, &easter_month, &easter_day);

    bool leap = (year & 3) == 0 && ((year % 100 != 0) || (year % 400 == 0));
    static const uint8_t month_days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    uint16_t easter_doy = easter_day;
    for (uint8_t m = 1; m < easter_month; m++)
        easter_doy += (m == 2) ? (28 + leap) : month_days[m - 1];
    uint16_t holiday_doy = easter_doy + offset_days;
    uint8_t m = 1;
    while (holiday_doy > 0)
    {
        uint8_t dim = (m == 2) ? (28 + leap) : month_days[m - 1];
        if (holiday_doy <= dim) break;
        holiday_doy -= dim;
        m++;
    }
    *month = m;
    *day = (uint8_t)holiday_doy;
}

static inline bool is_public_holiday(uint8_t month, uint8_t day, uint16_t year, const uint16_t *holidays, size_t holiday_count)
{
    uint16_t md = month * 100 + day;
    for (size_t i = 0; i < holiday_count; i++)
    {
        // Dynamic holidays: offsets from Easter (-60 to +60)
        if ((int16_t)holidays[i] >= -60 && (int16_t)holidays[i] <= 60)
        {
            uint8_t dyn_month, dyn_day;
            calculate_dynamic_holiday(year, (int16_t)holidays[i], &dyn_month, &dyn_day);
            if (month == dyn_month && day == dyn_day)
                return true;
        }
        else if (holidays[i] > 0)
        {
            if (holidays[i] == md)
                return true;
        }
    }
    return false;
}

#endif // CALENDAR_FACE_HOLIDAYS_H
