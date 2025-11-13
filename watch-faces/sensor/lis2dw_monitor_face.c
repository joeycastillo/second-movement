/*
 * MIT License
 *
 * Copyright (c) 2025 Konrad Rieck
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
#include "lis2dw_monitor_face.h"
#include "watch.h"
#include "watch_utility.h"

/* Display frequency */
#define DISPLAY_FREQUENCY 8

/* Settings */
#define NUM_SETTINGS 7

static void _settings_title_display(lis2dw_monitor_state_t *state, char *buf1, char *buf2)
{
    char buf[10];
    watch_display_text_with_fallback(WATCH_POSITION_TOP, buf1, buf2);
    if (watch_get_lcd_type() != WATCH_LCD_TYPE_CUSTOM) {
        snprintf(buf, sizeof(buf), "%2d", state->settings_page + 1);
        watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, buf, buf);
    }
}

static bool _settings_blink(uint8_t subsecond)
{
    if (subsecond % 2 == 0) {
        watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "      ", "      ");
        return true;
    }
    return false;
}

static void _settings_mode_display(void *context, uint8_t subsecond)
{
    char buf[10];
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    _settings_title_display(state, "MODE ", "MO");
    if (_settings_blink(subsecond))
        return;

    switch (state->ds.mode) {
        case LIS2DW_MODE_LOW_POWER:
            snprintf(buf, sizeof(buf), "  LO  ");
            break;
        case LIS2DW_MODE_HIGH_PERFORMANCE:
            snprintf(buf, sizeof(buf), "  HI  ");
            break;
        case LIS2DW_MODE_ON_DEMAND:
            snprintf(buf, sizeof(buf), "  OD  ");
            break;
    }
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_mode_advance(void *context)
{
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    switch (state->ds.mode) {
        case LIS2DW_MODE_LOW_POWER:
            state->ds.mode = LIS2DW_MODE_HIGH_PERFORMANCE;
            break;
        case LIS2DW_MODE_HIGH_PERFORMANCE:
            state->ds.mode = LIS2DW_MODE_ON_DEMAND;
            break;
        case LIS2DW_MODE_ON_DEMAND:
            state->ds.mode = LIS2DW_MODE_LOW_POWER;
            break;
    }
}

static void _settings_data_rate_display(void *context, uint8_t subsecond)
{
    char buf[10];
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    _settings_title_display(state, "RATE ", "DR");
    if (_settings_blink(subsecond))
        return;

    switch (state->ds.data_rate) {
        case LIS2DW_DATA_RATE_POWERDOWN:
            snprintf(buf, sizeof(buf), "  --  ");
            break;
        case LIS2DW_DATA_RATE_LOWEST:
            snprintf(buf, sizeof(buf), "  LO  ");
            break;
        case LIS2DW_DATA_RATE_12_5_HZ:
            snprintf(buf, sizeof(buf), "  12Hz");
            break;
        case LIS2DW_DATA_RATE_25_HZ:
            snprintf(buf, sizeof(buf), "  25Hz");
            break;
        default:
            snprintf(buf, sizeof(buf), "  HI  ");
            break;
    }
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_data_rate_advance(void *context)
{
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    switch (state->ds.data_rate) {
        case LIS2DW_DATA_RATE_POWERDOWN:
            state->ds.data_rate = LIS2DW_DATA_RATE_LOWEST;
            break;
        case LIS2DW_DATA_RATE_LOWEST:
            state->ds.data_rate = LIS2DW_DATA_RATE_12_5_HZ;
            break;
        case LIS2DW_DATA_RATE_12_5_HZ:
            state->ds.data_rate = LIS2DW_DATA_RATE_25_HZ;
            break;
        case LIS2DW_DATA_RATE_25_HZ:
            state->ds.data_rate = LIS2DW_DATA_RATE_POWERDOWN;
            break;
        default:
            state->ds.data_rate = LIS2DW_DATA_RATE_POWERDOWN;
            break;
    }
}

static void _settings_low_power_display(void *context, uint8_t subsecond)
{
    char buf[10];
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    _settings_title_display(state, "LO PM", "LP");
    if (_settings_blink(subsecond))
        return;

    switch (state->ds.low_power) {
        case LIS2DW_LP_MODE_1:
            snprintf(buf, sizeof(buf), " L1 12");
            break;
        case LIS2DW_LP_MODE_2:
            snprintf(buf, sizeof(buf), " L2 14");
            break;
        case LIS2DW_LP_MODE_3:
            snprintf(buf, sizeof(buf), " L3 14");
            break;
        case LIS2DW_LP_MODE_4:
            snprintf(buf, sizeof(buf), " L4 14");
            break;
    }
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_low_power_advance(void *context)
{
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    switch (state->ds.low_power) {
        case LIS2DW_LP_MODE_1:
            state->ds.low_power = LIS2DW_LP_MODE_2;
            break;
        case LIS2DW_LP_MODE_2:
            state->ds.low_power = LIS2DW_LP_MODE_3;
            break;
        case LIS2DW_LP_MODE_3:
            state->ds.low_power = LIS2DW_LP_MODE_4;
            break;
        case LIS2DW_LP_MODE_4:
            state->ds.low_power = LIS2DW_LP_MODE_1;
            break;
    }
}

static void _settings_bwf_mode_display(void *context, uint8_t subsecond)
{
    char buf[10];
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    _settings_title_display(state, "BWF  ", "BW");
    if (_settings_blink(subsecond))
        return;

    switch (state->ds.bwf_mode) {
        case LIS2DW_BANDWIDTH_FILTER_DIV2:
            snprintf(buf, sizeof(buf), "   2  ");
            break;
        case LIS2DW_BANDWIDTH_FILTER_DIV4:
            snprintf(buf, sizeof(buf), "   4  ");
            break;
        case LIS2DW_BANDWIDTH_FILTER_DIV10:
            snprintf(buf, sizeof(buf), "  10  ");
            break;
        case LIS2DW_BANDWIDTH_FILTER_DIV20:
            snprintf(buf, sizeof(buf), "  20  ");
            break;
    }
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_bwf_mode_advance(void *context)
{
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    switch (state->ds.bwf_mode) {
        case LIS2DW_BANDWIDTH_FILTER_DIV2:
            state->ds.bwf_mode = LIS2DW_BANDWIDTH_FILTER_DIV4;
            break;
        case LIS2DW_BANDWIDTH_FILTER_DIV4:
            state->ds.bwf_mode = LIS2DW_BANDWIDTH_FILTER_DIV10;
            break;
        case LIS2DW_BANDWIDTH_FILTER_DIV10:
            state->ds.bwf_mode = LIS2DW_BANDWIDTH_FILTER_DIV20;
            break;
        case LIS2DW_BANDWIDTH_FILTER_DIV20:
            state->ds.bwf_mode = LIS2DW_BANDWIDTH_FILTER_DIV2;
            break;
    }
}

static void _settings_range_display(void *context, uint8_t subsecond)
{
    char buf[10];
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    _settings_title_display(state, "RANGE", "RA");
    if (_settings_blink(subsecond))
        return;

    switch (state->ds.range) {
        case LIS2DW_RANGE_2_G:
            snprintf(buf, sizeof(buf), "   2g ");
            break;
        case LIS2DW_RANGE_4_G:
            snprintf(buf, sizeof(buf), "   4g ");
            break;
        case LIS2DW_RANGE_8_G:
            snprintf(buf, sizeof(buf), "   8g ");
            break;
        case LIS2DW_RANGE_16_G:
            snprintf(buf, sizeof(buf), "  16g ");
            break;
    }
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_range_advance(void *context)
{
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    switch (state->ds.range) {
        case LIS2DW_RANGE_2_G:
            state->ds.range = LIS2DW_RANGE_4_G;
            break;
        case LIS2DW_RANGE_4_G:
            state->ds.range = LIS2DW_RANGE_8_G;
            break;
        case LIS2DW_RANGE_8_G:
            state->ds.range = LIS2DW_RANGE_16_G;
            break;
        case LIS2DW_RANGE_16_G:
            state->ds.range = LIS2DW_RANGE_2_G;
            break;
    }
}

static void _settings_filter_display(void *context, uint8_t subsecond)
{
    char buf[10];
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    _settings_title_display(state, "FLT  ", "FL");
    if (_settings_blink(subsecond))
        return;

    switch (state->ds.filter) {
        case LIS2DW_FILTER_LOW_PASS:
            snprintf(buf, sizeof(buf), "  LP  ");
            break;
        case LIS2DW_FILTER_HIGH_PASS:
            snprintf(buf, sizeof(buf), "  HP  ");
            break;
    }
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_filter_advance(void *context)
{
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    switch (state->ds.filter) {
        case LIS2DW_FILTER_LOW_PASS:
            state->ds.filter = LIS2DW_FILTER_HIGH_PASS;
            break;
        case LIS2DW_FILTER_HIGH_PASS:
            state->ds.filter = LIS2DW_FILTER_LOW_PASS;
            break;
    }
}

static void _settings_low_noise_display(void *context, uint8_t subsecond)
{
    char buf[10];
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    _settings_title_display(state, "LO NO", "LN");
    if (_settings_blink(subsecond))
        return;

    snprintf(buf, sizeof(buf), " %3s  ", state->ds.low_noise ? "ON" : "OFF");
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_low_noise_advance(void *context)
{
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;
    state->ds.low_noise = !state->ds.low_noise;
}

/* Play beep sound */
static inline void _beep()
{
    if (!movement_button_should_sound())
        return;
    watch_buzzer_play_note(BUZZER_NOTE_C7, 50);
}

/* Print lis2dw status to console. */
static void _lis2dw_print_state(lis2dw_device_state_t *ds)
{
    printf("LIS2DW status:\n");
    printf("  Power mode:\t%x\n", ds->mode);
    printf("  Data rate:\t%x\n", ds->data_rate);
    printf("  LP mode:\t%x\n", ds->low_power);
    printf("  BW filter:\t%x\n", ds->bwf_mode);
    printf("  Range:\t%x \n", ds->range);
    printf("  Filter type:\t%x\n", ds->filter);
    printf("  Low noise:\t%x\n", ds->low_noise);
    printf("\n");
}

static void _lis2dw_get_state(lis2dw_device_state_t *ds)
{
    ds->mode = lis2dw_get_mode();
    ds->data_rate = lis2dw_get_data_rate();
    ds->low_power = lis2dw_get_low_power_mode();
    ds->bwf_mode = lis2dw_get_bandwidth_filtering();
    ds->range = lis2dw_get_range();
    ds->filter = lis2dw_get_filter_type();
    ds->low_noise = lis2dw_get_low_noise_mode();
}

static void _lis2dw_set_state(lis2dw_device_state_t *ds)
{
    lis2dw_set_mode(ds->mode);
    lis2dw_set_data_rate(ds->data_rate);
    lis2dw_set_low_power_mode(ds->low_power);
    lis2dw_set_bandwidth_filtering(ds->bwf_mode);
    lis2dw_set_range(ds->range);
    lis2dw_set_filter_type(ds->filter);
    lis2dw_set_low_noise_mode(ds->low_noise);

    /* Additionally, set the background rate to the data rate. */
    movement_set_accelerometer_background_rate(ds->data_rate);
}

static void _monitor_display(lis2dw_monitor_state_t *state)
{
    char buf[10];

    snprintf(buf, sizeof(buf), " %C ", "XYZ"[state->axis]);
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, buf, buf);

    snprintf(buf, sizeof(buf), "%2d", state->axis + 1);
    watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, buf, buf);

    if (state->show_title) {
        snprintf(buf, sizeof(buf), "LIS2DW");
        watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
        return;
    }

    if (state->ds.data_rate == LIS2DW_DATA_RATE_POWERDOWN) {
        /* No measurements available. */
        snprintf(buf, sizeof(buf), "  --  ");
    } else if (state->axis == 0) {
        char sign = (state->reading.x) >= 0 ? ' ' : '-';
        snprintf(buf, sizeof(buf), "%c%.5d", sign, abs(state->reading.x));
    } else if (state->axis == 1) {
        char sign = (state->reading.y) >= 0 ? ' ' : '-';
        snprintf(buf, sizeof(buf), "%c%.5d", sign, abs(state->reading.y));
    } else {
        char sign = (state->reading.z) >= 0 ? ' ' : '-';
        snprintf(buf, sizeof(buf), "%c%.5d", sign, abs(state->reading.z));
    }
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _monitor_update(lis2dw_monitor_state_t *state)
{
    lis2dw_fifo_t fifo;
    float x = 0, y = 0, z = 0;

    lis2dw_read_fifo(&fifo, LIS2DW_FIFO_TIMEOUT / DISPLAY_FREQUENCY);
    if (fifo.count == 0) {
        return;
    }

    /* Add up samples in fifo */
    for (uint8_t i = 0; i < fifo.count; i++) {
        x += fifo.readings[i].x;
        y += fifo.readings[i].y;
        z += fifo.readings[i].z;
    }

    /* Divide by number of samples */
    state->reading.x = (int16_t) (x / fifo.count);
    state->reading.y = (int16_t) (y / fifo.count);
    state->reading.z = (int16_t) (z / fifo.count);

    lis2dw_clear_fifo();
}

static void _switch_to_monitor(lis2dw_monitor_state_t *state)
{
    /* Switch to recording page */
    movement_request_tick_frequency(DISPLAY_FREQUENCY);
    state->page = PAGE_LIS2DW_MONITOR;
    state->show_title = DISPLAY_FREQUENCY;
    _monitor_display(state);
}

static void _switch_to_settings(lis2dw_monitor_state_t *state)
{
    /* Switch to chirping page */
    movement_request_tick_frequency(4);
    state->page = PAGE_LIS2DW_SETTINGS;
    state->settings_page = 0;
    state->settings[state->settings_page].display(state, 0);
}

static bool _monitor_loop(movement_event_t event, void *context)
{
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_clear_colon();
            _monitor_update(state);
            _monitor_display(state);
            break;
        case EVENT_TICK:
            _monitor_update(state);
            _monitor_display(state);
            state->show_title = (state->show_title > 0) ? state->show_title - 1 : 0;
            break;
        case EVENT_ALARM_BUTTON_UP:
            state->axis = (state->axis + 1) % 3;
            _monitor_display(state);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            /* Do nothing. */
            break;
        case EVENT_LIGHT_LONG_PRESS:
            _switch_to_settings(state);
            _beep();
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

static bool _settings_loop(movement_event_t event, void *context)
{
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            state->settings[state->settings_page].display(context, event.subsecond);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            state->settings_page = (state->settings_page + 1) % NUM_SETTINGS;
            state->settings[state->settings_page].display(context, event.subsecond);
            break;
        case EVENT_MODE_BUTTON_UP:
            _lis2dw_set_state(&state->ds);
            _lis2dw_print_state(&state->ds);
            _switch_to_monitor(state);
            _beep();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            /* Do nothing. */
            break;
        case EVENT_ALARM_BUTTON_UP:
            /* Advance current settings */
            state->settings[state->settings_page].advance(context);
            state->settings[state->settings_page].display(context, event.subsecond);
            break;
        default:
            _lis2dw_set_state(&state->ds);
            movement_default_loop_handler(event);
            break;
    }
    return true;
}

void lis2dw_monitor_face_setup(uint8_t watch_face_index, void **context_ptr)
{
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(lis2dw_monitor_state_t));
        memset(*context_ptr, 0, sizeof(lis2dw_monitor_state_t));
    }
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) * context_ptr;

    /* Default setup */
    state->axis = 0;

    /* Initialize settings */
    uint8_t settings_page = 0;
    state->settings = malloc(NUM_SETTINGS * sizeof(lis2dw_settings_t));
    state->settings[settings_page].display = _settings_mode_display;
    state->settings[settings_page].advance = _settings_mode_advance;
    settings_page++;
    state->settings[settings_page].display = _settings_data_rate_display;
    state->settings[settings_page].advance = _settings_data_rate_advance;
    settings_page++;
    state->settings[settings_page].display = _settings_low_power_display;
    state->settings[settings_page].advance = _settings_low_power_advance;
    settings_page++;
    state->settings[settings_page].display = _settings_bwf_mode_display;
    state->settings[settings_page].advance = _settings_bwf_mode_advance;
    settings_page++;
    state->settings[settings_page].display = _settings_range_display;
    state->settings[settings_page].advance = _settings_range_advance;
    settings_page++;
    state->settings[settings_page].display = _settings_filter_display;
    state->settings[settings_page].advance = _settings_filter_advance;
    settings_page++;
    state->settings[settings_page].display = _settings_low_noise_display;
    state->settings[settings_page].advance = _settings_low_noise_advance;
    settings_page++;
}

void lis2dw_monitor_face_activate(void *context)
{
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    /* Setup lis2dw to run in background at 12.5 Hz sampling rate. */
    movement_set_accelerometer_background_rate(LIS2DW_DATA_RATE_12_5_HZ);

    /* Enable fifo and clear it. */
    lis2dw_enable_fifo();
    lis2dw_clear_fifo();

    /* Print lis2dw status to console. */
    _lis2dw_get_state(&state->ds);
    _lis2dw_print_state(&state->ds);

    /* Switch to monitor page. */
    _switch_to_monitor(state);
}

bool lis2dw_monitor_face_loop(movement_event_t event, void *context)
{
    lis2dw_monitor_state_t *state = (lis2dw_monitor_state_t *) context;

    switch (state->page) {
        default:
        case PAGE_LIS2DW_MONITOR:
            return _monitor_loop(event, context);
        case PAGE_LIS2DW_SETTINGS:
            return _settings_loop(event, context);
    }
}

void lis2dw_monitor_face_resign(void *context)
{
    (void) context;
    lis2dw_clear_fifo();
    lis2dw_disable_fifo();
}

movement_watch_face_advisory_t lis2dw_monitor_face_advise(void *context)
{
    (void) context;
    movement_watch_face_advisory_t retval = { 0 };
    return retval;
}
