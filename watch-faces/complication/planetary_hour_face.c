/*
 * MIT License
 *
 * Copyright (c) 2025 Your Name
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
 *
 * Planetary Hour Face
 * This face calculates and displays the current planetary hour based on the user's location and time.
 * Location can be set with an alarm long press, and the planetary hour is determined by the sunrise and sunset times.
 * Once location is set, short press on the alarm buttom will increment the target hour
 *
 */

#include "planetary_hour_face.h" 
#include "sunriset.h"
#include "watch.h"
#include "watch_utility.h"
#include "movement.h"
#include "watch_rtc.h" // For WATCH_RTC_REFERENCE_YEAR and related RTC definitions
#include <string.h>
#include <math.h> // Ensure math functions are available
#include <time.h>
#include <stdlib.h>
#include "filesystem.h"
#include <stdio.h> // For debug logging

#define SUNRISE_SUNSET_ALTITUDE (-35.0 / 60.0) // Altitude for sunrise/sunset calculations

#define PLANETARY_HOUR_ERROR 255 // Define an error code for planetary hour calculation failure
#define ZODIAC_SIGN_ERROR 255 // Define an error code for zodiac sign calculation failure

// Prototypes
static void calculate_astrological_sign(planetary_hour_state_t *state);


static const uint8_t _location_count = sizeof(longLatPresets) / sizeof(long_lat_presets_t);

// Update zodiac_signs to include date ranges for each sign
static const struct {
    const char *name;
    uint8_t start_month;
    uint8_t start_day;
    uint8_t end_month;
    uint8_t end_day;
} zodiac_signs[] = {
    {"Aries ", 3, 21, 4, 19},
    {"Taurus", 4, 20, 5, 20},
    {"Gemini", 5, 21, 6, 20},
    {"Cancer", 6, 21, 7, 22},
    {"Leo   ", 7, 23, 8, 22},
    {"Virgo ", 8, 23, 9, 22},
    {"Libra ", 9, 23, 10, 22},
    {"Scorpi", 10, 23, 11, 21},
    {"Sagitt", 11, 22, 12, 21},
    {"Capric", 12, 22, 1, 19},
    {"Aquari", 1, 20, 2, 18},
    {"Pisces", 2, 19, 3, 20}
};

// Map of planetary rulers to day of the week values
static const uint8_t week_days_to_chaldean_order[] = {
    3,  // Sunday
    6,  // Monday
    2,  // Tuesday
    5,  // Wednesday
    1,  // Thursday
    4,  // Friday
    0   // Saturday
};

// Define the struct for planet names and abbreviations
typedef struct {
    const char *name;
    const char *abbreviation;
} planet_names_t;

// Map of the Chaldean order numbers to the planets and abbreviations
static const planet_names_t planet_names[] = {
    {"Satur", "SA"},
    {"Jupit", "JU"},
    {"Mars ", "MA"},
    {"Sun  ", "SU"},
    {"Venus", "VE"},
    {"Mercu", "ME"},
    {"Moon ", "MO"}
};

static lat_lon_settings_t _planetary_hour_face_struct_from_latlon(int16_t val) {
    lat_lon_settings_t retval;

    retval.sign = val < 0;
    val = abs(val);
    retval.hundredths = val % 10;
    val /= 10;
    retval.tenths = val % 10;
    val /= 10;
    retval.ones = val % 10;
    val /= 10;
    retval.tens = val % 10;
    val /= 10;
    retval.hundreds = val % 10;

    return retval;
}

static void _planetary_hour_set_expiration(planetary_hour_state_t *state, watch_date_time_t next_hours_offset) {
    uint32_t timestamp = watch_utility_date_time_to_unix_time(next_hours_offset, 0);
    state->hour_offset_expires = watch_utility_date_time_from_unix_time(timestamp + 60, 0);
}

