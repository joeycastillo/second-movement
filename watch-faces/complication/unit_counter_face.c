#include <stdlib.h>
#include <string.h>
#include "unit_counter_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "bac.h"

#define ELIMINATION_RATE_H 0.15f // Average elimination rate (g/kg/hour)
#define ALCOHOL_DENSITY 0.789  // g/ml

// Forward declarations for static functions
static void print_unit_count(unit_counter_state_t *state);
static uint32_t calculate_time_to_point_two(float current_bac);

void unit_counter_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(unit_counter_state_t));
        unit_counter_state_t *state = (unit_counter_state_t *)*context_ptr;
        memset(*context_ptr, 0, sizeof(unit_counter_state_t));
        state->unit_count = 0;
        state->weight = 95;
        state->sex = 0;
        state->edit_on = false;
        state->edit_weight = false;
    }
}

static void print_unit_count(unit_counter_state_t *state) {
    char buf[4];
    // We actually do not want to pad with 0 but with space.
    sprintf(buf, "%2d", state->unit_count);
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "UC", "Uts");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    // Let's also get the BAC.
    float bac = unit_counter_calculate_bac(state);
    char result[3][8];
    parse_bac_into_result(bac, result);
    char hour_buf[11];
    char minute_buf[11];
    char second_buf[11];
    sprintf(hour_buf, " %s", result[0]);
    sprintf(minute_buf, "%s", result[1]);
    sprintf(second_buf, "%s", result[2]);
    watch_display_text(WATCH_POSITION_HOURS, hour_buf);
    watch_display_text(WATCH_POSITION_MINUTES, minute_buf);
    watch_display_text(WATCH_POSITION_SECONDS, second_buf);
}

void parse_bac_into_result(float val, char result[3][8]) {
    char str[32] = {0};
    sprintf(str, "%.6f", val); // Convert float to string with 6 decimals

    // The digit before the dot
    result[0][0] = str[0];
    result[0][1] = '\0';

    // The first and second digits after the dot
    strncpy(result[1], &str[2], 2);
    result[1][2] = '\0';

    // The third and fourth digits after the dot
    strncpy(result[2], &str[4], 2);
    result[2][2] = '\0';
}

float unit_counter_calculate_bac(unit_counter_state_t *state) {
    float result;
    float alcohol_g = 0.0;
    watch_date_time_t now = movement_get_local_date_time();
    uint32_t current_time_unix = watch_utility_date_time_to_unix_time(now, movement_get_current_timezone_offset());
    uint32_t time_elapsed_seconds = current_time_unix - state->start_time;
    if (time_elapsed_seconds < 0) {
        time_elapsed_seconds = 0;
    }
    if (state->unit_count == 0) {
        return 0;
    }
    for (int i = 0; i < state->unit_count; i++) {
        float unit_g = (state->units[i].volume * (state->units[i].percentage / 10) * ALCOHOL_DENSITY) / 100;
        alcohol_g += unit_g;
    }
    if (state->sex == 1) {
        result = bac_for_women_from_weight_and_alcohol_grams(state->weight, alcohol_g, state->start_time, current_time_unix);
    }
    else {
        result = bac_for_men_from_weight_and_alcohol_grams(state->weight, alcohol_g, state->start_time, current_time_unix);
    }
    return result;
}

void unit_counter_face_activate(void *context) {
    watch_set_led_off();

    unit_counter_state_t *state = (unit_counter_state_t *)context;

    float bac = unit_counter_calculate_bac(state);
    if (bac == 0) {
        state->unit_count = 0;
        state->start_time = 0;
    }
    print_unit_count(state);
}

void draw_screen(unit_counter_state_t *state) {
    if (state->screen_delta == 0) {
        print_unit_count(state);
    }
    if (state->screen_delta == 1) {
        print_edit_screen(state);
    }
    if (state->screen_delta == 2) {
        unit_counter_print_time_to_sober_screen(state);
    }
    if (state->screen_delta == 3) {
        unit_counter_print_settings_screen(state);
    }
}

void unit_counter_print_time_to_sober_screen(unit_counter_state_t *state) {
    char buf[10];
    // We actually do not want to pad with 0 but with space.
    sprintf(buf, "%2d", state->unit_count);
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "TS", "TTS");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    // First calculate the number of seconds.
    float bac = unit_counter_calculate_bac(state);
    uint32_t time_to_sober = calculate_time_to_point_two(bac);
    // Now convert to hours, minutes and seconds.
    uint32_t hours = time_to_sober / 3600;
    uint32_t minutes = (time_to_sober % 3600) / 60;
    uint32_t seconds = time_to_sober % 60;
    sprintf(buf, "%2d%02d%02d", hours, minutes, seconds);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

