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
 */

#include "planetary_face.h" 
// #include "sunriset.h"
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

// map of the chaldean order numbers to the planets and abbreviations
static const struct planet_names {
    const char *name;
    const char *abbreviation;
} planet_names[] = {
    {"Satur", "SA"},
    {"Jupit", "JU"},
    {"Mars ", "MA"},
    {"Sun  ", "SU"},
    {"Venus", "VE"},
    {"Mercu", "ME"},
    {"Moon ", "MO"}
};



// Prototype for __sunriset__
int __sunriset__(int year, int month, int day, double lon, double lat, double altit, int upper_limb, double *rise, double *set);

// Function prototypes
static void calculate_planetary_hour(planetary_state_t *state);
static void calculate_astrological_sign(planetary_state_t *state);

// Function to set up the planetary face, allocating memory for the context
void planetary_face_setup(__attribute__((unused)) uint8_t watch_face_index, void **context_ptr) {
    printf("[DEBUG] Setting up planetary face\n");
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(planetary_state_t));
        if (*context_ptr == NULL) {
            printf("[ERROR] Memory allocation failed for planetary face\n");
            return;
        }
        memset(*context_ptr, 0, sizeof(planetary_state_t));
        printf("[DEBUG] Context initialized\n");
    }
}

// Function to activate the planetary face, initializing planetary hour and zodiac sign
void planetary_face_activate(void *context) {
    printf("[DEBUG] Activating planetary face\n");
    planetary_state_t *state = (planetary_state_t *)context;
    calculate_planetary_hour(state);
    calculate_astrological_sign(state);
    printf("[DEBUG] Planetary hour: %d, Zodiac sign: %d\n", state->current_planetary_hour, state->current_zodiac_sign);
}

// Main loop for the planetary face, handling events and updating the display
bool planetary_face_loop(movement_event_t event, void *context) {
    printf("[DEBUG] Event type: %d\n", event.event_type);
    planetary_state_t *state = (planetary_state_t *)context;

    // Check if the context is null to avoid dereferencing a null pointer
    if (state == NULL) {
        printf("[ERROR] Context is NULL\n");
        watch_display_text(WATCH_POSITION_TOP, "Error");
        watch_display_text(WATCH_POSITION_BOTTOM, "Error");
        return false;
    }

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            calculate_planetary_hour(state); // Recalculate planetary hour on activation
            calculate_astrological_sign(state); // Recalculate zodiac sign on activation
            printf("[DEBUG] Recalculated planetary hour: %d, Zodiac sign: %d\n", state->current_planetary_hour, state->current_zodiac_sign);
            break;

        case EVENT_ALARM_LONG_PRESS:
            if (state->page == 0) {
            if (state->longLatToUse != 0) {
                state->longLatToUse = 0;
                _sunrise_sunset_face_update(state);
                break;
            }
                state->page++;
                state->active_digit = 0;
                watch_clear_display();
                movement_request_tick_frequency(4);
                _update_location_settings_display(event, context);
            }
            else {
                state->active_digit = 0;
                state->page = 0;
                _update_location_register(state);
                _sunrise_sunset_face_update(state);
            }
            break;
        case EVENT_TIMEOUT:
            if (load_location_from_filesystem().reg == 0) {
                // if no location set, return home
                movement_move_to_face(0);
            } else if (state->page || state->rise_index) {
                // otherwise on timeout, exit settings mode and return to the next sunrise or sunset
                state->page = 0;
                state->rise_index = 0;
                movement_request_tick_frequency(1);
                _sunrise_sunset_face_update(state);
            }
            break;
        case EVENT_TICK:
            calculate_planetary_hour(state); // Update planetary hour on each tick
            printf("[DEBUG] Updated planetary hour: %d\n", state->current_planetary_hour);
            break;
        default:
            return movement_default_loop_handler(event); // Handle other events with default handler
    }

    // Display the planetary hour or an error message if unavailable
    if (state->current_planetary_hour == PLANETARY_HOUR_ERROR) {
        printf("[ERROR] Planetary hour calculation failed\n");
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "Error", "ER");
    } else {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, planet_names[state->current_planetary_hour].name, planet_names[state->current_planetary_hour].abbreviation);
    }

    // Display the zodiac sign or an error message if unavailable
    if (state->current_zodiac_sign == ZODIAC_SIGN_ERROR) {
        printf("[ERROR] Zodiac sign calculation failed\n");
        watch_display_text(WATCH_POSITION_BOTTOM, "Error ");
    } else {
        watch_display_text(WATCH_POSITION_BOTTOM, zodiac_signs[state->current_zodiac_sign].name);
    }

    return true;
}

