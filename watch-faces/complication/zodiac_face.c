#include <stdlib.h>
#include <string.h>
#include "zodiac_face.h"
#include "watch.h"
#include "watch_utility.h"

// Zodiac signs and their start dates (month and day)
static const struct {
    const char *name;
    uint8_t start_month;
    uint8_t start_day;
    const char *abbreviation;
} zodiac_signs[] = {
    {"Aries", 3, 21, "AR"},
    {"Tauru", 4, 20, "TA"},
    {"Gemin", 5, 21, "GE"},
    {"Cance", 6, 21, "CA"},
    {"Leo  ", 7, 23, "LE"},
    {"Virgo", 8, 23, "VI"},
    {"Libra", 9, 23, "LI"},
    {"Scorp", 10, 23, "SC"},
    {"Sagit", 11, 22, "SA"},
    {"Capri", 12, 22, "CA"},
    {"Aquar", 1, 20, "AQ"},
    {"Pisce", 2, 19, "PI"},
};

#define ZODIAC_SIGN_COUNT (sizeof(zodiac_signs) / sizeof(zodiac_signs[0]))

// Function to determine the current zodiac sign based on the current date
static uint8_t get_current_zodiac_sign(void) {
    watch_date_time_t now = movement_get_local_date_time();
    uint8_t month = now.unit.month;
    uint8_t day = now.unit.day;

    for (uint8_t i = 0; i < ZODIAC_SIGN_COUNT; i++) {
        uint8_t start_month = zodiac_signs[i].start_month;
        uint8_t start_day = zodiac_signs[i].start_day;

        if ((month < start_month) || (month == start_month && day < start_day)) {
            return (i == 0) ? (uint8_t)(ZODIAC_SIGN_COUNT - 1) : (uint8_t)(i - 1);
        }
    }

    // If no match is found, default to the first zodiac sign (Aries)
    return 0;
}

void zodiac_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(zodiac_face_state_t));
        zodiac_face_state_t *state = (zodiac_face_state_t *)*context_ptr;
        memset(state, 0, sizeof(zodiac_face_state_t));

        // Initialize to the current zodiac sign based on the date
        state->current_sign_index = get_current_zodiac_sign();
    }
}

void zodiac_face_activate(void *context) {
    zodiac_face_state_t *state = (zodiac_face_state_t *)context;

    // Display the current zodiac sign and its start date
    const char *current_sign = zodiac_signs[state->current_sign_index].name;
    uint8_t start_month = zodiac_signs[state->current_sign_index].start_month;
    uint8_t start_day = zodiac_signs[state->current_sign_index].start_day;

    char month[4];
    snprintf(month, sizeof(month), "%02d", start_month);
    char day[4];
    snprintf(day, sizeof(day), "%02d", start_day);

    watch_display_text_with_fallback(WATCH_POSITION_TOP, current_sign, zodiac_signs[state->current_sign_index].abbreviation);

    watch_display_text(WATCH_POSITION_HOURS, month);
    watch_display_text(WATCH_POSITION_MINUTES, day);
    watch_display_text(WATCH_POSITION_SECONDS, "ST");
}

bool zodiac_face_loop(movement_event_t event, void *context) {
    zodiac_face_state_t *state = (zodiac_face_state_t *)context;

    switch (event.event_type) {
        case EVENT_ALARM_BUTTON_UP:
            // Move to the next zodiac sign
            state->current_sign_index = (state->current_sign_index + 1) % ZODIAC_SIGN_COUNT;
            zodiac_face_activate(context); // Update the display
            break;

        case EVENT_LIGHT_BUTTON_UP:
            // Move to the previous zodiac sign
            if (state->current_sign_index == 0) {
                state->current_sign_index = ZODIAC_SIGN_COUNT - 1;
            } else {
                state->current_sign_index--;
            }
            zodiac_face_activate(context); // Update the display
            break;

        case EVENT_LIGHT_LONG_PRESS:
            movement_illuminate_led();
            break;

        case EVENT_ALARM_BUTTON_DOWN:
        case EVENT_ALARM_LONG_PRESS:
        case EVENT_LIGHT_BUTTON_DOWN:
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void zodiac_face_resign(void *context) {
    (void)context;
}