void unit_counter_print_settings_screen(unit_counter_state_t *state) {
    char buf[10];
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "SE", "SE");
    watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, "  ", "  ");
    // Write the sex in the bottom.
    sprintf(buf, "%s %3d", state->sex ? " F" : "MM", state->weight);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    // If we are editing, then let's blink it a bit.
    watch_date_time_t now = movement_get_local_date_time();
    uint32_t current_time_unix = watch_utility_date_time_to_unix_time(now, movement_get_current_timezone_offset());
    if (state->edit_on && current_time_unix % 2 == 0) {
        if (!state->edit_weight) {
            watch_display_text(WATCH_POSITION_HOURS, "  ");
        }
        else {
            watch_display_text(WATCH_POSITION_MINUTES, "  ");
            watch_display_text(WATCH_POSITION_SECONDS, "  ");
        }
    }
}

void print_edit_screen(unit_counter_state_t *state) {
    char buf[10];
    int delta = state->unit_count - state->edit_offset;
    sprintf(buf, "%2d", delta);
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "ED", "EDT");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    // There is one value to display, and another to talk about to the computer.
    int computer_delta = delta - 1;
    int volume = state->units[computer_delta].volume;
    int percentage = state->units[computer_delta].percentage;
    // Depending on edit mode or not, we want to display different things.
    if (state->edit_on) {
        // Check if we are editing volume or percentage.
        if (state->is_alc_cont_screen) {
            // Blank it out first.
            int tens = percentage * 10;
            watch_display_text(WATCH_POSITION_HOURS, "  ");
            watch_display_text(WATCH_POSITION_MINUTES, "  ");
            watch_display_text(WATCH_POSITION_SECONDS, "  ");
            sprintf(buf, "%d", tens % 100);
            watch_display_text(WATCH_POSITION_SECONDS, buf);
            sprintf(buf, "%2d", (tens - (tens % 100) ) / 100);
            watch_display_text(WATCH_POSITION_MINUTES, buf);
        }
        else {
            sprintf(buf, "%d", volume);
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
        }
        // Blink it a bit.
        watch_date_time_t now = movement_get_local_date_time();
        uint32_t current_time_unix = watch_utility_date_time_to_unix_time(now, movement_get_current_timezone_offset());
        if (current_time_unix % 2 == 0) {
            watch_display_text(WATCH_POSITION_HOURS, "  ");
            watch_display_text(WATCH_POSITION_MINUTES, "  ");
            watch_display_text(WATCH_POSITION_SECONDS, "  ");
        }
    }
    else {
        sprintf(buf, "%d", volume);
        // Split the volume in two parts. First part should be the first number.
        char first_part[3];
        sprintf(first_part, " %d", volume / 100);
        watch_display_text(WATCH_POSITION_HOURS, first_part);

        char second_part[3];
        second_part[0] = buf[1];
        second_part[1] = buf[2];
        second_part[2] = '\0';
        watch_display_text(WATCH_POSITION_MINUTES, second_part);
        sprintf(buf, "%d", percentage);
        watch_display_text(WATCH_POSITION_SECONDS, buf);
    }
}

