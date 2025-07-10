/*
 * MIT License
 *
 * Copyright (c) 2024 <#author_name#>
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
#include "beer_counter_face.h"
#include "watch.h"
#include "watch_utility.h"

#define ALCOHOL_DENSITY 0.789  // g/ml
#define ELIMINATION_RATE_H 0.15f // Average elimination rate (g/kg/hour)
#define ELIMINATION_RATE 0.00004167f // Average elimination rate (g/kg/seconds)
#define RESORPTION_DEFICIT 0.9 // 100% - 10%
/* resorption deficit is between 10% and 30% depending on enzyme activity, 
 * filling of the stomach, concentration of alcohol in different beverages, etc.
 * here it is chosen to be on the safer side, so that the probability for sobriety is higher
 * when sobertime reaches 0.
 */

static inline int32_t get_tz_offset(movement_settings_t *settings) {
    return movement_timezone_offsets[settings->bit.time_zone] * 60;
}

static float calculate_bac(beer_counter_state_t *state) {
    watch_date_time_t now = watch_rtc_get_date_time();
    uint32_t current_time_unix = watch_utility_date_time_to_unix_time(now, 0);
    uint32_t time_elapsed_seconds = current_time_unix - state->last_time_bac;

    float new_alcohol_g = (state->beer_count * state->drink_vol * state->alc_cont * ALCOHOL_DENSITY)/100; //because we put in drink_vol as integers we have to divide by 100
    float old_alcohol_g = (state->old_beer_count * state->drink_vol * state->alc_cont * ALCOHOL_DENSITY)/100;

    float widmark_factor = (state->sex == 0) ? 0.68 : 0.55;
    
    float seidl_factor;
    if (state->sex == 0) {
        seidl_factor = (0.31608 - (0.004821 * state->weight) + (0.004432 * state->height));
        if (seidl_factor < 0.64) {
            seidl_factor = widmark_factor;
        }
    } else {
        seidl_factor = (0.31223 - (0.006446 * state->weight) + (0.004466 * state->height));
        if (seidl_factor < 0.54) {
            seidl_factor = widmark_factor;
        }
    }
    
    float old_bac = ((old_alcohol_g / (state->weight * seidl_factor)) *RESORPTION_DEFICIT);
    float new_bac = (new_alcohol_g / (state->weight * seidl_factor)) * RESORPTION_DEFICIT;
    float delta_bac = new_bac - old_bac;

    float current_bac;
    if (state->beer_count == state->old_beer_count) {
        current_bac = state->old_bac - (ELIMINATION_RATE * time_elapsed_seconds);
    } else {
        current_bac = (state->old_bac + delta_bac) - (ELIMINATION_RATE * time_elapsed_seconds);
        state->old_beer_count = state->beer_count;
    }
    
    if (current_bac < 0) {
        current_bac = 0;
    }

    state->old_bac = current_bac;
    state->last_time_bac = current_time_unix;
    return current_bac;
}

static uint32_t calculate_time_to_sober(float current_bac) {
    float time_to_sober_hours = current_bac / ELIMINATION_RATE_H;
    uint32_t time_to_sober_seconds = (uint32_t)(time_to_sober_hours * 3600);
    return time_to_sober_seconds;
}

static void print_error_message(void) {
    char buf[14];
    snprintf(buf, sizeof(buf), "BM I ERROR");
    watch_display_text(0, buf);
}

static inline bool lcd_is_custom(void) {
    return watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM;
}

static void print_beer_count(beer_counter_state_t *state) {
    char buf[14];
    if (lcd_is_custom()) {
        watch_clear_display();
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "BEERS", "BC");
        snprintf(buf, sizeof(buf), "  %02d", state->beer_count);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    } else {
        snprintf(buf, sizeof(buf), "BC    %02d  ", state->beer_count);
        watch_display_text(0, buf);
    }
}

static void print_bac(beer_counter_state_t *state) {
    float bac = calculate_bac(state);
    char buf[14];
    if (lcd_is_custom()) {
        watch_clear_display();
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "BAC", "BA C");
        snprintf(buf, sizeof(buf), "  %.2f", bac);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    } else {
        snprintf(buf, sizeof(buf), "BA C  %.2f", bac);
        watch_display_text(0, buf);
    }
}

