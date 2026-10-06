/* SPDX-License-Identifier: MIT */

/*
 * MIT License
 *
 * Copyright © 2021-2023 Joey Castillo <joeycastillo@utexas.edu> <jose.castillo@gmail.com>
 * Copyright © 2022 David Keck <davidskeck@users.noreply.github.com>
 * Copyright © 2022 TheOnePerson <a.nebinger@web.de>
 * Copyright © 2023 Jeremy O'Brien <neutral@fastmail.com>
 * Copyright © 2023 Mikhail Svarichevsky <3@14.by>
 * Copyright © 2023 Wesley Aptekar-Cassels <me@wesleyac.com>
 * Copyright © 2024 Matheus Afonso Martins Moreira <matheus.a.m.moreira@gmail.com>
 * Copyright © 2026 Chris Gillings <git@ned-ludd.com>
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
#include "sun_transits_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_common_display.h"
#include <math.h>
#include "filesystem.h"

#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

/*
 * display segment code to display triple bar at Daynum 0
 * because this is the signifier/logo of this face
 *     _
 *     -
 *     -

*/

typedef enum {
    A, B, C, D, E, F, G
} segment_t;

static void set_segment_at_position(segment_t segment, uint8_t position) {
    digit_mapping_t segmap;
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        segmap = Custom_LCD_Display_Mapping[position];
    } else {
        segmap = Classic_LCD_Display_Mapping[position];
    }
    const uint8_t com_pin = segmap.segment[segment].address.com;
    const uint8_t seg = segmap.segment[segment].address.seg;
    watch_set_pixel(com_pin, seg);
}

static movement_location_t load_location_from_filesystem() {
    movement_location_t location = {0};

    filesystem_read_file("location.u32", (char *) &location.reg, sizeof(movement_location_t));

    return location;
}

static uint8_t hemisphere() {
    // the hemisphere determines the six-month offset for named transits

    // return +1 for northern hemisphere
    // return -1 for southern hemisphere
    // return 0 for equator??

    movement_location_t movement_location;
    movement_location = load_location_from_filesystem();

    if (movement_location.reg == 0 || movement_location.bit.latitude > 0) {
       return 1;
    } else if (movement_location.bit.latitude < 0) {
       return -1;
    }
    return 0;
}

static void watch_display_transit_event_text(double transit_angle) {

    // display watch face text according to the transit angle
   // so that the viewer can tell which transit event they're looking at

    char custom_lcd[3+1];
    char classic_lcd[2+1];
    char daynum[2+1];
    uint16_t t_angle = transit_angle;

    // southern hemisphere (or equator) has transit names shifted by six months
    if (hemisphere() < 0) {
        t_angle = ((int)transit_angle + 180) % 360;
    }
    /*
    Transit angles and their names
    0     March equinox "ME" "VE"
    45    cross-quarter "CQ" "Be" Beltane
    90    June solstice "JS" "SS" "MS"
    135   cross-quarter "CQ" "Lu" Lughnasadh "La" (Lammas)
    180   September equinox "SE"
    225   cross-quarter "CQ" "Sa"  Samhain
    270   December solstice "DS" "WS" "MW"
    315   cross-quarter "CQ" "Im"  Imbolc
    360   March equinox again
    */
    // transit type is displayed in the Day digits
    // for both LCD types
    char equinox[2+1] = "=E";
    char solstice[2+1] = "=S";
    char quarter[2+1] = "=q";

    switch(t_angle) {
        case 0:
        case 360:
            strcpy(custom_lcd,"Aut");
            strcpy(classic_lcd,"Au");
            strcpy(daynum,equinox);
            break;
        case 45:
            strcpy(custom_lcd,"Sam");
            strcpy(classic_lcd,"Sa");
            strcpy(daynum,quarter);
            break;
        case 90:
            strcpy(custom_lcd,"Win");
            strcpy(classic_lcd,"Wi");
            strcpy(daynum,solstice);
            break;
        case 135:
            strcpy(custom_lcd,"Imb");
            strcpy(classic_lcd,"Ib");
            strcpy(daynum,quarter);
            break;
        case 180:
            strcpy(custom_lcd,"Spr");
            strcpy(classic_lcd,"S ");
            strcpy(daynum,equinox);
            break;
        case 225:
            strcpy(custom_lcd,"BeL");
            strcpy(classic_lcd,"Be");
            strcpy(daynum,quarter);
            break;
        case 270:
            strcpy(custom_lcd,"SUM");
            strcpy(classic_lcd,"Su");
            strcpy(daynum,solstice);
            break;
        case 315:
            strcpy(custom_lcd,"LUg");
            strcpy(classic_lcd,"Lu");
            strcpy(daynum,quarter);
            break;
        default:
            strcpy(custom_lcd,"ERR");
            strcpy(classic_lcd,"ER");
            strcpy(daynum,"");
            break;
    }
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, custom_lcd, classic_lcd);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, daynum);
    // display triple bar in Day digit 2
    // because it signifies high/middle/low for the range of sun transits to E/S/q
    set_segment_at_position(A, 2);
    set_segment_at_position(D, 2);
    set_segment_at_position(G, 2);
}

