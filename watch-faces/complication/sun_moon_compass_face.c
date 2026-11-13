/*
 * MIT License
 *
 * Copyright (c) 2026 Raffael Vogler
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
#include <math.h>
#include <stdio.h>

// for testing sun_compass and moon_compass using this file
#ifndef SUN_MOON_COMPASS_TEST

#include "filesystem.h"
#include "sun_moon_compass_face.h"


// classic display
static const char  TL0[] = "C0"; // C0mpass
static const char  TR0[] = "2L"; // 2un, Luna

// custom display
static const char  TL1[] = "CPS"; // ComPasS
static const char  TR1[] = "SL";  // Sun, Luna

// set LAT,LON to NAN if they should be sourced from location.u32
static const float LAT  = NAN;
static const float LON  = NAN;

static const uint8_t UPDATE_INTERVAL_T = 60;

static double sun_compass(int, int, int, int, int, int, float, float);
static double moon_compass(int, int, int, int, int, int, float, float);


// Loads coordinates from location.u32 if they are set there. 
// For setting coordinates to that file use sunrise/sunset face.
static void load_location(sun_moon_compass_state_t *state)
{
    if(!isnan(LAT) && !isnan(LON)) {
        state->lat = LAT;
        state->lon = LON;
    } else {
        movement_location_t location = {0};

        // prevents incomplete loading of locations from file
        if (filesystem_get_file_size("location.u32") == sizeof(location)
            && filesystem_read_file("location.u32", (char *)&location.reg, sizeof(location))
            && location.reg != 0) {
            state->lat = location.bit.latitude / 100.0f;
            state->lon = location.bit.longitude / 100.0f;
        } else {
            printf("ERROR: No coords specified.\n");
        }
        state->location_is_set = (
            !isnan(state->lat) && !isnan(state->lon) &&
            isfinite(state->lat) && isfinite(state->lon) &&
            (state->lat != 0.0f || state->lon != 0.0f) &&
            state->lat >= -90.0f && state->lat <= 90.0f &&
            state->lon >= -180.0f && state->lon <= 180.0f
        );
    }
    printf("INFO: lat = %6.2f, lon = %6.2f\n", state->lat, state->lon);
}

static void _display(const sun_moon_compass_state_t *state) {
    char buf[12];

    snprintf(buf, sizeof(buf), "%2d", (int)lroundf(state->sun_pos / 6.0f) % 60);
    watch_display_text(WATCH_POSITION_HOURS, buf);

    snprintf(buf, sizeof(buf), "%2d", (int)lroundf(state->moon_pos / 6.0f) % 60);
    watch_display_text(WATCH_POSITION_MINUTES, buf);

    if (state->ctr < 0) {
        watch_display_text(WATCH_POSITION_SECONDS, "  ");
    } else {
        snprintf(buf, sizeof(buf), "%02d", state->ctr);
        watch_display_text(WATCH_POSITION_SECONDS, buf);
    }
}

void sun_moon_compass_face_setup(uint8_t watch_face_index,
                                 void **context_ptr) {
    (void)watch_face_index;

    if (*context_ptr == NULL) {
        sun_moon_compass_state_t *state = malloc(sizeof(*state));
        if (state == NULL) return;

        *state = (sun_moon_compass_state_t){0};
        *context_ptr = state;
    }
}

void sun_moon_compass_face_activate(void *context) {
    sun_moon_compass_state_t *state = context;
    if (state == NULL) return;
    load_location(state);
    watch_set_colon();
}

static void _update(sun_moon_compass_state_t *state) {
    watch_date_time_t now = movement_get_utc_date_time();
    int year = now.unit.year + WATCH_RTC_REFERENCE_YEAR;

    state->sun_pos = (float) sun_compass(now.unit.hour, now.unit.minute, now.unit.second, 
        year, now.unit.month, now.unit.day, state->lat, state->lon);
    state->moon_pos = (float) moon_compass(now.unit.hour, now.unit.minute, now.unit.second, 
        year, now.unit.month, now.unit.day, state->lat, state->lon);
}

bool sun_moon_compass_face_loop(movement_event_t event, void *context) {
    sun_moon_compass_state_t *state = (sun_moon_compass_state_t *)context;
    if (state == NULL) return movement_default_loop_handler(event);

    // check if location could be loaded
    if (!state->location_is_set) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, TL1, TL0);
        watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, TR1, TR0);
        watch_display_text(WATCH_POSITION_HOURS, "  ");
        watch_display_text(WATCH_POSITION_MINUTES, "  ");
        watch_display_text(WATCH_POSITION_SECONDS, "--");
        return movement_default_loop_handler(event);
    }

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, TL1, TL0);
            watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, TR1, TR0);

            state->ctr = 0;
            _update(state);
            _display(state);
            break;
        case EVENT_TICK:
            state->ctr = (state->ctr == 0) ? UPDATE_INTERVAL_T - 1 : state->ctr - 1;
            if (state->ctr == 0) _update(state);
            _display(state);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            state->ctr = -1;
            _update(state);
            _display(state);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void sun_moon_compass_face_resign(void *context) {
    (void) context;
}

#endif


/* ====================================================================================================================
 *
 * Where the magic happens ...
 */