// this will return an entry from the planet_names map
static planet_names_t planetary_ruler_from_base_and_time(watch_date_time_t base_sunrise_local,
                                                         watch_date_time_t hour_start_local) {
    // 1. Calculate the day of the week from base_sunrise_local
    int year = base_sunrise_local.unit.year + WATCH_RTC_REFERENCE_YEAR;
    int month = base_sunrise_local.unit.month;
    int day = base_sunrise_local.unit.day;

    // Adjust for Zeller's Congruence if the month is January or February
    if (month < 3) {
        month += 12;
        year -= 1;
    }

    uint8_t day_of_week = (day + (2 * month) + (3 * (month + 1) / 5) + year + (year / 4) - (year / 100) + (year / 400) + 1) % 7;

    // 2. Calculate the time difference between sunrise and hour_start_local
    uint32_t sunrise_unix = watch_utility_date_time_to_unix_time(base_sunrise_local, 0);
    uint32_t hour_start_unix = watch_utility_date_time_to_unix_time(hour_start_local, 0);

    // Handle cases where the sunrise is the day before
    if (hour_start_unix < sunrise_unix) {
        hour_start_unix += 86400; // Add 24 hours in seconds
    }

    // Calculate the time difference in hours
    double time_since_sunrise = (double)(hour_start_unix - sunrise_unix) / 3600.0;
    int time_since_sunrise_int = (int)floor(time_since_sunrise);

    // 3. Get the ruler of the day from the day of the week
    uint8_t ruler_of_day_index = week_days_to_chaldean_order[day_of_week];

    // 4. Calculate the planetary ruler of the hour
    int ruler_index = (ruler_of_day_index + time_since_sunrise_int) % 7;

    // Return the corresponding planet from the planet_names map
    return planet_names[ruler_index];
}

// --- Small time helpers (same rounding/carry style as your sunrise/sunset face) ---
static watch_date_time_t _local_decimal_hours_to_dt(watch_date_time_t day_local, double local_hours_dec) {
    watch_date_time_t t = day_local;

    double minutes = 60.0 * fmod(local_hours_dec, 1.0);
    double seconds = 60.0 * fmod(minutes, 1.0);

    t.unit.hour = (uint8_t)floor(local_hours_dec);
    t.unit.minute = (seconds < 30.0) ? (uint8_t)floor(minutes) : (uint8_t)ceil(minutes);

    if (t.unit.minute == 60) {
        t.unit.minute = 0;
        t.unit.hour = (uint8_t)((t.unit.hour + 1) % 24);
        if (t.unit.hour == 0) {
            uint32_t ts = watch_utility_date_time_to_unix_time(t, 0);
            ts += 86400;
            t = watch_utility_date_time_from_unix_time(ts, 0);
        }
    }
    while (t.unit.hour >= 24) {
        t.unit.hour -= 24;
        uint32_t ts = watch_utility_date_time_to_unix_time(t, 0);
        ts += 86400;
        t = watch_utility_date_time_from_unix_time(ts, 0);
    }
    return t;
}

static bool _compute_local_sun_times(watch_date_time_t day_local,
                                     double lon, double lat, double hours_from_utc,
                                     watch_date_time_t *sunrise_local,
                                     watch_date_time_t *sunset_local) {
    double rise_utc_dec, set_utc_dec;
    uint8_t result = sun_rise_set(day_local.unit.year + WATCH_RTC_REFERENCE_YEAR,
                                  day_local.unit.month, day_local.unit.day,
                                  lon, lat, &rise_utc_dec, &set_utc_dec);
    if (result != 0) return false; // polar day/night or error
    *sunrise_local = _local_decimal_hours_to_dt(day_local, rise_utc_dec + hours_from_utc);
    *sunset_local  = _local_decimal_hours_to_dt(day_local, set_utc_dec  + hours_from_utc);
    return true;
}

static inline uint32_t _unix(watch_date_time_t t) { return watch_utility_date_time_to_unix_time(t, 0); }
static inline watch_date_time_t _from_unix(uint32_t ts) { return watch_utility_date_time_from_unix_time(ts, 0); }
static inline watch_date_time_t _midnight_of(watch_date_time_t t) {
    t.unit.hour = t.unit.minute = t.unit.second = 0;
    return t;
}
static inline watch_date_time_t _add_days(watch_date_time_t day_midnight, int d) {
    return _from_unix(_unix(day_midnight) + 86400u * (uint32_t)(d >= 0 ? d : -d) * (d >= 0 ? 1 : -1));
}

// A "segment" is either day (sunrise→sunset) or night (sunset→next sunrise). It’s keyed by the day it starts on.
typedef struct {
    bool is_day;                 // true = day seg, false = night seg
    watch_date_time_t day0;      // local midnight for the "day" the segment is keyed to
    watch_date_time_t start;     // local
    watch_date_time_t end;       // local
    double hour_len_sec;         // seconds length for one of the 12 hours in this segment
} ph_segment_t;

