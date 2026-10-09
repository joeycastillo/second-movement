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
#include <math.h>
#include "sidereal_time_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_common_display.h"
#include "filesystem.h"

#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

// 2.4 volts seems to offer adequate warning of a low battery condition?
// refined based on user reports and personal observations; may need further adjustment.
#ifndef SIDEREAL_TIME_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD
#define SIDEREAL_TIME_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD 2400
#endif

static void sidereal_time_indicate(watch_indicator_t indicator, bool on) {
    if (on) {
        watch_set_indicator(indicator);
    } else {
        watch_clear_indicator(indicator);
    }
}

static void sidereal_time_indicate_low_available_power(sidereal_time_state_t *state) {
    // Set the low battery indicator if battery power is low
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        // interlocking arrows imply "exchange" the battery.
        sidereal_time_indicate(WATCH_INDICATOR_ARROWS, state->battery_low);
    } else {
        // LAP indicator on classic LCD is an adequate fallback.
        sidereal_time_indicate(WATCH_INDICATOR_LAP, state->battery_low);
    }
}

static void sidereal_time_check_battery_periodically(sidereal_time_state_t *state, watch_date_time_t date_time) {
    // check the battery voltage once a day
    if (date_time.unit.day == state->last_battery_check) { return; }

    state->last_battery_check = date_time.unit.day;

    uint16_t voltage = watch_get_vcc_voltage();

    state->battery_low = voltage < SIDEREAL_TIME_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD;

    sidereal_time_indicate_low_available_power(state);
}

static movement_location_t load_location_from_filesystem() {
    movement_location_t location = {0};

    filesystem_read_file("location.u32", (char *) &location.reg, sizeof(movement_location_t));

    return location;
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

#define M_PI 3.1415926535897932384

static double degrees_to_radians(double degrees) {
    return degrees * (M_PI / 180.0);
}
static double dcos(double degrees) {
    return cos(degrees_to_radians(degrees));
}
static double dsin(double degrees) {
    return sin(degrees_to_radians(degrees));
}

static double get_GMST(double jd, watch_date_time_t utc_dt) {
    double DTT = jd - 2451545.0;

    double H = (double)utc_dt.unit.hour + (double)utc_dt.unit.minute / 60 + (double)utc_dt.unit.second / 3600;

    // Also compute T the number of TT centuries since the year 2000.

    double T = DTT / 36525;

    // Then the Greenwich mean sidereal time in hours is:

    // double GMST = mod (6.697375 + 0.065707485828 DUT + 1.0027379 H + 0.0854103 T + 0.0000258 T2, 24) h

    // Or, assuming JDTT = JDUT :

    // this alternative formula can be used for approximate GMST at 0h UT
    // with a loss of precision of 0.1 second per century:

    double GMST = fmod (18.697375 + 24.065709824279 * DTT, 24);

    // GAST is obtained by adding a correction to the Greenwich mean sidereal time computed above. 

    // longitude of ascending noe of the moon
    double omega = 125.04 - 0.052954 * DTT;
    // Mean Longitude of the Sun
    double L = 280.47 + 0.98565 * DTT;
    // obliquity
    double epsilon = 23.4393 - 0.0000004 * DTT;
    // nutation in longitude
    double delta = -0.000319 * dsin(omega) - 0.000024 * dsin(2*L);
    // nutation in right ascension or the equation of the equinoxes
    double eqeq = delta * dcos(epsilon);
    // Greenwich Apparent Sidereal Time
    double GAST = GMST + eqeq;

    GMST = GAST;

#if __EMSCRIPTEN__
    char logbuf[48];
    sprintf(logbuf, "Date time %02d:%02d:%02d -> GMST %f", utc_dt.unit.hour, utc_dt.unit.minute, utc_dt.unit.second, GMST);
    emscripten_log(EM_LOG_CONSOLE, logbuf);
    sprintf(logbuf, "eqeq %f, GAST %f", eqeq, GAST);
    emscripten_log(EM_LOG_CONSOLE, logbuf);
#endif

    return GMST;
}

static void sidereal_time_display_all(sidereal_time_state_t *state, watch_date_time_t date_time) {
    char buf[8 + 1];

    watch_date_time_t rtc_date_time = watch_rtc_get_date_time();

    double jd = convert_date_to_julian_date(rtc_date_time);
    double gmst = get_GMST(jd,rtc_date_time);

    movement_location_t movement_location;
    // if (state->longLatToUse == 0 || _location_count <= 1)
        movement_location = load_location_from_filesystem();
    // else{
        // movement_location.bit.longitude = longLatPresets[state->longLatToUse].longitude;
    // }
    double lon = (double)movement_location.bit.longitude / 100.0;

    double lst = gmst + (lon / 15.0);
    while (lst > 24) { lst -= 24; }

    uint8_t sid_hour = (int)lst;
    double minute_second = (lst - sid_hour)*60;
    uint8_t sid_minute = (int)minute_second;
    uint8_t sid_second = (int)((minute_second - sid_minute)*60);

#if __EMSCRIPTEN__
    char logbuf[50];
    sprintf(logbuf, "GMST %f + long %f = lst %f", gmst, lon/15, lst);
    emscripten_log(EM_LOG_CONSOLE, logbuf);
    sprintf(logbuf, "");
    sprintf(logbuf, "Sidereal = %2d:%02d:%02d", sid_hour, sid_minute, sid_second);
    emscripten_log(EM_LOG_CONSOLE, logbuf);
#endif

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "Sid", "Sd");
    sprintf(buf, "");
    sprintf( buf, "%2d%02d%02d", sid_hour, sid_minute, sid_second);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static void sidereal_time_display_sidereal_time(sidereal_time_state_t *state, watch_date_time_t current) {
    sidereal_time_display_all(state, current);
}

void sidereal_time_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(sidereal_time_state_t));
        memset(*context_ptr, 0, sizeof(sidereal_time_state_t));
    }
}


void sidereal_time_face_activate(void *context) {
    sidereal_time_state_t *state = (sidereal_time_state_t *) context;

    // sidereal_time_indicate_time_signal(false);

    watch_set_colon();

    // this ensures that none of the timestamp fields will match, so we can re-render them all.
    state->date_time.previous.reg = 0xFFFFFFFF;
}

bool sidereal_time_face_loop(movement_event_t event, void *context) {
    sidereal_time_state_t *state = (sidereal_time_state_t *) context;
    watch_date_time_t current;

    switch (event.event_type) {
        case EVENT_LOW_ENERGY_UPDATE:
            break;
        case EVENT_TICK:
        case EVENT_ACTIVATE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "Sid", "Sd");
            current = movement_get_utc_date_time();

            sidereal_time_display_sidereal_time(state, current);

            sidereal_time_check_battery_periodically(state, current);

            state->date_time.previous = current;

            break;
        case EVENT_ALARM_LONG_PRESS:
            break;
        case EVENT_BACKGROUND_TASK:
            // uncomment this line to snap back to the sidereal_time face when the hour signal sounds:
            // movement_move_to_face(state->watch_face_index);
            // movement_play_signal();
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void sidereal_time_face_resign(void *context) {
    (void) context;
}

movement_watch_face_advisory_t sidereal_time_face_advise(void *context) {
    movement_watch_face_advisory_t retval = { 0 };
    sidereal_time_state_t *state = (sidereal_time_state_t *) context;

    if (state->time_signal_enabled) {
        watch_date_time_t date_time = movement_get_utc_date_time();
        retval.wants_background_task = date_time.unit.minute == 0;
    }

    return retval;
}