static void print_sober_time(beer_counter_state_t *state) {
    float bac = state->old_bac;
    uint32_t time_to_sober_seconds = calculate_time_to_sober(bac);
    uint32_t hours = time_to_sober_seconds / 3600;
    uint32_t minutes = (time_to_sober_seconds % 3600) / 60;
    char buf[16];
    if (lcd_is_custom()) {
        watch_clear_display();
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "SOBER", "TT S");
        snprintf(buf, sizeof(buf), " %03u%02u", (unsigned int)hours, (unsigned int)minutes);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    } else {
        snprintf(buf, sizeof(buf), "TT S %03u%02u", (unsigned int)hours, (unsigned int)minutes);
        watch_display_text(0, buf);
    }
    if (time_to_sober_seconds == 0) {
        state->beer_count = 0;
    }
}

static void print_weight(beer_counter_state_t *state) {
    char buf[14];
    if (lcd_is_custom()) {
        watch_clear_display();
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "WEIGH", "WE");
        snprintf(buf, sizeof(buf), "%3u KG", (unsigned int)state->weight);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    } else {
        snprintf(buf, sizeof(buf), "WE   %03u  ", (unsigned int)state->weight);
        watch_display_text(0, buf);
    }
}

static void print_height(beer_counter_state_t *state) {
    char buf[14];
    if (lcd_is_custom()) {
        watch_clear_display();
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "HEIGH", "HE");
        snprintf(buf, sizeof(buf), "%3u CM", (unsigned int)state->height);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    } else {
        snprintf(buf, sizeof(buf), "HE   %03u  ", (unsigned int)state->height);
        watch_display_text(0, buf);
    }
}

static void print_sex(beer_counter_state_t *state) {
    if (lcd_is_custom()) {
        watch_clear_display();
        // Use WATCH_POSITION_TOP with fallback
        watch_display_text_with_fallback(WATCH_POSITION_TOP, 
            state->sex == 0 ? "MALE" : "FEMAL",
            state->sex == 0 ? "MA" : "FE");
        watch_display_text(WATCH_POSITION_BOTTOM, "   SEX");
    } else {
        char buf[14];
        snprintf(buf, sizeof(buf), "%s     SEX", state->sex == 0 ? "MA" : "FE");
        watch_display_text(0, buf);
    }
}


static void print_drink_vol(beer_counter_state_t *state) {
    char buf[14];
    if (lcd_is_custom()) {
        watch_clear_display();
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "VOL", "VD");
        snprintf(buf, sizeof(buf), "%4umL", (unsigned int)state->drink_vol);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    } else {
        snprintf(buf, sizeof(buf), "VD  %04u  ", (unsigned int)state->drink_vol);
        watch_display_text(0, buf);
    }
}

static void print_alc_cont(beer_counter_state_t *state) {
    char buf[14];
    if (lcd_is_custom()) {
        watch_clear_display();
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "ALCon", "AC");
        snprintf(buf, sizeof(buf), "%3u%%", (unsigned int)state->alc_cont);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    } else {
        snprintf(buf, sizeof(buf), "AC   %03u  ", (unsigned int)state->alc_cont);
        watch_display_text(0, buf);
    }
}

static bool quick_ticks_running = false;
static void abort_quick_ticks(beer_counter_state_t *state) {
    if (quick_ticks_running) {
        quick_ticks_running = false;
        if (state->mode == weight_screen || state->mode == height_screen || state->mode == sex_screen || state->mode == bac_screen || state->mode == sober_screen || state->mode == drink_vol_screen || state->mode == alc_cont_screen)
            movement_request_tick_frequency(4);
        else
            movement_request_tick_frequency(1);
    }
}

void beer_counter_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(beer_counter_state_t));
        memset(*context_ptr, 0, sizeof(beer_counter_state_t));
        beer_counter_state_t *state = (beer_counter_state_t *)*context_ptr;
        // Set default values
        state->weight = 50;  // Default weight in kg
        state->height = 150; // Default height in cm
        state->sex = 0;      // Default sex (0 for male, 1 for female)
        state->beer_count = 0;
        state->drink_vol = 500;
        state->alc_cont = 5;
        state->mode = weight_screen;
    }
}

void beer_counter_face_activate(void *context) {
    movement_request_tick_frequency(1);
    quick_ticks_running = false;
    watch_set_led_off();
    beer_counter_state_t *state = (beer_counter_state_t *)context;

    float bac = calculate_bac(state);
    if (bac == 0) {
        state->beer_count = 0; 
        state->old_beer_count = 0;
        state->old_bac = 0;
    }
    print_beer_count(state);
}