// Build a segment for a given local day (midnight) and type.
static bool _build_segment(bool is_day,
                           watch_date_time_t day0,
                           double lon, double lat, double hours_from_utc,
                           ph_segment_t *seg) {
    watch_date_time_t sr, ss;
    if (!_compute_local_sun_times(day0, lon, lat, hours_from_utc, &sr, &ss)) return false;

    watch_date_time_t sr_next, ss_dummy;
    if (!is_day) {
        watch_date_time_t day1 = _add_days(day0, 1);
        if (!_compute_local_sun_times(day1, lon, lat, hours_from_utc, &sr_next, &ss_dummy)) return false;
    }

    seg->is_day = is_day;
    seg->day0 = day0;
    if (is_day) {
        seg->start = sr;
        seg->end   = ss;
    } else {
        seg->start = ss;              // sunset(today)
        seg->end   = sr_next;         // sunrise(tomorrow)
    }
    double seg_len_sec = (double)((int32_t)_unix(seg->end) - (int32_t)_unix(seg->start));
    if (seg_len_sec <= 0.0) seg_len_sec = 1.0; // guard
    seg->hour_len_sec = seg_len_sec / 12.0;
    return true;
}

// Decide which segment "now" is in, and what "day0" that segment uses.
static bool _locate_segment_for_now(watch_date_time_t now_local,
                                    double lon, double lat, double hours_from_utc,
                                    ph_segment_t *seg) {
    watch_date_time_t today0 = _midnight_of(now_local);
    watch_date_time_t sr_today, ss_today;
    if (!_compute_local_sun_times(today0, lon, lat, hours_from_utc, &sr_today, &ss_today)) return false;

    uint32_t ts_now = _unix(now_local);
    if (ts_now >= _unix(sr_today) && ts_now < _unix(ss_today)) {
        return _build_segment(true, today0, lon, lat, hours_from_utc, seg);        // day of today
    } else if (ts_now >= _unix(ss_today)) {
        return _build_segment(false, today0, lon, lat, hours_from_utc, seg);       // night keyed to today
    } else {
        watch_date_time_t yday0 = _add_days(today0, -1);
        return _build_segment(false, yday0, lon, lat, hours_from_utc, seg);        // pre-sunrise → last night's segment
    }
}

// Get start of the hour containing 'ref' within seg, idx in [0..11].
static watch_date_time_t _hour_start_in_segment(const ph_segment_t *seg,
                                                watch_date_time_t ref,
                                                int *out_idx) {
    uint32_t ts_ref = _unix(ref), ts_s = _unix(seg->start), ts_e = _unix(seg->end);
    if (ts_ref < ts_s) ts_ref = ts_s;
    if (ts_ref >= ts_e) ts_ref = ts_e - 1;

    double pos = (double)((int32_t)ts_ref - (int32_t)ts_s);
    int idx = (int)floor(pos / seg->hour_len_sec);
    if (idx < 0) idx = 0;
    if (idx > 11) idx = 11;

    uint32_t ts_hour_start = ts_s + (uint32_t)floor(seg->hour_len_sec * (double)idx);
    if (out_idx) *out_idx = idx;
    return _from_unix(ts_hour_start);
}