/* copied in from astrolib.c */

//Special "Math.floor()" function used by convertDateToJulianDate()
static double _astro_special_floor(double d) {
    if(d > 0) {
        return floor(d);
    }
    return floor(d) - 1;
}

static double convert_date_to_julian_date(watch_date_time_t date_time) {

    uint16_t year = date_time.unit.year + WATCH_RTC_REFERENCE_YEAR;
    uint8_t month = date_time.unit.month;
    uint8_t day = date_time.unit.day;
    uint8_t hour = date_time.unit.hour;
    uint8_t minute = date_time.unit.minute;
    uint8_t second = date_time.unit.second;

    // copied from astrolib.c */

    if (month < 3){
        year = year - 1;
        month = month + 12;
    }

    double b = 0;
    if (!(year < 1582 || (year == 1582 && (month < 10 || (month == 10 && day < 5))))) {
        double a = _astro_special_floor(year / 100.0);
        b = 2 - a + _astro_special_floor(a / 4.0);
    }

    double jd = _astro_special_floor(365.25 * (year + 4716)) + _astro_special_floor(30.6001 * (month + 1)) + day + b - 1524.5;
    jd += hour / 24.0;
    jd += minute / 24.0 / 60.0;
    jd += second / 24.0 / 60.0 / 60.0;

    return jd;
}

static watch_date_time_t julian_date_to_date_time(double jd)
{
    // function provided by ChatGPT

    watch_date_time_t dt = { .reg = 0 };

    /*
     * Julian Date starts at noon, so adding 0.5 makes the
     * integer part correspond to midnight.
     */
    double jd_midnight = jd + 0.5;

    int64_t z = (int64_t)floor(jd_midnight);
    double f = jd_midnight - (double)z;

    /*
     * Convert the Julian day number to Gregorian Y/M/D.
     * Valid for all dates in the Sensor Watch's range.
     */
    int64_t a = z + 32044;
    int64_t b = (4 * a + 3) / 146097;
    int64_t c = a - (146097 * b) / 4;
    int64_t d = (4 * c + 3) / 1461;
    int64_t e = c - (1461 * d) / 4;
    int64_t m = (5 * e + 2) / 153;

    int day   = (int)(e - (153 * m + 2) / 5 + 1);
    int month = (int)(m + 3 - 12 * (m / 10));
    int year  = (int)(100 * b + d - 4800 + m / 10);

    /*
     * Convert fractional day to time.
     * Round rather than truncate, since the astronomical
     * calculation will generally produce fractional seconds.
     */
    int total_seconds = (int)floor(f * 86400.0 + 0.5);

    /*
     * Rounding can occasionally produce exactly 86400.
     * In that case the date belongs to the following day.
     */
    if (total_seconds >= 86400) {
        total_seconds = 0;

        /*
         * Advance the Gregorian date by one day.
         * We can do this without another JD conversion.
         */
        day++;

        int days_in_month;

        switch (month) {
        case 2:
            days_in_month =
                ((year % 4 == 0 && year % 100 != 0) ||
                 (year % 400 == 0)) ? 29 : 28;
            break;

        case 4:
        case 6:
        case 9:
        case 11:
            days_in_month = 30;
            break;

        default:
            days_in_month = 31;
            break;
        }

        if (day > days_in_month) {
            day = 1;
            month++;

            if (month > 12) {
                month = 1;
                year++;
            }
        }
    }

    dt.unit.second = total_seconds % 60;
    dt.unit.minute = (total_seconds / 60) % 60;
    dt.unit.hour   = total_seconds / 3600;

    dt.unit.day    = day;
    dt.unit.month  = month;

    /*
     * Sensor Watch represents 2020 as year 0.
     */
    dt.unit.year   = year - WATCH_RTC_REFERENCE_YEAR;

    return dt;
}

