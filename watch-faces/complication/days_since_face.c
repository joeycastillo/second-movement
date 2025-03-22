/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
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
#include "days_since_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "filesystem.h"

static int days_since_instances;

static void persist_date(days_since_state_t *state) {
    days_since_date_t maybe_date;
    days_since_date_t current_date = {0};
    current_date.bit.year = state->working_year;
    current_date.bit.month = state->working_month;
    current_date.bit.day = state->working_day;

    char filename[13];

    maybe_date.reg = 0xFFFFFFFF;
    sprintf(filename, "since%03d.u32", state->face_index);

    filesystem_read_file(filename, (char *) &maybe_date.reg, sizeof(days_since_date_t));
    if (current_date.reg != maybe_date.reg) {
        filesystem_write_file(filename, (char *) &current_date.reg, sizeof(days_since_date_t));
    }
}

static uint32_t _days_since_face_juliandaynum(uint16_t year, uint16_t month, uint16_t day) {
    // from here: https://en.wikipedia.org/wiki/Julian_day#Julian_day_number_calculation
    return (1461 * (year + 4800 + (month - 14) / 12)) / 4 + (367 * (month - 2 - 12 * ((month - 14) / 12))) / 12 - (3 * ((year + 4900 + (month - 14) / 12) / 100))/4 + day - 32075;
}

static void _days_since_face_update(days_since_state_t *state) {
    char buf[15];
    watch_date_time_t date_time = watch_rtc_get_date_time();
    uint32_t julian_now_date = _days_since_face_juliandaynum(date_time.unit.year + WATCH_RTC_REFERENCE_YEAR, date_time.unit.month, date_time.unit.day);
    uint32_t julian_since_date = _days_since_face_juliandaynum(state->working_year, state->working_month, state->working_day);
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "DAY", "DA");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    if (julian_now_date < julian_since_date) {
        sprintf(buf, "%6lu", julian_since_date - julian_now_date);
    } else {
        sprintf(buf, "%6lu", julian_now_date - julian_since_date);
    }
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static void _days_since_face_abort_quick_cycle(days_since_state_t *state) {
    if (state->quick_cycle) {
        state->quick_cycle = false;
        movement_request_tick_frequency(4);
    }
}

static void _days_since_face_increment(days_since_state_t *state) {
    state->birthday_changed = true;
    switch (state->current_page) {
        case PAGE_YEAR:
            state->working_year = state->working_year + 1;
            if (state->working_year > 2080) state->working_year = 1900;
            break;
        case PAGE_MONTH:
            state->working_month = (state->working_month % 12) + 1;
            break;
        case PAGE_DAY:
            state->working_day = (state->working_day % watch_utility_days_in_month(state->working_month, state->working_year)) + 1;
            break;
        default:
            break;
    }
}

void days_since_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(days_since_state_t));
        memset(*context_ptr, 0, sizeof(days_since_state_t));
        days_since_date_t since_date = {0};
        days_since_state_t *state = (days_since_state_t *)*context_ptr;
        state->face_index = days_since_instances++;

        // load date from file if it exists
        char filename[13];
        sprintf(filename, "since%03d.u32", state->face_index);
        if (filesystem_file_exists(filename)) {
            filesystem_read_file(filename, (char *) &since_date, sizeof(days_since_date_t));
        } else {
            // if birth date is not set, set a reasonable starting date. this works well for anyone under 65, but
            // you can keep pressing to go back to 1900; just go past the year 2080.
            since_date.bit.year = 1959;
            since_date.bit.month = 1;
            since_date.bit.day = 1;
        }

        state->working_year = since_date.bit.year;
        state->working_month = since_date.bit.month;
        state->working_day = since_date.bit.day;
    }
}

void days_since_face_activate(void *context) {
    days_since_state_t *state = (days_since_state_t *)context;

    state->current_page = PAGE_DISPLAY;
    state->quick_cycle = false;
    state->ticks = 0;
}