// Advance (or rewind) hour starts by K steps across segments.
static bool _advance_hour_start(watch_date_time_t now_local,
                                int32_t k,
                                double lon, double lat, double hours_from_utc,
                                watch_date_time_t *out_hour_start,
                                ph_segment_t *out_seg) {
    ph_segment_t seg;
    if (!_locate_segment_for_now(now_local, lon, lat, hours_from_utc, &seg)) return false;

    int idx;
    watch_date_time_t hour_start = _hour_start_in_segment(&seg, now_local, &idx);

    while (k != 0) {
        if (k > 0) {
            int remaining_in_seg = 11 - idx;
            if (k <= remaining_in_seg) {
                // stay in this segment
                uint32_t ts = _unix(hour_start) + (uint32_t)floor(seg.hour_len_sec * (double)k);
                hour_start = _from_unix(ts);
                idx += k;
                k = 0;
            } else {
                // jump to next segment's first hour
                k -= (remaining_in_seg + 1);
                // next segment: day -> night(today), night -> day(tomorrow)
                watch_date_time_t next_day0 = seg.day0;
                bool next_is_day = !seg.is_day;
                if (!seg.is_day) { // night -> next day
                    next_day0 = _add_days(seg.day0, 1);
                }
                if (!_build_segment(next_is_day, next_day0, lon, lat, hours_from_utc, &seg)) return false;
                hour_start = seg.start; // first hour start in next segment
                idx = 0;
            }
        } else { // k < 0
            int back_in_seg = idx; // how many we can go back within this segment
            int need = -k;
            if (need <= back_in_seg) {
                uint32_t ts = _unix(hour_start) - (uint32_t)floor(seg.hour_len_sec * (double)need);
                hour_start = _from_unix(ts);
                idx -= need;
                k = 0;
            } else {
                // go to previous segment's last hour
                k += (back_in_seg + 1);
                // previous segment: day -> night(yesterday), night -> day(today)
                watch_date_time_t prev_day0 = seg.day0;
                bool prev_is_day = !seg.is_day;
                if (seg.is_day) { // day -> previous night's segment (keyed to yesterday)
                    prev_day0 = _add_days(seg.day0, -1);
                }
                ph_segment_t pseg;
                if (!_build_segment(prev_is_day, prev_day0, lon, lat, hours_from_utc, &pseg)) return false;
                hour_start = _from_unix(_unix(pseg.start) + (uint32_t)floor(pseg.hour_len_sec * 11.0));
                seg = pseg;
                idx = 11;
            }
        }
    }

    if (out_hour_start) *out_hour_start = hour_start;
    if (out_seg) *out_seg = seg;
    return true;
}

static void _planetary_hour_face_advance_digit(planetary_hour_state_t *state) {
    state->location_state.location_changed = true;
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        switch (state->location_state.page) {
            case 1: // latitude
                switch (state->location_state.active_digit) {
                    case 0: // tens
                        state->location_state.working_latitude.tens = (state->location_state.working_latitude.tens + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_latitude)) > 9000) {
                            // prevent latitude from going over ±90.
                            // TODO: perform these checks when advancing the digit?
                            state->location_state.working_latitude.ones = 0;
                            state->location_state.working_latitude.tenths = 0;
                            state->location_state.working_latitude.hundredths = 0;
                        }
                        break;
                    case 1:
                        state->location_state.working_latitude.ones = (state->location_state.working_latitude.ones + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_latitude)) > 9000) state->location_state.working_latitude.ones = 0;
                        break;
                    case 2:
                        state->location_state.working_latitude.tenths = (state->location_state.working_latitude.tenths + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_latitude)) > 9000) state->location_state.working_latitude.tenths = 0;
                        break;
                    case 3:
                        state->location_state.working_latitude.hundredths = (state->location_state.working_latitude.hundredths + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_latitude)) > 9000) state->location_state.working_latitude.hundredths = 0;
                        break;
                    case 4:
                        state->location_state.working_latitude.sign++;
                        break;
                }
                break;
            case 2: // longitude
                switch (state->location_state.active_digit) {
                    case 0:
                        // Increase tens and handle carry-over to hundreds
                        state->location_state.working_longitude.tens++;
                        if (state->location_state.working_longitude.tens >= 10) {
                            state->location_state.working_longitude.tens = 0;
                            state->location_state.working_longitude.hundreds++;
                        }

                        // Reset if we've gone over ±180
                        if (abs(_latlon_from_struct(state->location_state.working_longitude)) > 18000) {
                            state->location_state.working_longitude.hundreds = 0;
                            state->location_state.working_longitude.tens = 0;
                            state->location_state.working_longitude.ones = 0;
                            state->location_state.working_longitude.tenths = 0;
                            state->location_state.working_longitude.hundredths = 0;
                        }
                        break;
                    case 1:
                        state->location_state.working_longitude.ones = (state->location_state.working_longitude.ones + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_longitude)) > 18000) state->location_state.working_longitude.ones = 0;
                        break;
                    case 2:
                        state->location_state.working_longitude.tenths = (state->location_state.working_longitude.tenths + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_longitude)) > 18000) state->location_state.working_longitude.tenths = 0;
                        break;
                    case 3:
                        state->location_state.working_longitude.hundredths = (state->location_state.working_longitude.hundredths + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_longitude)) > 18000) state->location_state.working_longitude.hundredths = 0;
                        break;
                    case 4:
                        state->location_state.working_longitude.sign++;
                        break;
                }
                break;
        }
    } else {
        switch (state->location_state.page) {
            case 1: // latitude
                switch (state->location_state.active_digit) {
                    case 0:
                        state->location_state.working_latitude.sign++;
                        break;
                    case 1:
                        // we skip this digit
                        break;
                    case 2:
                        state->location_state.working_latitude.tens = (state->location_state.working_latitude.tens + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_latitude)) > 9000) {
                            // prevent latitude from going over ±90.
                            // TODO: perform these checks when advancing the digit?
                            state->location_state.working_latitude.ones = 0;
                            state->location_state.working_latitude.tenths = 0;
                            state->location_state.working_latitude.hundredths = 0;
                        }
                        break;
                    case 3:
                        state->location_state.working_latitude.ones = (state->location_state.working_latitude.ones + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_latitude)) > 9000) state->location_state.working_latitude.ones = 0;
                        break;
                    case 4:
                        state->location_state.working_latitude.tenths = (state->location_state.working_latitude.tenths + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_latitude)) > 9000) state->location_state.working_latitude.tenths = 0;
                        break;
                    case 5:
                        state->location_state.working_latitude.hundredths = (state->location_state.working_latitude.hundredths + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_latitude)) > 9000) state->location_state.working_latitude.hundredths = 0;
                        break;
                }
                break;
            case 2: // longitude
                switch (state->location_state.active_digit) {
                    case 0:
                        state->location_state.working_longitude.sign++;
                        break;
                    case 1:
                        state->location_state.working_longitude.hundreds = (state->location_state.working_longitude.hundreds + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_longitude)) > 18000) {
                            // prevent longitude from going over ±180
                            state->location_state.working_longitude.tens = 8;
                            state->location_state.working_longitude.ones = 0;
                            state->location_state.working_longitude.tenths = 0;
                            state->location_state.working_longitude.hundredths = 0;
                        }
                        break;
                    case 2:
                        state->location_state.working_longitude.tens = (state->location_state.working_longitude.tens + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_longitude)) > 18000) state->location_state.working_longitude.tens = 0;
                        break;
                    case 3:
                        state->location_state.working_longitude.ones = (state->location_state.working_longitude.ones + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_longitude)) > 18000) state->location_state.working_longitude.ones = 0;
                        break;
                    case 4:
                        state->location_state.working_longitude.tenths = (state->location_state.working_longitude.tenths + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_longitude)) > 18000) state->location_state.working_longitude.tenths = 0;
                        break;
                    case 5:
                        state->location_state.working_longitude.hundredths = (state->location_state.working_longitude.hundredths + 1) % 10;
                        if (abs(_latlon_from_struct(state->location_state.working_longitude)) > 18000) state->location_state.working_longitude.hundredths = 0;
                        break;
                }
                break;
        }
    }
}

