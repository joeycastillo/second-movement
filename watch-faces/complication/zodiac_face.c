#include <stdlib.h>
#include <string.h>
#include "zodiac_face.h"
#include "watch.h"
#include "watch_utility.h"

// Zodiac signs and their end dates (month and day)
static const struct {
    const char *name;
    uint8_t end_month;
    uint8_t end_day;
    const char *abbreviation;
} zodiac_signs[] = {
    {"Aries", 4, 19, "AR"},
    {"Tauru", 5, 20, "TA"},
    {"Gemin", 6, 20, "GE"},
    {"Cance", 7, 22, "CA"},
    {"Leo  ", 8, 22, "LE"},
    {"Virgo", 9, 22, "VI"},
    {"Libra", 10, 22, "LI"},
    {"Scorp", 11, 21, "SC"},
    {"Sagit", 12, 21, "SA"},
    {"Capri", 1, 19, "CA"},
    {"Aquar", 2, 18, "AQ"},
    {"Pisce", 3, 20, "PI"},
};

#define ZODIAC_SIGN_COUNT (sizeof(zodiac_signs) / sizeof(zodiac_signs[0]))

// Function to determine the current zodiac sign based on the current date
static uint8_t get_current_zodiac_sign(void) {
    watch_date_time_t now = movement_get_local_date_time();
    uint8_t month = now.unit.month;
    uint8_t day = now.unit.day;

    for (uint8_t i = 0; i < ZODIAC_SIGN_COUNT; i++) {
        uint8_t end_month = zodiac_signs[i].end_month;
        uint8_t end_day = zodiac_signs[i].end_day;

        if ((month < end_month) || (month == end_month && day <= end_day)) {
            return i;
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

    // Display the current zodiac sign and its end date
    const char *current_sign = zodiac_signs[state->current_sign_index].name;
    uint8_t end_month = zodiac_signs[state->current_sign_index].end_month;
    uint8_t end_day = zodiac_signs[state->current_sign_index].end_day;

    char buf[7];
    snprintf(buf, sizeof(buf), "%02d%02d%02s", end_month, end_day, "ZF");

    watch_display_text_with_fallback(WATCH_POSITION_TOP, current_sign, zodiac_signs[state->current_sign_index].abbreviation);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

bool zodiac_face_loop(movement_event_t event, void *context) {
    zodiac_face_state_t *state = (zodiac_face_state_t *)context;

    switch (event.event_type) {
        case EVENT_ALARM_BUTTON_UP:
            // Move to the next zodiac sign
            state->current_sign_index = (state->current_sign_index + 1) % ZODIAC_SIGN_COUNT;
            zodiac_face_activate(context); // Update the display
            break;

        case EVENT_LIGHT_LONG_PRESS:
            // Move to the previous zodiac sign
            if (state->current_sign_index == 0) {
                state->current_sign_index = ZODIAC_SIGN_COUNT - 1;
            } else {
                state->current_sign_index--;
            }
            zodiac_face_activate(context); // Update the display
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void zodiac_face_resign(void *context) {
    (void)context;
}