static watch_date_time_t convert_julian_date_to_gregorian_date(double jd) {

    // algorithm provided by ChatGPT

    watch_date_time_t gregorian_date;
    uint32_t l, n, i, j, d, m, y;

    l = jd + 68569;
    n = (4*l) / 146097;
    l = l - (146097*n + 3)/4;
    i = (4000*(l+1))/1461001;
    l = l - (1461*i)/4 + 31;
    j = (80*l)/2447;
    d = l - (2447*j)/80;
    l = j/11;
    m = j + 2 - 12*l;
    y = 100*(n-49) + i + l;

    gregorian_date.unit.year  = y;
    gregorian_date.unit.month = m;
    gregorian_date.unit.day   = d;

    return gregorian_date;
}

#define M_PI 3.1415926535897932384

static double degrees_to_radians(double degrees) {
    return degrees * (M_PI / 180.0);
}
static double dsin(double degrees) {
    return sin(degrees_to_radians(degrees));
}

static double solar_longitude_2(double jd) {

    // calculate **approximate** solar longitude for a Julian date

    // (calculation offered by ChatGPT)

    double T = (jd - 2451545.0) / 36525.0;

    double L0 = 280.46646
       + 36000.76983 * T
       + 0.0003032 * T*T;

    double M = 357.52911
      + 35999.05029 * T
      - 0.0001537 * T*T;

    double C = (1.914602 - 0.004817*T - 0.000014*T*T) * dsin(M)
      + (0.019993 - 0.000101*T) * dsin(2*M)
      + 0.000289 * dsin(3*M);

    double longitude = L0 + C;
    return longitude;
}

static double true_solar_longitude(double jd) {

    // calculate a reasonably accurate solar longitude for a Julian date
    // (from NOAA Solar Calculations spreadsheet formulas)
    //   https://gml.noaa.gov/grad/solcalc/calcdetails.html

    // Julian century
    double G = (jd - 2451545)/36525;
    //  Geom Mean Long Sun (deg)
    double I = fmod(280.46646 + G *(36000.76983 + G * 0.0003032),360);
    // Geom Mean Anom Sun (deg)
    double J = 357.52911 + G * (35999.05029 - 0.0001537 * G);
    // Eccent Earth Orbit
    double K = 0.016708634 - G * (0.000042037 + 0.0000001267 * G);
    // Sun Eq of Ctr
    double L = dsin(J) * (1.914602 - G * (0.004817 + 0.000014 * G ))
               + dsin(2*J) * (0.019993 - 0.000101 * G) + dsin(3*J) * 0.000289;
    // Sun True Long (deg)
    double M = I + L;

    return M;
}

static double solar_longitude_usmil(double jd) {

    // calculate **approximate** solar longitude for a Julian date

    // calculation taken from US Navy site
    //   https://aa.usno.navy.mil/faq/sun_approx

    // adjust jd to j2000
    double jd_2000 = jd - 2451545.0;
    double g = fmod(357.529 + 0.98560028 * jd_2000,360.0);
    double q = fmod(280.459 + 0.98564736 * jd_2000,360.0);
    double longitude = fmod(q + 1.915 * dsin(g) + 0.020 * dsin(2*g),360.0);
    return longitude;
}

