/*
 * MIT License
 *
 * Copyright (c) <https://github.com/metrast/>

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
#include "cpr_helper_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_slcd.h"
#include "watch_common_display.h"

#if defined(WATCH_GREEN_TCC_CHANNEL) || defined(WATCH_BLUE_TCC_CHANNEL)
#define MAX_SETTINGS_INDEX 5
#else
#define MAX_SETTINGS_INDEX 4
#endif

#if !defined(WATCH_GREEN_TCC_CHANNEL)
// Red + Blue (no green)
#define DEFAULT_ADR_COLOR_RED 4
#define DEFAULT_ADR_COLOR_GREEN 0
#define DEFAULT_ADR_COLOR_BLUE 0

#define DEFAULT_SHOCK_COLOR_RED 0
#define DEFAULT_SHOCK_COLOR_GREEN 0
#define DEFAULT_SHOCK_COLOR_BLUE 4

#elif !defined(WATCH_BLUE_TCC_CHANNEL)
// Red + Green (no blue)
#define DEFAULT_ADR_COLOR_RED 4
#define DEFAULT_ADR_COLOR_GREEN 0
#define DEFAULT_ADR_COLOR_BLUE 0

#define DEFAULT_SHOCK_COLOR_RED 2
#define DEFAULT_SHOCK_COLOR_GREEN 2
#define DEFAULT_SHOCK_COLOR_BLUE 0

#else
// Full RGB: Red + Green + Blue
#define DEFAULT_ADR_COLOR_RED 4
#define DEFAULT_ADR_COLOR_GREEN 0
#define DEFAULT_ADR_COLOR_BLUE 0

#define DEFAULT_SHOCK_COLOR_RED 0
#define DEFAULT_SHOCK_COLOR_GREEN 0
#define DEFAULT_SHOCK_COLOR_BLUE 4
#endif


// distant future for background task: January 1, 2083
static const watch_date_time_t distant_future = {
    .unit = {0, 0, 0, 1, 1, 63}
};

static movement_color_t saved_led_color;
static uint8_t saved_dwell;

static inline bool lcd_is_custom(void) {
    return watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM;
}

static void save_timestamp(cpr_helper_state_t *state) {
    if (state->timestamp_count < CPR_MAX_TIMESTAMPS) {
        watch_date_time_t now = watch_rtc_get_date_time();
        state->timestamps[state->timestamp_count] = now;

        uint32_t now_unix = watch_utility_date_time_to_unix_time(now, 0);
        uint32_t start_unix = watch_utility_date_time_to_unix_time(state->start_time, 0);
        state->timestamp_elapsed[state->timestamp_count] = now_unix - start_unix;

        state->adrenaline_counts[state->timestamp_count] = state->adrenaline_count;
        state->shock_counts[state->timestamp_count] = state->shock_count;
        state->timestamp_count++;
    }
}

static void print_timestamp_counts(cpr_helper_state_t *state, uint8_t adr_count, uint8_t shk_count) {
    (void) state;
    char buf[4];
    if (lcd_is_custom()) {
        snprintf(buf, sizeof(buf), "%2u", shk_count);
        watch_display_text(WATCH_POSITION_TOP_LEFT, buf);
        snprintf(buf, sizeof(buf), "%2u", adr_count);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    } else {
        snprintf(buf, sizeof(buf), "%2u", shk_count);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
        snprintf(buf, sizeof(buf), "%2u", adr_count);
        watch_display_text(WATCH_POSITION_SECONDS, buf);
    }
}

static void print_elapsed_time(uint32_t seconds) {
    char buf[14];
    if ((seconds / 60) < 100) {
        snprintf(buf, sizeof(buf), "%02u%02u  ", (unsigned)(seconds / 60), (unsigned)(seconds % 60));
    } else {
        snprintf(buf, sizeof(buf), "%lu%02lu  ", (unsigned long)(seconds / 60), (unsigned long)(seconds % 60));
    }
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static void indicators(cpr_helper_state_t *state) {
    if (state->timestamp_index == 0) {
        watch_set_indicator(WATCH_INDICATOR_LAP);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_LAP);
    }

    if (lcd_is_custom()) {
        if (state->timestamp_index == state->timestamp_count - 1) {
            watch_set_indicator(WATCH_INDICATOR_SLEEP);
        } else {
            watch_clear_indicator(WATCH_INDICATOR_SLEEP);
        }
    } else {
        if (state->timestamp_index == state->timestamp_count - 1) {
            watch_set_indicator(WATCH_INDICATOR_BELL);
        } else {
            watch_clear_indicator(WATCH_INDICATOR_BELL);
        }
    }
}

static void timer_start(cpr_helper_state_t *state) {
    if (state->running) {
        if (movement_button_should_sound()) {
                watch_buzzer_play_note_with_volume(BUZZER_NOTE_C8, 50, movement_button_volume());
            }
        state->running = false;
        state->in_timestamp_view = true;
        state->timestamp_index = state->timestamp_count;
        state->show_elapsed_time_in_review = true;
        save_timestamp(state);
        movement_cancel_background_task();
        print_elapsed_time(state->timestamp_elapsed[state->timestamp_index]);
        print_timestamp_counts(state,
                                state->adrenaline_counts[state->timestamp_index],
                                state->shock_counts[state->timestamp_index]);
        indicators(state);
    } else if (state->start_time.reg == 0) {
        if (movement_button_should_sound()) {
                watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
            }
        state->running = true;
        state->in_timestamp_view = false;
        state->start_time = watch_rtc_get_date_time();
        save_timestamp(state);
        movement_schedule_background_task(distant_future);
    }
}

static void timer_screen(cpr_helper_state_t *state) {
    if (state->running) {
        watch_date_time_t now = watch_rtc_get_date_time();
        uint32_t now_timestamp = watch_utility_date_time_to_unix_time(now, 0);
        uint32_t start_timestamp = watch_utility_date_time_to_unix_time(state->start_time, 0);
        state->seconds_counted = now_timestamp - start_timestamp;
    }

    if (state->seconds_counted >= 11999) { // 199 min 59 sec
        state->running = false;
        movement_cancel_background_task();
        watch_display_text(WATCH_POSITION_BOTTOM, "19959");
        return;
    }

    uint32_t total_minutes = state->seconds_counted / 60;
    uint8_t seconds = state->seconds_counted % 60;

    if (state->running && seconds == 0 && total_minutes > 0 && (total_minutes % 4 == 0)) {
    movement_play_alarm();
    }

    char buf[14];
    sprintf(buf, "%02lu%02u", (unsigned long)total_minutes, seconds);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static void print_shock_count(cpr_helper_state_t *state) {
    char buf[4];
    if (lcd_is_custom()) {
        snprintf(buf, sizeof(buf), "%2u", state->shock_count);
        watch_display_text(WATCH_POSITION_TOP_LEFT, buf);
    } else {
        snprintf(buf, sizeof(buf), "%2u", state->shock_count);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }
}

static void print_adrenaline_count(cpr_helper_state_t *state) {
    char buf[4];
    if (lcd_is_custom()) {
        snprintf(buf, sizeof(buf), "%2u", state->adrenaline_count);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    } else {
        snprintf(buf, sizeof(buf), "%2u", state->adrenaline_count);
        watch_display_text(WATCH_POSITION_SECONDS, buf);
    }
}

static void illuminate_shock_led(cpr_helper_state_t *state) {
    movement_color_t shock_color = {
        .red = (state->led_shock_red == 0) ? DEFAULT_SHOCK_COLOR_RED : state->led_shock_red,
        .green = (state->led_shock_green == 0) ? DEFAULT_SHOCK_COLOR_GREEN : state->led_shock_green,
        .blue = (state->led_shock_blue == 0) ? DEFAULT_SHOCK_COLOR_BLUE : state->led_shock_blue,
    };
    movement_set_backlight_dwell(1);
    movement_set_backlight_color(shock_color);
    movement_illuminate_led();
}

static void illuminate_adrenaline_led(cpr_helper_state_t *state) {
    movement_color_t adr_color = {
        .red = (state->led_adrenaline_red == 0) ? DEFAULT_ADR_COLOR_RED : state->led_adrenaline_red,
        .green = (state->led_adrenaline_green == 0) ? DEFAULT_ADR_COLOR_GREEN : state->led_adrenaline_green,
        .blue = (state->led_adrenaline_blue == 0) ? DEFAULT_ADR_COLOR_BLUE : state->led_adrenaline_blue,
    };
    movement_set_backlight_dwell(1);
    movement_set_backlight_color(adr_color);
    movement_illuminate_led();
}

static void shock_count_increment(cpr_helper_state_t *state) {
    int max_shock_count = lcd_is_custom() ? 99 : 39;

    if (state->shock_count < max_shock_count) {
        state->shock_count++;
        save_timestamp(state);
    }

    if (state->shock_count > state->last_displayed_shock_count) {
        illuminate_shock_led(state);
    }

    print_shock_count(state);
}

static void adrenaline_count_increment(cpr_helper_state_t *state) {
    if (state->adrenaline_count < 99) {
        state->adrenaline_count++;
        movement_force_led_off();
        save_timestamp(state);
    }
    if (state->adrenaline_count > state->last_displayed_adrenaline_count) {
        illuminate_adrenaline_led(state);
    }
    print_adrenaline_count(state);
}

static void print_timestamp(watch_date_time_t t) {
    char buf[10];
    if (lcd_is_custom()) {
        snprintf(buf, sizeof(buf), "%02u%02u%02u", t.unit.hour, t.unit.minute, t.unit.second);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    } else {
        snprintf (buf, sizeof(buf), "%02u%02u", t.unit.hour, t.unit.minute);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    }
}

static void toggle_timestamp(cpr_helper_state_t *state) {
    if (state->timestamp_index >= state->timestamp_count) 
        return;
    if (!state->show_elapsed_time_in_review) {
        print_timestamp(state->timestamps[state->timestamp_index]);
    } else {
        print_elapsed_time(state->timestamp_elapsed[state->timestamp_index]);
    }
    print_timestamp_counts(state,
        state->adrenaline_counts[state->timestamp_index],
        state->shock_counts[state->timestamp_index]);
}

static void review_screen(cpr_helper_state_t *state) {
    if (state->timestamp_count == 0) {
        watch_display_text(WATCH_POSITION_BOTTOM, "0000  ");
        print_adrenaline_count(state);
        print_shock_count(state);
        return;
    }

    if (state->in_timestamp_view) {
        movement_force_led_off();
    }

    if (state->timestamp_index >= state->timestamp_count) {
        state->timestamp_index = 0;
    }

    uint8_t adr_count = state->adrenaline_counts[state->timestamp_index];
    uint8_t shk_count = state->shock_counts[state->timestamp_index];

    indicators(state);
    toggle_timestamp(state);

    if (shk_count > state->last_displayed_shock_count) {
        illuminate_shock_led(state);
    }
    if (adr_count > state->last_displayed_adrenaline_count) {
        illuminate_adrenaline_led(state);
    }

    state->last_displayed_shock_count = shk_count;
    state->last_displayed_adrenaline_count = adr_count;
}

static void reset_state(cpr_helper_state_t *state) {
    state->start_time.reg = 0;
    state->seconds_counted = 0;
    state->adrenaline_count = 0;
    state->shock_count = 0;
    state->in_timestamp_view = false;
    state->timestamp_index = 0;
    state->last_displayed_shock_count = 0;
    state->last_displayed_adrenaline_count = 0;
    state->timestamp_count = 0;
    memset(state->adrenaline_counts, 0, sizeof(state->adrenaline_counts));
    memset(state->shock_counts, 0, sizeof(state->shock_counts));
    memset(&state->start_time, 0, sizeof(state->start_time));
    watch_clear_display();
    if (movement_button_should_sound()) {
                watch_buzzer_play_note_with_volume(BUZZER_NOTE_C6, 50, movement_button_volume());
            }
    watch_display_text(WATCH_POSITION_BOTTOM, "0000  ");
    print_adrenaline_count(state);
    print_shock_count(state);
}

static void shock_led_settings(cpr_helper_state_t *state) {
    movement_set_backlight_dwell(3);
    switch (state->settings_index) {
        case 0:
            state->led_shock_red = (state->led_shock_red + 1) % 16;
            break;
        case 1:
            state->led_shock_green = (state->led_shock_green + 1) % 16;
            break;
        case 2:
            state->led_shock_blue = (state->led_shock_blue + 1) % 16;
            break;
        default:
            return;
    }

    movement_color_t shock_color = {
        .red = state->led_shock_red,
        .green = state->led_shock_green,
        .blue = state->led_shock_blue,
    };
    movement_set_backlight_color(shock_color);
    movement_force_led_on(shock_color.red | (shock_color.red << 4),
                          shock_color.green | (shock_color.green << 4),
                          shock_color.blue | (shock_color.blue << 4));
}

static void adrenaline_led_settings(cpr_helper_state_t *state) {
    movement_set_backlight_dwell(3);
    switch (state->settings_index) {
        case 3:
            state->led_adrenaline_red = (state->led_adrenaline_red + 1) % 16;
            break;
        case 4:
            state->led_adrenaline_green = (state->led_adrenaline_green + 1) % 16;
            break;
        case 5:
            state->led_adrenaline_blue = (state->led_adrenaline_blue + 1) % 16;
            break;
        default:
            return;
    }

    movement_color_t adr_color = {
        .red = state->led_adrenaline_red,
        .green = state->led_adrenaline_green,
        .blue = state->led_adrenaline_blue,
    };
    movement_set_backlight_color(adr_color);
    movement_force_led_on(adr_color.red | (adr_color.red << 4),
                          adr_color.green | (adr_color.green << 4),
                          adr_color.blue | (adr_color.blue << 4));
}

static void print_settings (cpr_helper_state_t *state) {
    watch_clear_display();
    const char *color_label;
    const char *type_label;
    uint8_t intensity = 0;

    switch (state->settings_index) {
        case 0: type_label = "Sh"; color_label = " red  "; intensity = state->led_shock_red; break;
#ifdef WATCH_GREEN_TCC_CHANNEL
        case 1: type_label = "Sh"; color_label = " green"; intensity = state->led_shock_green; break;
#endif
#ifdef WATCH_BLUE_TCC_CHANNEL
        case 2: type_label = "Sh"; color_label = " blue "; intensity = state->led_shock_blue; break;
#endif
        case 3: type_label = "Ad"; color_label = " red  "; intensity = state->led_adrenaline_red; break;
#ifdef WATCH_GREEN_TCC_CHANNEL
        case 4: type_label = "Ad"; color_label = " green"; intensity = state->led_adrenaline_green; break;
#endif
#ifdef WATCH_BLUE_TCC_CHANNEL
        case 5: type_label = "Ad"; color_label = " blue "; intensity = state->led_adrenaline_blue; break;
#endif
        default:
            state->settings_index = (state->settings_index + 1) % (MAX_SETTINGS_INDEX + 1);
            print_settings(state);
            return;
    }

    char buf[14];
    snprintf(buf, sizeof(buf), "%s", type_label);
    watch_display_text(WATCH_POSITION_TOP_LEFT, buf);
    snprintf(buf, sizeof(buf), "%s", color_label);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    if (lcd_is_custom()) {
        snprintf(buf, sizeof(buf), "%02u", intensity);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    } else {
        snprintf(buf, sizeof(buf), "%2u", intensity);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }
}

static bool settings_index_advance(cpr_helper_state_t *cpr_helper_state) {
    cpr_helper_state->settings_index = (cpr_helper_state->settings_index + 1) % (MAX_SETTINGS_INDEX + 1);
    print_settings(cpr_helper_state);
    return true;
}

static bool settings_increment(cpr_helper_state_t *cpr_helper_state) {

#if !defined(WATCH_BLUE_TCC_CHANNEL) || !defined(WATCH_GREEN_TCC_CHANNEL)
    uint8_t idx = cpr_helper_state->settings_index;
#endif

#ifndef WATCH_BLUE_TCC_CHANNEL
    if (idx == 2 || idx == 5) {
        return true;
    }
#endif

#ifndef WATCH_GREEN_TCC_CHANNEL
    if (idx == 1 || idx == 4) {
        return true;
    }
#endif

    adrenaline_led_settings(cpr_helper_state);
    shock_led_settings(cpr_helper_state);
    print_settings(cpr_helper_state);
    return true;
}


static void toggle_settings(cpr_helper_state_t *state) {
    if (!state->in_settings_mode && !state->running && !state->in_timestamp_view) {
        state->in_settings_mode = true;
        state->settings_index = 0;
        print_settings(state);
    } else if (state->running || state->in_timestamp_view) {
        return;
    } else {
        state->in_settings_mode = false;
        watch_clear_display();
        watch_display_text(WATCH_POSITION_BOTTOM, "0000  ");
        print_adrenaline_count(state);
        print_shock_count(state);
    }
}

void cpr_helper_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(cpr_helper_state_t));
        memset(*context_ptr, 0, sizeof(cpr_helper_state_t));

        cpr_helper_state_t *state = (cpr_helper_state_t *)*context_ptr;

        state->led_shock_red = DEFAULT_SHOCK_COLOR_RED;
        state->led_shock_green = DEFAULT_SHOCK_COLOR_GREEN;
        state->led_shock_blue = DEFAULT_SHOCK_COLOR_BLUE;

        state->led_adrenaline_red = DEFAULT_ADR_COLOR_RED;
        state->led_adrenaline_green = DEFAULT_ADR_COLOR_GREEN;
        state->led_adrenaline_blue = DEFAULT_ADR_COLOR_BLUE;
    }
}

void cpr_helper_face_activate(void *context) {
    if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();

    cpr_helper_state_t *state = (cpr_helper_state_t *)context;

    saved_led_color = movement_backlight_color();
    saved_dwell = movement_get_backlight_dwell();

    movement_set_backlight_dwell(0);
    if (state->running) {
        movement_schedule_background_task(distant_future);
    }
}

bool cpr_helper_face_loop(movement_event_t event, void *context) {
    cpr_helper_state_t *state = (cpr_helper_state_t *)context;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            state->in_settings_mode = false;
            watch_set_colon();
            print_adrenaline_count(state);
            print_shock_count(state);
            if (state->timestamp_count > 0 && !state->running) {
                state->in_timestamp_view = true;
                review_screen(state);
                indicators(state);
                state->last_displayed_adrenaline_count = state->adrenaline_counts[state->timestamp_index];
                state->last_displayed_shock_count = state->shock_counts[state->timestamp_index];
            }
            //fall through
        case EVENT_TICK:
            if (state->in_timestamp_view || state->in_settings_mode){
                break;
            } else if (state->start_time.reg == 0) {
                watch_display_text(WATCH_POSITION_BOTTOM, "0000");
                movement_force_led_off();
            } else {
                timer_screen(state);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->running && !state->in_timestamp_view) {
                adrenaline_count_increment(state);
            } else if (!state->running && state->in_timestamp_view) {
                state->timestamp_index++;
                review_screen(state);
            } else if (state->in_settings_mode && !state->in_timestamp_view) {
                settings_increment(state);
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            watch_set_led_off();
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if (state->running && !state->in_timestamp_view) {
                shock_count_increment(state);
            } else if (!state->running && state->in_timestamp_view && !state->in_settings_mode) {
                state->show_elapsed_time_in_review = !state->show_elapsed_time_in_review;
                toggle_timestamp(state);
            } else if (state->in_settings_mode) {
                settings_index_advance(state);
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (!state->running) {
                movement_set_backlight_color((movement_color_t){ .red = 0, .green = 4, .blue = 0 });
                movement_illuminate_led();
                reset_state(state);
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            timer_start(state);
            break;
        case EVENT_MODE_LONG_PRESS:
            toggle_settings(state);
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void cpr_helper_face_resign(void *context) {
    (void) context;

    movement_set_backlight_color(saved_led_color);
    movement_set_backlight_dwell(saved_dwell);
    movement_force_led_off();
    movement_cancel_background_task();
}