bool beer_counter_face_loop(movement_event_t event, void *context) {
    beer_counter_state_t *state = (beer_counter_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            state->mode = bc_screen;
            print_beer_count(state);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            watch_set_led_off();
            switch (state->mode) {
                case bac_screen:
                    state->mode = sober_screen;
                    print_sober_time(state);
                    break;
                case sober_screen:
                    state->mode = bc_screen;
                    print_beer_count(state);
                    break;
                case bc_screen:
                    state->mode = bac_screen;
                    print_bac(state);
                    break;
                case weight_screen:
                    state->mode = height_screen;
                    print_height(state);
                    break;
                case height_screen:
                    state->mode = sex_screen;
                    print_sex(state);
                    break;
                case sex_screen:
                    state->mode = weight_screen;
                    print_weight(state);
                    break;
                case drink_vol_screen:
                    state->mode = alc_cont_screen;
                    print_alc_cont(state);
                    break;
                case alc_cont_screen:
                    state->mode = drink_vol_screen;
                    print_drink_vol(state);
                    break;
                default:
                    state->mode = bc_screen;
                    print_beer_count(state);
                    break;
            }
            quick_ticks_running = false;
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            watch_set_led_off();
            break;

        case EVENT_LIGHT_LONG_PRESS:
            if (state->mode == bc_screen) {
                state->beer_count = (state->beer_count > 1) ? state->beer_count - 1 : 1;
                print_beer_count(state);
                break;
            }
            break;

        case EVENT_ALARM_BUTTON_UP:
            if (!quick_ticks_running) {
            abort_quick_ticks(state);
            }
            switch (state->mode) {
                case weight_screen:
                    state->weight = (state->weight < 250) ? state->weight + 1 : 30;
                    print_weight(state);
                    break;
                case height_screen:
                    state->height = (state->height < 220) ? state->height + 1 : 130;
                    print_height(state);
                    break;
                case sex_screen:
                    state->sex = !state->sex;
                    print_sex(state);
                    break;
                case drink_vol_screen:
                    state->drink_vol = (state->drink_vol < 1000) ? state->drink_vol + 10 : 20;
                    print_drink_vol(state);
                    break;
                case alc_cont_screen:
                    state->alc_cont = (state->alc_cont < 100) ? state->alc_cont + 1 : 1;
                    print_alc_cont(state);
                    break;
                default:
                    state->beer_count++;
                    watch_date_time_t now = watch_rtc_get_date_time();
                    state->last_time = watch_utility_date_time_to_unix_time(now, 0);
                    print_beer_count(state);
                    break;
            }
            break;

        case EVENT_ALARM_LONG_PRESS:
            switch(state->mode) {
                case weight_screen:
                case height_screen:
                case drink_vol_screen:
                case alc_cont_screen:
                    quick_ticks_running = true;
                    movement_request_tick_frequency(8);
                    break;
                default:
                    quick_ticks_running = false;
                    break;
            }
            if (state->mode == bc_screen) {
                state->beer_count = 0;
                state->old_beer_count = 0;
                state->old_bac = 0;
                watch_date_time_t now = watch_rtc_get_date_time();
                state->last_time = watch_utility_date_time_to_unix_time(now, 0);
                print_beer_count(state);
            }
            break;

        case EVENT_TICK:
            if (quick_ticks_running) {
                if (HAL_GPIO_BTN_ALARM_read()) {
                    switch (state->mode) {
                        case weight_screen:
                            state->weight = (state->weight < 250) ? state->weight + 1 : state->weight;
                            print_weight(state);
                            break;
                        case height_screen:
                            state->height = (state->height < 220) ? state->height + 1 : state->height;
                            print_height(state);
                            break;
                        case drink_vol_screen:
                            state->drink_vol = (state->drink_vol < 1000) ? state->drink_vol + 10 : state->drink_vol;
                            print_drink_vol(state);
                            break;
                        case alc_cont_screen:
                            state->alc_cont = (state->alc_cont < 100) ? state->alc_cont + 1 : state->alc_cont;
                            print_alc_cont(state);
                            break;
                        default:
                            abort_quick_ticks(state);
                            break;
                    }
                } else {
                    abort_quick_ticks(state);
                }
            }
            break;

        case EVENT_MODE_LONG_PRESS:
            switch (state->mode) {
                case weight_screen:
                case height_screen:
                case sex_screen:
                case bac_screen:
                case drink_vol_screen:
                case alc_cont_screen:
                    state->mode = bc_screen;
                    print_beer_count(state);
                    break;
                case sober_screen:
                    state->mode = weight_screen;
                    print_weight(state);
                    break;
                default:
                    state->mode = drink_vol_screen;
                    print_drink_vol(state);
                    break;
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

void beer_counter_face_resign(void *context) {
    (void)context;
}