static double find_solar_transit(double target_longitude, double starting_julian_date, double increment) {

    // find the Julian date for a target solar longitude
    // by repeatedly incrementing from starting_julian_date
    // until we find the target

    // (code provided by ChatGPT)

    double previous_longitude = true_solar_longitude(starting_julian_date);
    double longitude;

    while (1) {
        starting_julian_date += increment;
        longitude = true_solar_longitude(starting_julian_date);

        if (longitude >= target_longitude) {
            // Linear interpolation between previous_longitude and longitude
            double fraction = (target_longitude - previous_longitude) / (longitude - previous_longitude);

            starting_julian_date -= increment * (1.0 - fraction);
            return starting_julian_date;
        }

        previous_longitude = longitude;
    }
}

// static void sun_transits_display_all(watch_date_time_t date_time) {
static double sun_transits_display_all(double julian_now_date_exact) {
    char buf[8 + 1];
    int net_degrees;
    int net_seconds;
    float net_time;

    double jd, longitude, julian_date;
    int jd_frac,jd_int;

    char logbuf[48];
/*
    // testing each transit
    julian_now_date_exact = 2461310; // 225 deg
    julian_now_date_exact = 2461360; // 270 deg
    julian_now_date_exact = 2461410; // 315 deg
    julian_now_date_exact = 2461450; // 360 deg
    julian_now_date_exact = 2461500; // 45 deg
    julian_now_date_exact = 2461560; // 90 deg
    julian_now_date_exact = 2461590; // 135 deg
*/

#if __EMSCRIPTEN__
    snprintf( logbuf, sizeof(logbuf), "Julian date today = %f", julian_now_date_exact);
    emscripten_log(EM_LOG_CONSOLE, logbuf);
#endif

    // convert current Julian date to YMD-HMS to check calculation
    // watch_date_time_t now_date_check = julian_date_to_date_time(julian_now_date_exact);

    // get current solar longitude
    double current_solar_longitude = true_solar_longitude(julian_now_date_exact);

#if __EMSCRIPTEN__
    snprintf( logbuf, sizeof(logbuf), "Current solar longitude = %f", current_solar_longitude);
    emscripten_log(EM_LOG_CONSOLE, logbuf);
#endif

    // calculate next transit longitude (multiple of 45)
    double next_transit_longitude = (int)(current_solar_longitude/45+1) * 45;

#if __EMSCRIPTEN__
    snprintf( logbuf, sizeof(logbuf), "Next transit longitude = %f", next_transit_longitude);
    emscripten_log(EM_LOG_CONSOLE, logbuf);
#endif

    // find Julian date of next transit
    double next_transit_julian_date = find_solar_transit(next_transit_longitude,julian_now_date_exact,0.01);

#if __EMSCRIPTEN__
    snprintf( logbuf, sizeof(logbuf), "Julian date next transit = %f", next_transit_julian_date);
    emscripten_log(EM_LOG_CONSOLE, logbuf);
#endif

/*
#if __EMSCRIPTEN__
    // check that next transit's calculated longitude is as expected
    snprintf( logbuf, sizeof(logbuf), "Next transit longitude check = %f", true_solar_longitude(next_transit_julian_date));
    emscripten_log(EM_LOG_CONSOLE, logbuf);
#endif
*/

    // convert next transit Julian date to YMD-HMS
    watch_date_time_t next_transit_date = julian_date_to_date_time(next_transit_julian_date);

    // apply UTC offset to date displayed
    uint32_t timestamp = watch_utility_date_time_to_unix_time(next_transit_date, 0);
    timestamp += (double)movement_get_timezone_offset_for_date(next_transit_date);
    next_transit_date = watch_utility_date_time_from_unix_time(timestamp, 0);

#if __EMSCRIPTEN__
    snprintf( logbuf, sizeof(logbuf), 
        "Next transit date = %4d-%02d-%02d %02d:%02d:%02d",
        next_transit_date.unit.year+WATCH_RTC_REFERENCE_YEAR,
        next_transit_date.unit.month,
        next_transit_date.unit.day,
        next_transit_date.unit.hour,
        next_transit_date.unit.minute,
        next_transit_date.unit.second);
    emscripten_log(EM_LOG_CONSOLE, logbuf);
#endif

    // display next transit date on watch face
    snprintf(
        buf,
        sizeof(buf),
        "  %02d%.02d%02d",
        next_transit_date.unit.day,
        next_transit_date.unit.month,
        next_transit_date.unit.year+20
    );

    watch_set_decimal_if_available();
    watch_display_transit_event_text(next_transit_longitude);
    watch_display_text(WATCH_POSITION_BOTTOM, buf + 2);

    // show Bell indicator if today is the next transit
    watch_date_time_t now_date_time = movement_get_local_date_time();
    if (now_date_time.unit.year == next_transit_date.unit.year &&
        now_date_time.unit.month == next_transit_date.unit.month &&
        now_date_time.unit.day == next_transit_date.unit.day) {
        watch_set_indicator(WATCH_INDICATOR_BELL);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_BELL);
    }

    return next_transit_julian_date;
}