bool days_since_face_loop(movement_event_t event, void *context) {
    days_since_state_t *state = (days_since_state_t *)context;

    char buf[9];

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _days_since_face_update(state);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
        case EVENT_TICK:
            if (state->quick_cycle) {
                if (HAL_GPIO_BTN_ALARM_read()) {
                    _days_since_face_increment(state);
                } else {
                    _days_since_face_abort_quick_cycle(state);
                }
            }
            switch (state->current_page) {
                // if in settings mode, update whatever the current page is
                case PAGE_YEAR:
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "Year ", "YR");
                    if (event.subsecond % 2) {
                        sprintf(buf, "%4d  ", state->working_year);
                        watch_display_text(WATCH_POSITION_BOTTOM, buf);
                    } else {
                        watch_display_text(WATCH_POSITION_BOTTOM, "      ");
                    }
                    break;
                case PAGE_MONTH:
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "Month", "MO");
                    if (event.subsecond % 2) {
                        sprintf(buf, "%2d    ", state->working_month);
                        watch_display_text(WATCH_POSITION_BOTTOM, buf);
                    } else {
                        watch_display_text(WATCH_POSITION_BOTTOM, "      ");
                    }
                    break;
                case PAGE_DAY:
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "Day  ", "DA");
                    if (event.subsecond % 2) {
                        sprintf(buf, "  %2d  ", state->working_day);
                        watch_display_text(WATCH_POSITION_BOTTOM, buf);
                    } else {
                        watch_display_text(WATCH_POSITION_BOTTOM, "      ");
                    }
                    break;
                // otherwise, check if we have to update. the display only needs to change at midnight!
                case PAGE_DISPLAY: {
                    watch_date_time_t date_time = watch_rtc_get_date_time();
                    if (date_time.unit.hour == 0 &&  date_time.unit.minute == 0 && date_time.unit.second == 0) {
                        _days_since_face_update(state);
                    }
                    break;}
                case PAGE_DATE:
                    if (state->ticks > 0) {
                        state->ticks--;
                    } else {
                        state->current_page = PAGE_DISPLAY;
                        _days_since_face_update(state);
                    }
                    break;
                default:
                    break;
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            // only illuminate if we're in display mode
            switch (state->current_page) {
                case PAGE_DISPLAY:
                    // fall through
                case PAGE_DATE:
                    movement_illuminate_led();
                    break;
                default:
                    break;
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // otherwise use the light button to advance settings pages.
            switch (state->current_page) {
                case PAGE_YEAR:
                    // fall through
                case PAGE_MONTH:
                    // fall through
                case PAGE_DAY:
                    // go to next setting page...
                    state->current_page = (state->current_page + 1) % 4;
                    if (state->current_page == PAGE_DISPLAY) {
                        // ...unless we've been pushed back to display mode.
                        movement_request_tick_frequency(1);
                        // save the date if it changed
                        persist_date(state);
                        // and force display since it normally won't update til midnight.
                        _days_since_face_update(state);
                    }
                    break;
                default:
                    break;
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            // if we are on a settings page, increment whatever value we're setting.
            switch (state->current_page) {
                case PAGE_YEAR:
                    // fall through
                case PAGE_MONTH:
                    // fall through
                case PAGE_DAY:
                    _days_since_face_abort_quick_cycle(state);
                    _days_since_face_increment(state);
                    break;
                case PAGE_DISPLAY:
                {
                    watch_date_time_t date_time = watch_rtc_get_date_time();
                    uint32_t julian_now_date = _days_since_face_juliandaynum(date_time.unit.year + WATCH_RTC_REFERENCE_YEAR, date_time.unit.month, date_time.unit.day);
                    uint32_t julian_since_date = _days_since_face_juliandaynum(state->working_year, state->working_month, state->working_day);
                    if (julian_now_date < julian_since_date) {
                        watch_display_text_with_fallback(WATCH_POSITION_TOP, "Until", "DA");
                    } else {
                        watch_display_text_with_fallback(WATCH_POSITION_TOP, "SINCE", "DA");
                    }
                    state->current_page = PAGE_DATE;
                    sprintf(buf, "%02d%02d%02d", state->working_year % 100, state->working_month % 100, state->working_day % 100);
                    watch_display_text(WATCH_POSITION_BOTTOM, buf);
                    state->ticks = 2;
                }
                    break;
                default:
                    break;
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            // if we aren't already in settings mode, put us there.
            switch (state->current_page) {
                case PAGE_DISPLAY:
                    state->current_page++;
                    movement_request_tick_frequency(4);
                    break;
                case PAGE_YEAR:
                    // fall through
                case PAGE_MONTH:
                    // fall through
                case PAGE_DAY:
                    state->quick_cycle = true;
                    movement_request_tick_frequency(8);
                    break;
                default:
                    break;
            }
            break;
        case EVENT_ALARM_LONG_UP:
            _days_since_face_abort_quick_cycle(state);
            break;
        case EVENT_TIMEOUT:
            _days_since_face_abort_quick_cycle(state);
            // return home if we're on a settings page (this saves our changes when we resign).
            if (state->current_page != PAGE_DISPLAY) {
                movement_move_to_face(0);
            }
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void days_since_face_resign(void *context) {
    days_since_state_t *state = (days_since_state_t *)context;

    // if the user changed their birth date, store it to the birth date register
    if (state->birthday_changed) {
        days_since_state_t *state = (days_since_state_t *)context;
        persist_date(state);
        state->birthday_changed = false;
    }
}
