/*
 * MIT License
 *
 * Copyright (c) 2025 Moritz Glöckl
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
#include "calendar_face.h"
#include "watch_utility.h"
#include "calendar_face_holidays.h"

// Example: Fill with holidays for Austria and pan-European holidays
// Checkout calendar_face_holidays.h for more holidays and examples
static const uint16_t holidays[] = {
    HOLIDAY_NEW_YEARS_DAY,            // Jan 1
    HOLIDAY_AT_EPIPHANY,              // Jan 6
    HOLIDAY_GOOD_FRIDAY,              // Dynamic (Easter - 2)
    HOLIDAY_EASTER_SUNDAY,            // Dynamic (Easter)
    HOLIDAY_EASTER_MONDAY,            // Dynamic (Easter + 1)
    HOLIDAY_AT_ASSUMPTION_DAY,        // Aug 15
    HOLIDAY_MAY_DAY,                  // May 1
    HOLIDAY_ASCENSION_DAY,            // Dynamic (Easter + 39)
    HOLIDAY_PENTECOST_MONDAY,         // Dynamic (Easter + 50)
    HOLIDAY_CORPUS_CHRISTI,           // Dynamic (Easter + 60)
    HOLIDAY_AT_NATIONAL_DAY,          // Oct 26
    HOLIDAY_AT_IMMACULATE_CONCEPTION, // Dec 8
    HOLIDAY_CHRISTMAS_DAY,            // Dec 25
    HOLIDAY_BOXING_DAY                // Dec 26
};
#define HOLIDAY_COUNT (sizeof(holidays) / sizeof(holidays[0]))

static void reset_state(calendar_state_t *state)
{
    state->scrolling_mode = 0;
    state->blink_state = 0;
    state->scroll_tick_count = 0;
}
static inline uint8_t days_in_month(uint8_t m, uint8_t y)
{
    static const uint8_t month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint16_t year = y + 2020;
    if (m == 2)
    {
        bool leap = ((year & 3) == 0) && ((year % 100 != 0) || (year % 400 == 0));
        return 28 + leap;
    }
    return month_days[m - 1];
}

static void display_calendar(uint8_t m, uint8_t d, uint8_t y, bool show_holiday)
{
    static const char months[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static const char weekdays[7][3] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, months[m - 1], months[m - 1]);
    char year_buf[4];
    year_buf[0] = '0' + ((y + 20) / 10);
    year_buf[1] = '0' + ((y + 20) % 10);
    year_buf[2] = '\0';
    watch_display_text(WATCH_POSITION_TOP_RIGHT, year_buf);
    char main_buf[8];
    uint8_t wd = watch_utility_get_iso8601_weekday_number(y + 2020, m, d);
    main_buf[0] = (d < 10) ? ' ' : '0' + (d / 10);
    main_buf[1] = '0' + (d % 10);
    main_buf[2] = '-';
    main_buf[3] = weekdays[(wd == 7) ? 0 : wd][0];
    main_buf[4] = weekdays[(wd == 7) ? 0 : wd][1];
    main_buf[5] = '\0';
    watch_display_text(WATCH_POSITION_BOTTOM, main_buf);
    if (show_holiday)
        watch_set_indicator(WATCH_INDICATOR_BELL);
    else
        watch_clear_indicator(WATCH_INDICATOR_BELL);
}

static void scroll_day_forward(calendar_state_t *s)
{
    s->scroll_day++;
    if (s->scroll_day > days_in_month(s->scroll_month, s->scroll_year))
    {
        s->scroll_day = 1;
        s->scroll_month++;
        if (s->scroll_month > 12)
        {
            s->scroll_month = 1;
            s->scroll_year = (s->scroll_year + 1) % 100; // wrap year at 100
        }
    }
}
static void scroll_day_backward(calendar_state_t *s)
{
    if (s->scroll_day > 1)
    {
        s->scroll_day--;
    }
    else
    {
        s->scroll_month--;
        if (s->scroll_month < 1)
        {
            s->scroll_month = 12;
            s->scroll_year = (s->scroll_year == 0) ? 99 : s->scroll_year - 1;
        }
        s->scroll_day = days_in_month(s->scroll_month, s->scroll_year);
    }
}
static void scroll_month_forward(calendar_state_t *s)
{
    s->scroll_month++;
    if (s->scroll_month > 12)
    {
        s->scroll_month = 1;
        s->scroll_year = (s->scroll_year + 1) % 100; // wrap year at 100
    }
}
static void scroll_month_backward(calendar_state_t *s)
{
    if (s->scroll_month > 1)
    {
        s->scroll_month--;
    }
    else
    {
        s->scroll_month = 12;
        s->scroll_year = (s->scroll_year == 0) ? 99 : s->scroll_year - 1;
    }
}
static void scroll_year_forward(calendar_state_t *s) { s->scroll_year = (s->scroll_year + 1) % 100; /* % 100, but faster */ }
static void scroll_year_backward(calendar_state_t *s) { s->scroll_year = (s->scroll_year == 0) ? 99 : s->scroll_year - 1; }

static void (*scroll_forward[3])(calendar_state_t *) = {scroll_day_forward, scroll_month_forward, scroll_year_forward};
static void (*scroll_backward[3])(calendar_state_t *) = {scroll_day_backward, scroll_month_backward, scroll_year_backward};