static double sun_transits_display_sun_transits(double julian_date) {
    return sun_transits_display_all(julian_date);
}

void sun_transits_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(sun_transits_state_t));
        sun_transits_state_t *state = (sun_transits_state_t *) *context_ptr;
        state->time_signal_enabled = false;
        state->watch_face_index = watch_face_index;
    }
}

void sun_transits_face_activate(void *context) {
    sun_transits_state_t *state = (sun_transits_state_t *) context;

    // sun_transits_indicate_time_signal(false);

    // watch_set_colon();

    // this ensures that none of the timestamp fields will match, so we can re-render them all.
    state->date_time.previous.reg = 0xFFFFFFFF;
}

bool sun_transits_face_loop(movement_event_t event, void *context) {
    sun_transits_state_t *state = (sun_transits_state_t *) context;
    watch_date_time_t current;
    double current_julian_date;
    static double last_transit_event_julian_date;
    char logbuf[48];

    switch (event.event_type) {
        case EVENT_LOW_ENERGY_UPDATE:
            break;
        // case EVENT_TICK:
        case EVENT_ACTIVATE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "TRA", "TR");

            // start with current date_time
            current = movement_get_utc_date_time();

            // get current Julian date
            current_julian_date = convert_date_to_julian_date(current);

#if __EMSCRIPTEN__
    snprintf( logbuf, sizeof(logbuf), "Starting Julian date today = %f", current_julian_date);
    emscripten_log(EM_LOG_CONSOLE, logbuf);
#endif

            // save last event's Julian date
            last_transit_event_julian_date = sun_transits_display_sun_transits(current_julian_date);

            state->date_time.previous = current;

            break;

        case EVENT_LIGHT_BUTTON_UP:

#if __EMSCRIPTEN__
    snprintf( logbuf, sizeof(logbuf), "Previous Julian date target = %f", last_transit_event_julian_date-SUN_TRANSIT_MAXIMUM_DAY_GAP-2);
    emscripten_log(EM_LOG_CONSOLE, logbuf);
#endif
            // jump back to just before the previous transit
            last_transit_event_julian_date = sun_transits_display_sun_transits(last_transit_event_julian_date-SUN_TRANSIT_MAXIMUM_DAY_GAP-2);
            break;

        case EVENT_ALARM_BUTTON_UP:

#if __EMSCRIPTEN__
    snprintf( logbuf, sizeof(logbuf), "Next Julian date target = %f", last_transit_event_julian_date+SUN_TRANSIT_MINIMUM_DAY_GAP);
    emscripten_log(EM_LOG_CONSOLE, logbuf);
#endif
            // jump forward to just before the next transit
            last_transit_event_julian_date = sun_transits_display_sun_transits(last_transit_event_julian_date+SUN_TRANSIT_MINIMUM_DAY_GAP);
            break;

        case EVENT_ALARM_LONG_PRESS:
            break;
        case EVENT_BACKGROUND_TASK:
            // uncomment this line to snap back to the sun_transits face when the hour signal sounds:
            // movement_move_to_face(state->watch_face_index);
            // movement_play_signal();
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void sun_transits_face_resign(void *context) {
    (void) context;
}

movement_watch_face_advisory_t sun_transits_face_advise(void *context) {
    movement_watch_face_advisory_t retval = { 0 };
    sun_transits_state_t *state = (sun_transits_state_t *) context;

    if (state->time_signal_enabled) {
        watch_date_time_t date_time = movement_get_utc_date_time();
        retval.wants_background_task = date_time.unit.minute == 0;
    }

    return retval;
}