#define PI_D    3.14159265358979323846
#define DEG2RAD (PI_D / 180.0)
#define RAD2DEG (180.0 / PI_D)

static double norm360(double x)
{
    x = fmod(x, 360.0);
    if (x < 0.0) x += 360.0;
    return x;
}
static double sind(double d) { return sin(d * DEG2RAD); }
static double cosd(double d) { return cos(d * DEG2RAD); }


/*
 * Returns the angle (0..359, clockwise) on a circle whose 0° mark points at
 * the sun, at which North lies.
 * longitude: degrees, east positive. latitude: degrees, north positive.
 * Time is UTC. Valid for Gregorian dates (accuracy ~0.01° for 1950-2050).
 */
static double sun_compass(
    int hour, int minute, int second, int year, int month, int day, float latitude, float longitude
) {
    /* Julian Date (Gregorian calendar) */
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;
    long jdn = day + (153 * m + 2) / 5 + 365L * y + y / 4 - y / 100 + y / 400 - 32045;
    double jd = (double)jdn - 0.5 + (hour + minute / 60.0 + second / 3600.0) / 24.0;
    double n = jd - 2451545.0;                 /* days since J2000.0 */

    /* Sun's ecliptic longitude, obliquity */
    double L      = norm360(280.460 + 0.9856474 * n);
    double g      = norm360(357.528 + 0.9856003 * n) * DEG2RAD;
    double lambda = (L + 1.915 * sin(g) + 0.020 * sin(2.0 * g)) * DEG2RAD;
    double eps    = (23.439 - 0.0000004 * n) * DEG2RAD;

    /* Right ascension and declination (radians) */
    double ra  = atan2(cos(eps) * sin(lambda), cos(lambda));
    double dec = asin(sin(eps) * sin(lambda));

    /* Local hour angle */
    double gmst = norm360(280.46061837 + 360.98564736629 * n);
    double H    = (norm360(gmst + (double)longitude) * DEG2RAD) - ra;

    /* Azimuth, clockwise from North */
    double phi = (double)latitude * DEG2RAD;
    double az  = atan2(sin(H) * cos(dec),
                       cos(H) * sin(phi) * cos(dec) - sin(dec) * cos(phi));
    az = norm360(az * RAD2DEG + 180.0);

    /* Sun is at 0 on the circle; North is at -az (clockwise) */
    return norm360(-az);
}

/*
 * Returns where North is (0..359°, clockwise) on a circle whose 0° mark points at the Moon.
 * Inputs: UTC time and Gregorian date; longitude in degrees, east positive;
 *         latitude in degrees, north positive.
 */
