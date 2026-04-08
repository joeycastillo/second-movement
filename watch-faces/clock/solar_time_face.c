/*
 * MIT License
 *
 * Copyright (c) 2025 Raffael Mancini
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
 * Solar time formulas follow the notation from:
 *   https://www.pveducation.org/pvcdrom/properties-of-sunlight/solar-time
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "solar_time_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "filesystem.h"

#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/* ---------------------------------------------------------------------------
 * Solar time math (pveducation.org notation)
 * ---------------------------------------------------------------------------
 *
 *  LSTM = 15 * ΔTUTC                          [degrees]
 *  B    = (360 / 365) * (d - 81)              [degrees]  d = day-of-year
 *  EoT  = 9.87*sin(2B) - 7.53*cos(B)
 *           - 1.5*sin(B)                      [minutes]
 *  TC   = 4 * (Longitude - LSTM) + EoT        [minutes]
 *  LST  = LT + TC/60                          [hours]
 *  HRA  = 15 * (LST - 12)                     [degrees]
 * ---------------------------------------------------------------------------
 */

static movement_location_t _load_location(void) {
    movement_location_t loc = {0};
    filesystem_read_file("location.u32", (char *)&loc.reg, sizeof(loc.reg));
    return loc;
}

/* Compute and cache EoT and TC. Call when d != state->last_calc_d. */
static void _compute_daily(solar_time_state_t *state, uint16_t d) {
    /* LSTM — movement_get_current_timezone_offset() returns seconds from UTC */
    float delta_T_UTC = (float)movement_get_current_timezone_offset() / 3600.0f;
    float LSTM = 15.0f * delta_T_UTC;

    movement_location_t loc = _load_location();
    float longitude = (float)(int16_t)loc.bit.longitude / 100.0f;

    /* B in radians for sinf/cosf */
    float B = (360.0f / 365.0f) * ((float)d - 81.0f) * ((float)M_PI / 180.0f);

    state->EoT = 9.87f * sinf(2.0f * B) - 7.53f * cosf(B) - 1.5f * sinf(B);
    state->TC  = 4.0f * (longitude - LSTM) + state->EoT;
    state->last_calc_d = d;
}

/* Recompute if the day-of-year has rolled over. Returns current d. */
static uint16_t _maybe_recompute(solar_time_state_t *state, watch_date_time_t dt) {
    uint16_t d = watch_utility_days_since_new_year(
        (uint16_t)(dt.unit.year + WATCH_RTC_REFERENCE_YEAR),
        dt.unit.month,
        dt.unit.day
    );
    if (d != state->last_calc_d && _load_location().reg != 0) {
        _compute_daily(state, d);
    }
    return d;
}

/* LST as total seconds since midnight (0..86399).
 * LST = LT + TC/60  =>  in seconds: LT_sec + TC*60 */
static int32_t _lst_seconds(watch_date_time_t dt, float TC) {
    int32_t lt  = (int32_t)dt.unit.hour * 3600
                + (int32_t)dt.unit.minute * 60
                + (int32_t)dt.unit.second;
    int32_t tc  = (int32_t)(TC * 60.0f);
    return ((lt + tc) % 86400 + 86400) % 86400;
}

static void _update_display(solar_time_state_t *state, watch_date_time_t dt) {
    char bottom[7];

    if (_load_location().reg == 0) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "SOL", "SO");
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
        watch_display_text(WATCH_POSITION_BOTTOM, "no Loc");
        watch_clear_colon();
        return;
    }

    switch (state->mode) {

        case SOLAR_TIME_MODE_LST: {
            int32_t s = _lst_seconds(dt, state->TC);
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "SOL", "SO");
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "Ar");
            sprintf(bottom, "%02d%02d%02d",
                    (int)(s / 3600), (int)((s % 3600) / 60), (int)(s % 60));
            watch_set_colon();
            break;
        }

        case SOLAR_TIME_MODE_NOON: {
            /* Solar noon: moment when LST = 12:00 → LT_noon = 12h - TC/60 */
            int32_t s = (int32_t)(( 12.0f - state->TC / 60.0f) * 3600.0f);
            s = ((s % 86400) + 86400) % 86400;
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "NOO", "NO");
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "n ");
            sprintf(bottom, "%02d%02d  ", (int)(s / 3600), (int)((s % 3600) / 60));
            watch_set_colon();
            break;
        }

        case SOLAR_TIME_MODE_HRA: {
            /* HRA = 15 * (LST - 12); negative = morning, positive = afternoon */
            int32_t s   = _lst_seconds(dt, state->TC);
            int16_t hra = (int16_t)roundf(15.0f * ((float)s / 3600.0f - 12.0f));
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "HrA", "Hr");
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "n ");
            sprintf(bottom, "%+4d  ", (int)hra);
            watch_clear_colon();
            break;
        }

        default:
            return;
    }

    watch_display_text(WATCH_POSITION_BOTTOM, bottom);
}

/* ---- Movement callbacks -------------------------------------------------- */

void solar_time_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(solar_time_state_t));
        memset(*context_ptr, 0, sizeof(solar_time_state_t));
        /* last_calc_d == 0 guarantees recomputation on first tick */
    }
}

void solar_time_face_activate(void *context) {
    solar_time_state_t *state = (solar_time_state_t *)context;

#if __EMSCRIPTEN__
    /* In the simulator the browser exposes lat/lon as JS globals.
     * Write them to location.u32 if not already set. */
    int16_t browser_lat = EM_ASM_INT({ return lat; });
    int16_t browser_lon = EM_ASM_INT({ return lon; });
    if (browser_lat || browser_lon) {
        movement_location_t browser_loc = {0};
        filesystem_read_file("location.u32", (char *)&browser_loc.reg, sizeof(browser_loc.reg));
        if (browser_loc.reg == 0) {
            browser_loc.bit.latitude  = browser_lat;
            browser_loc.bit.longitude = browser_lon;
            filesystem_write_file("location.u32", (char *)&browser_loc.reg, sizeof(browser_loc.reg));
        }
    }
#endif

    /* Force recompute on activation: timezone or location may have changed */
    state->last_calc_d = 0;
    watch_date_time_t dt = movement_get_local_date_time();
    _maybe_recompute(state, dt);
}

bool solar_time_face_loop(movement_event_t event, void *context) {
    solar_time_state_t *state = (solar_time_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK: {
            watch_date_time_t dt = movement_get_local_date_time();
            _maybe_recompute(state, dt);
            _update_display(state, dt);
            break;
        }

        case EVENT_ALARM_BUTTON_UP:
            state->mode = (solar_time_mode_t)((state->mode + 1) % SOLAR_TIME_NUM_MODES);
            {
                watch_date_time_t dt = movement_get_local_date_time();
                _update_display(state, dt);
            }
            break;

        case EVENT_LOW_ENERGY_UPDATE: {
            if (!watch_sleep_animation_is_running()) watch_start_sleep_animation(1000);
            watch_date_time_t dt = movement_get_local_date_time();
            _maybe_recompute(state, dt);
            _update_display(state, dt);
            break;
        }

        case EVENT_TIMEOUT:
            state->mode = SOLAR_TIME_MODE_LST;
            if (_load_location().reg == 0) movement_move_to_face(0);
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void solar_time_face_resign(void *context) {
    solar_time_state_t *state = (solar_time_state_t *)context;
    state->mode = SOLAR_TIME_MODE_LST;
    watch_clear_colon();
}