// Function to release resources when the planetary face is no longer active
void planetary_face_resign(void *context) {
    free(context); // Free allocated memory for the context
}

// Function to calculate the current planetary hour based on sunrise and sunset times
static void calculate_planetary_hour(planetary_state_t *state) {
    // Get the current date and time from the watch
    watch_date_time_t current_time = movement_get_local_date_time();

    // Load the sunrise and sunset times for the current day
    double sunrise, sunset;
    movement_location_t location = load_location_from_filesystem();
    if (location.reg == 0) {
        // No location set, return an error state
        state->current_planetary_hour = PLANETARY_HOUR_ERROR;
        return;
    } else {
        double lat = (double)location.bit.latitude / 100.0; // Convert latitude to decimal degrees
        double lon = (double)location.bit.longitude / 100.0; // Convert longitude to decimal degrees
        int result = __sunriset__(
            current_time.unit.year + WATCH_RTC_REFERENCE_YEAR,
            current_time.unit.month,
            current_time.unit.day,
            lon,
            lat,
            SUNRISE_SUNSET_ALTITUDE, // Altitude for sunrise/sunset calculations
            1,                       // Upper limb
            &sunrise,
            &sunset
        );

        if (result != 0) {
            // __sunriset__ failed, set an error state
            state->current_planetary_hour = PLANETARY_HOUR_ERROR;
            return;
        }
    }

    printf("[DEBUG] Sunrise: %.2f, Sunset: %.2f\n", sunrise, sunset);
    // Corrected calculation to convert sunrise to local time and compute time since sunrise
    // Convert sunrise to local time
    const double timezone_offset = movement_get_current_timezone_offset() / 3600.0; // Convert seconds to hours
    sunrise += timezone_offset;
    if (sunrise >= 24.0) sunrise -= 24.0; // Adjust for next day
    if (sunrise < 0.0) sunrise += 24.0;  // Adjust for previous day

    // Calculate the current time in hours (local time)
    double current_time_in_hours = current_time.unit.hour + (current_time.unit.minute / 60.0);

    // Calculate time since sunrise
    double time_since_sunrise = current_time_in_hours - sunrise;
    if (time_since_sunrise < 0) {
        time_since_sunrise += 24.0; // Adjust for times past midnight
    }

    // Convert to integer hours
    int time_since_sunrise_int = (int)time_since_sunrise;

    // Debug log for time since sunrise
    printf("[DEBUG] Time since sunrise (local): %d hours\n", time_since_sunrise_int);

    // Get the planetary ruler of the day
    // Calculate the day of the week using Zeller's Congruence
    int y = current_time.unit.year + WATCH_RTC_REFERENCE_YEAR;
    int m = current_time.unit.month;
    int d = current_time.unit.day;

    if (m < 3) {
        m += 12;
        y -= 1;
    }

    uint8_t day_of_week = (d + (2 * m) + (3 * (m + 1) / 5) + y + (y / 4) - (y / 100) + (y / 400) + 1) % 7;
    printf("[DEBUG] Day of week: %d\n", day_of_week);
    uint8_t ruler_of_day_index = week_days_to_chaldean_order[day_of_week]; // Align the day of the week with the chaldean order
    printf("[DEBUG] Ruler of day index: %d\n", ruler_of_day_index);

    // Calculate the planetary ruler of the hour
    state->current_planetary_hour = (ruler_of_day_index + time_since_sunrise_int) % 7;
}

// Function to determine the current astrological sign based on the date
static void calculate_astrological_sign(planetary_state_t *state) {
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