static double moon_compass(
    int hour, int minute, int second, int year, int month, int day, float latitude, float longitude
) {
    /* 1. Julian Day (UTC) */
    int y = year, m = month;
    if (m <= 2) { y -= 1; m += 12; }
    int A = y / 100;
    int B = 2 - A + A / 4;
    double jd = floor(365.25 * (y + 4716)) + floor(30.6001 * (m + 1)) + day + B - 1524.5
              + (hour + minute / 60.0 + second / 3600.0) / 24.0;
    double d = jd - 2451545.0;
    double T = d / 36525.0;

    /* 2. Moon: ecliptic longitude, latitude, horizontal parallax (deg) */
    double L = 218.32 + 481267.881 * T
             + 6.29 * sind(135.0 + 477198.87 * T) - 1.27 * sind(259.3 - 413335.36 * T)
             + 0.66 * sind(235.7 + 890534.22 * T) + 0.21 * sind(269.9 + 954397.74 * T)
             - 0.19 * sind(357.5 +  35999.05 * T) - 0.11 * sind(186.5 + 966404.03 * T);
    double Bt = 5.13 * sind( 93.3 + 483202.02 * T) + 0.28 * sind(228.2 + 960400.89 * T)
              - 0.28 * sind(318.3 +   6003.15 * T) - 0.17 * sind(217.6 - 407332.21 * T);
    double P = 0.9508 + 0.0518 * cosd(135.0 + 477198.87 * T) + 0.0095 * cosd(259.3 - 413335.36 * T)
             + 0.0078 * cosd(235.7 + 890534.22 * T) + 0.0028 * cosd(269.9 + 954397.74 * T);

    /* 3. Ecliptic -> equatorial */
    double eps = 23.439291 - 0.0130042 * T;
    double xe = cosd(Bt) * cosd(L);
    double ye = cosd(eps) * cosd(Bt) * sind(L) - sind(eps) * sind(Bt);
    double ze = sind(eps) * cosd(Bt) * sind(L) + cosd(eps) * sind(Bt);
    double ra  = atan2(ye, xe) * RAD2DEG;
    double dec = asin(ze) * RAD2DEG;

    /* 4. Sidereal time -> hour angle */
    double gmst = 280.46061837 + 360.98564736629 * d
                + 0.000387933 * T * T - T * T * T / 38710000.0;
    double H = norm360(gmst + longitude - ra);

    /* 5. Topocentric correction (parallax ~1°) */
    double r   = 1.0 / sind(P);                                  /* distance in Earth radii */
    double u   = atan(0.99664719 * tan(latitude * DEG2RAD));
    double rc  = cos(u), rs = 0.99664719 * sin(u);
    double xh  = r * cosd(dec) * cosd(H) - rc;
    double yh  = r * cosd(dec) * sind(H);
    double zh  = r * sind(dec) - rs;
    double Ht  = atan2(yh, xh);                                  /* rad */
    double dct = atan2(zh, sqrt(xh * xh + yh * yh));             /* rad */

    /* 6. Azimuth of the Moon (0 = N, clockwise) */
    double phi = latitude * DEG2RAD;
    double az = norm360(atan2(-cos(dct) * sin(Ht),
                              sin(dct) * cos(phi) - cos(dct) * cos(Ht) * sin(phi)) * RAD2DEG);

    /* 7. North relative to the Moon */
    return norm360(360.0 - az);
}


/* ====================================================================================================================
 *
 * Tests / verification via suncalc.org and mooncalc.org
 * 
 * > cc -std=c11 -Wall -Wextra -Werror -o test_sun_moon_compass test_sun_moon_compass.c -lm
 * > ./sun_moon_compass_test
 *
 * #define SUN_MOON_COMPASS_TEST
 * #include "sun_moon_compass_face.c"
 * 
 * #include <stdio.h>
 * 
 * int main(void) {
 *     printf("Sun:  calc    / correct:\n");
 *     //                                            h , i , s, y   , m , d , lat       , lon           360 - Azimuth
 *     printf("      %6.2f  / %6.2f\n", sun_compass( 6, 34, 0, 2026, 10,  6,  51.61003f,   9.52887f), (360 - 109.38));
 *     printf("      %6.2f  / %6.2f\n", sun_compass(11, 34, 0, 2027,  5, 20,   0.86602f,  35.19293f), (360 - 305.4 ));
 *     printf("      %6.2f  / %6.2f\n", sun_compass(15, 24, 0, 2025,  9, 26,  56.75272f,  93.20074f), (360 - 320.95));
 *     printf("      %6.2f  / %6.2f\n", sun_compass( 6, 14, 0, 2029,  2, 22, -27.05913f, -56.28668f), (360 - 133.90));
 *     printf("      %6.2f  / %6.2f\n", sun_compass( 2, 20, 0, 2028, 11, 21, -28.61346f, 138.04439f), (360 -  20.66));
 * 
 *     printf("\nMoon: calc    / correct:\n");
 *     //                                             h , i , s, y   , m , d , lat       , lon           360 - Azimuth
 *     printf("      %6.2f  / %6.2f\n", moon_compass( 1, 26, 0, 2026,  6, 27,  51.83575f, -93.98685f), (360 - 146.54));
 *     printf("      %6.2f  / %6.2f\n", moon_compass(23, 43, 0, 2027,  1, 11, -29.53527f, -57.60013f), (360 - 272.25));
 *     printf("      %6.2f  / %6.2f\n", moon_compass( 3,  9, 0, 2028,  6, 12,  12.78845f,  -5.20113f), (360 - 146.6 ));
 *     printf("      %6.2f  / %6.2f\n", moon_compass(23, 48, 0, 2029, 11, 17,  48.73315f,  32.76762f), (360 - 262.69));
 *     printf("      %6.2f  / %6.2f\n", moon_compass(14, 31, 0, 2030,  9, 20,  38.46065f, 139.64262f), (360 -  67.67));
 * 
 *     return 0;
 * }
 *
 * Result:
 *
 * Sun:  calc / correct:
 *       250  / 250.62
 *        55  /  54.60
 *        39  /  39.05
 *       226  / 226.10
 *       339  / 339.34
 *
 * Moon: calc / correct:
 *       213  / 213.46
 *        88  /  87.75
 *       213  / 213.40
 *        97  /  97.31
 *       292  / 292.33
 */