void calendar_face_setup(uint8_t watch_face_index, void **context_ptr)
{
    (void)watch_face_index;
    if (*context_ptr == NULL)
    {
        *context_ptr = calloc(1, sizeof(calendar_state_t));
    }
}

void calendar_face_activate(void *context)
{
    calendar_state_t *state = (calendar_state_t *)context;
    movement_request_tick_frequency(4);
    reset_state(state);
}

bool calendar_face_loop(movement_event_t event, void *context)
{
    calendar_state_t *state = (calendar_state_t *)context;
    static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static const char *weekdays[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    switch (event.event_type)
    {
    case EVENT_MODE_BUTTON_UP:
        if ((state->scrolling_mode))
            state->scroll_mode = (state->scroll_mode + 1) % 3;
        else
            return movement_default_loop_handler(event);
        break;
    case EVENT_MODE_LONG_PRESS:
        if ((state->scrolling_mode))
        {
            reset_state(state);
        }
        else
        {
            movement_move_to_face(0);
        }
        break;
    case EVENT_ACTIVATE:
        watch_clear_display();
        reset_state(state);
        break;
    case EVENT_TICK:
    {
        watch_date_time_t now = watch_rtc_get_date_time();
        uint8_t m, d, y;
        bool show_holiday;
        if (!(state->scrolling_mode))
        {
            m = now.unit.month;
            d = now.unit.day;
            y = now.unit.year;
            show_holiday = is_public_holiday(m, d, y + 2020, holidays, HOLIDAY_COUNT);
            display_calendar(m, d, y, show_holiday);
            movement_request_tick_frequency(1);
        }
        else
        {
            m = state->scroll_month;
            d = state->scroll_day;
            y = state->scroll_year;
            state->blink_state = !(state->blink_state);
            bool blink = state->blink_state;
            bool show_month = !(state->scroll_mode == 1 && !blink);
            bool show_year = !(state->scroll_mode == 2 && !blink);
            bool show_day = !(state->scroll_mode == 0 && !blink);

            show_holiday = is_public_holiday(m, d, y + 2020, holidays, HOLIDAY_COUNT);
            if (show_month && show_year && show_day)
                display_calendar(m, d, y, show_holiday);
            else
            {
                if (!show_month)
                    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "   ", "   ");
                else
                    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, months[m - 1], months[m - 1]);
                if (!show_year)
                    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
                else
                {
                    char year_buf[4];
                    snprintf(year_buf, sizeof(year_buf), "%02d", (y + 20));
                    watch_display_text(WATCH_POSITION_TOP_RIGHT, year_buf);
                }
                if (!show_day)
                    watch_display_text(WATCH_POSITION_BOTTOM, "     ");
                else
                {
                    char main_buf[8];
                    uint8_t wd = watch_utility_get_iso8601_weekday_number(y + 2020, m, d);
                    snprintf(main_buf, sizeof(main_buf), "%2d-%s", d, weekdays[(wd == 7) ? 0 : wd]);
                    watch_display_text(WATCH_POSITION_BOTTOM, main_buf);
                }
                if (show_holiday)
                    watch_set_indicator(WATCH_INDICATOR_BELL);
                else
                    watch_clear_indicator(WATCH_INDICATOR_BELL);
            }

            movement_request_tick_frequency(4);
        }
        break;
    }
    case EVENT_LIGHT_BUTTON_UP:
        watch_set_led_off();
        scroll_backward[state->scroll_mode](state);
        break;
    case EVENT_LIGHT_LONG_PRESS:
        watch_set_led_off();
        for (int i = 0, n = (state->scroll_mode == 0) ? 7 : 3; i < n; i++)
            scroll_backward[state->scroll_mode](state);
        break;
    case EVENT_ALARM_BUTTON_UP:
        scroll_forward[state->scroll_mode](state);
        break;
    case EVENT_ALARM_LONG_PRESS:
        if (!state->scrolling_mode)
        {
            watch_date_time_t now = watch_rtc_get_date_time();
            state->scrolling_mode = true;
            state->scroll_day = now.unit.day;
            state->scroll_month = now.unit.month;
            state->scroll_year = now.unit.year;
            state->blink_state = true;
            state->scroll_tick_count = 0;
            state->scroll_mode = 0;
        }
        else
        {
            for (int i = 0, n = (state->scroll_mode == 0) ? 7 : 3; i < n; i++)
                scroll_forward[state->scroll_mode](state);
        }
        break;
    case EVENT_TIMEOUT:
        reset_state(state);
        movement_move_to_face(0);
        break;
    case EVENT_LOW_ENERGY_UPDATE:
    {
        watch_date_time_t now = watch_rtc_get_date_time();
        uint8_t m = now.unit.month;
        uint8_t d = now.unit.day;
        uint8_t y = now.unit.year;
        bool show_holiday = is_public_holiday(m, d, y + 2020, holidays, HOLIDAY_COUNT);
        display_calendar(m, d, y, show_holiday);
        break;
    }
    default:
        return movement_default_loop_handler(event);
    }
    return true;
}

void calendar_face_resign(void *context)
{
    calendar_state_t *state = (calendar_state_t *)context;
    if (state->scrolling_mode)
        reset_state(state);
}