bool unit_counter_face_loop(movement_event_t event, void *context) {
    unit_counter_state_t *state = (unit_counter_state_t *)context;

    switch (event.event_type) {
        case EVENT_LIGHT_BUTTON_DOWN:
            // Let's not trigger light.
            break;
        case EVENT_TICK:
            draw_screen(state);
            break;

        case EVENT_ACTIVATE:
            state->screen_delta = 0;
            state->edit_on = false;
            state->edit_offset = 0;
            print_unit_count(state);
            break;

        case EVENT_ALARM_LONG_PRESS:
            // If we are on the edit screen, we want to delete the currently viewed unit.
            if (state->screen_delta != 1) {
                break;
            }
            if (state->unit_count == 0) {
                // Should really not be possible, but OK.
                break;
            }
            int delta = state->unit_count - state->edit_offset;
            int computer_delta = delta - 1;
            state->unit_count--;
            // Now shift all the units so they are in order.
            for (int i = computer_delta; i < 20; i++) {
                state->units[i].volume = state->units[i + 1].volume;
                state->units[i].percentage = state->units[i + 1].percentage;
            }
            // If we are now on 0 beers, reset to first screen.
            if (state->unit_count == 0) {
                state->screen_delta = 0;
            }
            // Always reset the edit offset.
            state->edit_offset = 0;
            draw_screen(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            // If we are on the edit screen, we want to change the value.
            if (state->screen_delta == 1) {
                if (state->edit_on) {
                    int delta = state->unit_count - state->edit_offset;
                    int computer_delta = delta - 1;
                    if (state->is_alc_cont_screen) {
                        state->units[computer_delta].percentage += 5;
                        if (state->units[computer_delta].percentage > 250) {
                            state->units[computer_delta].percentage = 25;
                        }
                    }
                    else {
                        // Cycle through the available volumes.
                        if (state->units[computer_delta].volume == 300) {
                            state->units[computer_delta].volume = 330;
                        }
                        else if (state->units[computer_delta].volume == 330) {
                            state->units[computer_delta].volume = 400;
                        }
                        else if (state->units[computer_delta].volume == 400) {
                            state->units[computer_delta].volume = 440;
                        }
                        else if (state->units[computer_delta].volume == 440) {
                            state->units[computer_delta].volume = 500;
                        }
                        else if (state->units[computer_delta].volume == 500) {
                            state->units[computer_delta].volume = 600;
                        }
                        else if (state->units[computer_delta].volume == 600) {
                            state->units[computer_delta].volume = 700;
                        }
                        else if (state->units[computer_delta].volume == 700) {
                            state->units[computer_delta].volume = 750;
                        }
                        else if (state->units[computer_delta].volume == 750) {
                            state->units[computer_delta].volume = 300;
                        }
                    }
                }
                else {
                    // Cycle the units.
                    state->edit_offset++;
                    if (state->edit_offset >= state->unit_count) {
                        state->edit_offset = 0;
                    }
                }
                print_edit_screen(state);
                break;
            }
            if (state->screen_delta == 3) {
                // Don't add more beers to it from this screen and not in edit mode.
                if (!state->edit_on) {
                    break;
                }
                // Depending on the thing we are editing.
                if (state->edit_weight) {
                    state->weight += 1;
                    // Max is... I dunno, 120?
                    if (state->weight > 120) {
                        state->weight = 45;
                    }
                }
                else {
                    if (state->sex == 0) {
                        state->sex = 1;
                    }
                    else {
                        state->sex = 0;
                    }
                }
                unit_counter_print_settings_screen(state);
                break;
            }
            // Store the current last one in a temp variable. Unless its the first one.
            unit_t temp_unit = {500, 45};
            if (state->unit_count > 0) {
                temp_unit.volume = state->units[state->unit_count - 1].volume;
                temp_unit.percentage = state->units[state->unit_count - 1].percentage;
            }
            // Append one more item to the units array.
            state->unit_count++;
            state->units[state->unit_count - 1].volume = temp_unit.volume;
            state->units[state->unit_count - 1].percentage = temp_unit.percentage;
            // If the start time is not set, set it to the current timestamp.
            if (state->start_time == 0) {
                watch_date_time_t date_time = movement_get_local_date_time();
                state->start_time = watch_utility_date_time_to_unix_time(date_time, movement_get_current_timezone_offset());
            }
            print_unit_count(state);
            break;

        case EVENT_LIGHT_BUTTON_UP:
            // If the count is 0, then do nothing.
            if (state->unit_count == 0) {
                break;
            }
            // Cycle the modes. Unless in edit mode.
            if (state->screen_delta == 1 && state->edit_on) {
                if (!state->is_alc_cont_screen) {
                    state->is_alc_cont_screen = true;
                }
                else {
                    state->is_alc_cont_screen = false;
                    state->edit_offset++;
                }
                if (state->edit_offset >= state->unit_count) {
                    state->edit_offset = 0;
                }
                print_edit_screen(state);
                break;
            }
            // Edit mode settings
            if (state->screen_delta == 2 && state->edit_on) {
                state->edit_weight = !state->edit_weight;
                unit_counter_print_settings_screen(state);
                break;
            }
            state->screen_delta++;
            if (state->screen_delta > 3) {
                state->screen_delta = 0;
            }
            draw_screen(state);
            break;

        case EVENT_LIGHT_LONG_PRESS:
            // Only works in edit or settings mode.
            if (state->screen_delta == 1) {
                state->edit_on = !state->edit_on;
                if (state->edit_on) {
                    state->edit_offset = 0;
                    state->is_alc_cont_screen = false;
                }
                print_edit_screen(state);
            }
            if (state->screen_delta == 3) {
                state->edit_on = !state->edit_on;
                if (state->edit_on) {
                    state->edit_weight = true;
                }
                unit_counter_print_settings_screen(state);
            }
            break;

        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;


        default:
            return movement_default_loop_handler(event);
    }
    return true;

}

void unit_counter_face_resign(void *context) {
}

static uint32_t calculate_time_to_point_two(float current_bac) {
    current_bac = current_bac - 0.2;
    if (current_bac < 0) {
        current_bac = 0;
    }
    float time_to_sober_hours = current_bac / ELIMINATION_RATE_H;
    uint32_t time_to_sober_seconds = (uint32_t)(time_to_sober_hours * 3600);
    return time_to_sober_seconds;
}