// --------------- MAIN: Planetary Hour Face (with hour_offset) -----------------
static void _planetary_hour_face_update(planetary_hour_state_t *state) {
    char buf[14];
    movement_location_t movement_location;
    if (state->longLatToUse == 0 || _location_count <= 1) movement_location = load_location_from_filesystem();
    else {
        movement_location.bit.latitude  = longLatPresets[state->longLatToUse].latitude;
        movement_location.bit.longitude = longLatPresets[state->longLatToUse].longitude;
    }
    if (movement_location.reg == 0) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "PHour ", "PH");
        watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "No LOC", "No Loc");
        return;
    }

    watch_date_time_t now_local = movement_get_local_date_time();
    int16_t lat_centi = (int16_t)movement_location.bit.latitude;
    int16_t lon_centi = (int16_t)movement_location.bit.longitude;
    double lat = (double)lat_centi / 100.0;
    double lon = (double)lon_centi / 100.0;
    double hours_from_utc = ((double)movement_get_current_timezone_offset()) / 3600.0;

    // Find the target hour start by advancing hour_offset steps from "current planetary hour"
    watch_date_time_t target_hour_start;
    ph_segment_t seg_at_target;
    if (!_advance_hour_start(now_local, state->hour_offset, lon, lat, hours_from_utc,
                             &target_hour_start, &seg_at_target)) {
        // Fallback if polar day/night or compute error
        watch_clear_colon();
        watch_clear_indicator(WATCH_INDICATOR_PM);
        watch_clear_indicator(WATCH_INDICATOR_24H);
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "PHour", "PH");
        watch_display_text(WATCH_POSITION_BOTTOM, "None  ");
        return;
    }

    // ----- Planetary base selection (your rule; anchored to "now") -----
    watch_date_time_t today0 = _midnight_of(now_local);
    watch_date_time_t sr_today, ss_today;
    if (!_compute_local_sun_times(today0, lon, lat, hours_from_utc, &sr_today, &ss_today)) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "PHour", "PH");
        watch_display_text(WATCH_POSITION_BOTTOM, "None  ");
        return;
    }
    uint32_t ts_now = _unix(target_hour_start);
    _planetary_hour_set_expiration(state, target_hour_start);

    uint32_t ts_midnight_next = _unix(today0) + 86400;
    watch_date_time_t yday0 = _add_days(today0, -1);
    watch_date_time_t sr_yday, ss_yday;
    _compute_local_sun_times(yday0, lon, lat, hours_from_utc, &sr_yday, &ss_yday);

    watch_date_time_t base_sunrise =
        (ts_now >= _unix(sr_today) && ts_now < ts_midnight_next) ? sr_yday : sr_today;
    
    planet_names_t ruler = planetary_ruler_from_base_and_time(base_sunrise, target_hour_start);

    // ---- DISPLAY ----
    watch_set_colon();
    if (movement_clock_mode_24h()) watch_set_indicator(WATCH_INDICATOR_24H);
    if (!movement_clock_mode_24h()) {
        watch_date_time_t disp = target_hour_start;
        if (watch_utility_convert_to_12_hour(&disp)) watch_set_indicator(WATCH_INDICATOR_PM);
        else watch_clear_indicator(WATCH_INDICATOR_PM);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_PM);
    }

    watch_display_text_with_fallback(WATCH_POSITION_TOP, ruler.name, ruler.abbreviation);

    watch_date_time_t disp2 = target_hour_start;
    if (!movement_clock_mode_24h()) (void)watch_utility_convert_to_12_hour(&disp2);
    sprintf(buf, "%2d%02d%2d", disp2.unit.hour, disp2.unit.minute, target_hour_start.unit.day);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}


// Function to set up the planetary face, allocating memory for the context
void planetary_hour_face_setup(__attribute__((unused)) uint8_t watch_face_index, void **context_ptr) {
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(planetary_hour_state_t));
        if (*context_ptr == NULL) {
            return;
        }
        memset(*context_ptr, 0, sizeof(planetary_hour_state_t));
    }
}

// Function to activate the planetary face, initializing planetary hour and zodiac sign
void planetary_hour_face_activate(void *context) {
    planetary_hour_state_t *state = (planetary_hour_state_t *)context;
    // Initialize the location_state
    state->hour_offset = 0;
    state->longLatToUse = 0;
    state->hour_offset_expires = movement_get_local_date_time(); // force immediate update

    movement_location_t movement_location = load_location_from_filesystem();
    state->location_state.working_latitude = _planetary_hour_face_struct_from_latlon(movement_location.bit.latitude);
    state->location_state.working_longitude = _planetary_hour_face_struct_from_latlon(movement_location.bit.longitude);
    state->location_state.page = 0;
    state->location_state.active_digit = 0;
    state->location_state.location_changed = false;

    // calculate_astrological_sign(state);
}

// Main loop for the planetary face, handling events and updating the display
bool planetary_hour_face_loop(movement_event_t event, void *context) {
    planetary_hour_state_t *state = (planetary_hour_state_t *)context;

    // Check if the context is null to avoid dereferencing a null pointer
    if (state == NULL) {
        watch_display_text(WATCH_POSITION_TOP, "Error");
        watch_display_text(WATCH_POSITION_BOTTOM, "Error");
        return false;
    }

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            // calculate_astrological_sign(state); // Recalculate zodiac sign on activation
            _planetary_hour_face_update(state);
            break;

        case EVENT_LOW_ENERGY_UPDATE:
        case EVENT_TICK:
            if (state->location_state.page == 0) {
                // if entering low energy mode, start tick animation
                if (event.event_type == EVENT_LOW_ENERGY_UPDATE && !watch_sleep_animation_is_running()) watch_start_sleep_animation(1000);
                // check if we need to update the display
                watch_date_time_t date_time = movement_get_local_date_time();
                if (date_time.reg >= state->hour_offset_expires.reg) {
                    // and on the off chance that this happened before EVENT_TIMEOUT snapped us back to rise/set 0, go back now
                    state->hour_offset = 0;
                    _planetary_hour_face_update(state);
                }
            } else {
                _update_location_settings_display(event, &state->location_state);
            }
            break;

        case EVENT_ALARM_LONG_PRESS:
            if (state->location_state.page == 0) {
            if (state->longLatToUse != 0) {
                state->longLatToUse = 0;
                _planetary_hour_face_update(state);
                break;
            }
                state->location_state.page++;
                state->location_state.active_digit = 0;
                watch_clear_display();
                movement_request_tick_frequency(4);
                _update_location_settings_display(event, &state->location_state);
            }
            else {
                state->location_state.active_digit = 0;
                state->location_state.page = 0;
                _update_location_register(&state->location_state);
                _planetary_hour_face_update(state);
            }
            break;

        case EVENT_ALARM_BUTTON_UP:
            if (state->location_state.page) {
                _planetary_hour_face_advance_digit(state);
                _update_location_settings_display(event, &state->location_state);
            } else {
                state->hour_offset++;
                _planetary_hour_face_update(state);
            }
            break;

        case EVENT_TIMEOUT:
            if (load_location_from_filesystem().reg == 0) {
                // if no location set, return home
                movement_move_to_face(0);
            } else if (state->location_state.page || state->hour_offset) {
                // otherwise on timeout, exit settings mode and return to the current planetary hour
                state->location_state.page = 0;
                state->hour_offset = 0;
                movement_request_tick_frequency(1);
                _planetary_hour_face_update(state);
            }
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            if (state->location_state.page) {
                if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
                    state->location_state.active_digit++;
                    if (state->location_state.active_digit > 4) {
                        state->location_state.active_digit = 0;
                        state->location_state.page = (state->location_state.page + 1) % 3;
                        _update_location_register(&state->location_state);
                    }
                } else {
                    state->location_state.active_digit++;
                    if (state->location_state.page == 1 && state->location_state.active_digit == 1) state->location_state.active_digit++; // max latitude is +- 90, no hundreds place
                    if (state->location_state.active_digit > 5) {
                        state->location_state.active_digit = 0;
                        state->location_state.page = (state->location_state.page + 1) % 3;
                        _update_location_register(&state->location_state);
                    }
                }
                _update_location_settings_display(event, &state->location_state);
            } else if (_location_count <= 1) {
                movement_illuminate_led();
            }
            if (state->location_state.page == 0) {
                movement_request_tick_frequency(1);
                _planetary_hour_face_update(state);
            }
            break;

        case EVENT_LIGHT_LONG_PRESS:
            if (_location_count <= 1) break;
            else if (!state->location_state.page) movement_illuminate_led();
            break;

        case EVENT_LIGHT_BUTTON_UP:
            if (state->location_state.page == 0 && _location_count > 1) {
                state->longLatToUse = (state->longLatToUse + 1) % _location_count;
                _planetary_hour_face_update(state);
            }
            break;

        default:
            return movement_default_loop_handler(event); // Handle other events with default handler
    }
    return true;
}

// Function to release resources when the planetary face is no longer active
void planetary_hour_face_resign(void *context) {
    planetary_hour_state_t *state = (planetary_hour_state_t *)context;
    state->location_state.page = 0;
    state->location_state.active_digit = 0;
    state->hour_offset = 0;
    _update_location_register(&state->location_state);
}


// Function to determine the current astrological sign based on the date
static void calculate_astrological_sign(planetary_hour_state_t *state) {
    watch_date_time_t current_time = movement_get_local_date_time();
    uint8_t month = current_time.unit.month;
    uint8_t day = current_time.unit.day;

    for (int i = 0; i < (int)(sizeof(zodiac_signs) / sizeof(zodiac_signs[0])); i++) {
        if ((month == zodiac_signs[i].start_month && day >= zodiac_signs[i].start_day) ||
            (month == zodiac_signs[i].end_month && day <= zodiac_signs[i].end_day) ||
            (month > zodiac_signs[i].start_month && month < zodiac_signs[i].end_month)) {
            printf("Current Zodiac Sign: %s\n", zodiac_signs[i].name);
            state->current_zodiac_sign = i;
            return;
        }
    }

    // Set an error state if no match is found
    state->current_zodiac_sign = ZODIAC_SIGN_ERROR;
}
