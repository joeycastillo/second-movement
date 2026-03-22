/*
 * MIT License
 *
 * Copyright (c) 2024 <David Volovskiy>
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
#include "festival_schedule_face.h"
#include "festival_schedule_arr.h"
#include "watch_utility.h"

const char festival_name[2] = "BO";

const char festival_stage[FESTIVAL_SCHEDULE_STAGE_COUNT + 1][2] =
{
    [FESTIVAL_SCHEDULE_NO_STAGE]    = "  ",
    [FESTIVAL_SCHEDULE_THAT_TENT]   = "TT",
    [FESTIVAL_SCHEDULE_THE_OTHER]   = "OT",
    [FESTIVAL_SCHEDULE_THIS_TENT]   = "TS",
    [FESTIVAL_SCHEDULE_WHAT_STAGE]  = "WT",
    [FESTIVAL_SCHEDULE_WHICH_STAGE] = "WC",
    [FESTIVAL_SCHEDULE_STAGE_COUNT] = "  "
};

const char festival_genre[FESTIVAL_SCHEDULE_GENRE_COUNT + 1][6] =
{
    [FESTIVAL_SCHEDULE_NO_GENRE]    = " NONE ",
    [FESTIVAL_SCHEDULE_ALT]         = "   ALT",
    [FESTIVAL_SCHEDULE_COUNTRY]     = "Cuntry",
    [FESTIVAL_SCHEDULE_DANCE]       = " DaNCE",
    [FESTIVAL_SCHEDULE_DnB]         = " dnB  ",
    [FESTIVAL_SCHEDULE_DUBSTEP]     = "DUBStP",
    [FESTIVAL_SCHEDULE_EDM]         = "Edm&  ",
    [FESTIVAL_SCHEDULE_FOLK]        = " FOLK ",
    [FESTIVAL_SCHEDULE_HOUSE]       = " HOUSE",
    [FESTIVAL_SCHEDULE_OTHER]       = "OTHEr ",
    [FESTIVAL_SCHEDULE_POP]         = " POP  ",
    [FESTIVAL_SCHEDULE_PUNK]        = "PUNK  ",
    [FESTIVAL_SCHEDULE_RAP]         = "  rAP ",
    [FESTIVAL_SCHEDULE_ROCK]        = " ROCK ",
    [FESTIVAL_SCHEDULE_SOUL]        = " SOUL ",
    [FESTIVAL_SCHEDULE_WORLD]       = " World",
    [FESTIVAL_SCHEDULE_GENRE_COUNT] = "      "
};

#define FREQ_FAST 8
#define FREQ 4

#define TIMEOUT_SEC 10
#define TIMEOUT_HELD_SEC 2

#define MAX_LENGTH 6
#define LOOP_AMOUNT 2

static int16_t _text_pos;
static const char* _text_looping;
static uint8_t _text_looping_len;
static bool _is_text_looping;
static uint8_t loops_occurred;
static watch_date_time_t _starting_time;
static watch_date_time_t _ending_time;
static bool _quick_ticks_running;
static uint8_t _ts_ticks;
static festival_schedule_tick_reason _ts_ticks_purpose;
static const uint8_t _act_arr_size = sizeof(festival_acts) / sizeof(festival_schedule_t);
static bool in_le;


static uint8_t _get_next_act_num(uint8_t act_num, bool get_prev){
    int increment = get_prev ? -1 : 1;
    uint8_t next_act = act_num;
    do{
    next_act = (next_act + increment + _act_arr_size) % _act_arr_size;
    }
    while (festival_acts[next_act].start_time.reg == 0);
    return next_act;
}


// Returns 0 if they're the same; Positive if dt1 is newer than dt2; Negative o/w
static int _compare_dates_times(watch_date_time_t dt1, watch_date_time_t dt2) {
    return (dt1.reg >> 6) - (dt2.reg >> 6);
}

// Returns -1 if already passed, o/w days until start.
static int16_t _get_days_until(watch_date_time_t start_time, watch_date_time_t curr_time){
    start_time.unit.hour = start_time.unit.minute = start_time.unit.second = 0;
    curr_time.unit.hour = curr_time.unit.minute = curr_time.unit.second = 0;
    uint32_t now_timestamp = watch_utility_date_time_to_unix_time(curr_time, 0);
    uint32_t start_timestamp = watch_utility_date_time_to_unix_time(start_time, 0);
    int16_t days_until;
    if (now_timestamp > start_timestamp) // Date already passed
        days_until = -1;
    else
        days_until = (start_timestamp - now_timestamp) / (60 * 60 * 24);
    return days_until;
}

static bool _act_is_playing(uint8_t act_num, watch_date_time_t curr_time){
    if (act_num == FESTIVAL_SCHEDULE_NUM_ACTS) return false;
    return _compare_dates_times(festival_acts[act_num].start_time, curr_time) <= 0 && _compare_dates_times(curr_time, festival_acts[act_num].end_time) < 0;
}

static uint8_t _act_performing_on_stage(uint8_t stage, watch_date_time_t curr_time)
{
    for (int i = 0; i < FESTIVAL_SCHEDULE_NUM_ACTS; i++) {
        if (festival_acts[i].stage == stage && _act_is_playing(i, curr_time))
            return i;
    }
    return FESTIVAL_SCHEDULE_NUM_ACTS;
}

static uint8_t _find_first_available_act(uint8_t first_stage_to_check, watch_date_time_t curr_time, bool reverse)
{
    int increment = reverse ? -1 : 1;
    uint8_t last_stage = (first_stage_to_check - increment + FESTIVAL_SCHEDULE_STAGE_COUNT) % FESTIVAL_SCHEDULE_STAGE_COUNT;
    for (int i = first_stage_to_check;; i = (i + increment + FESTIVAL_SCHEDULE_STAGE_COUNT) % FESTIVAL_SCHEDULE_STAGE_COUNT) {
        uint8_t act_num = _act_performing_on_stage(i, curr_time);
        if (act_num != FESTIVAL_SCHEDULE_NUM_ACTS)
            return act_num;
        if (i == last_stage) break;
    }
    return FESTIVAL_SCHEDULE_NUM_ACTS;
}

static void _display_act(festival_schedule_state_t *state){
    char buf[MAX_LENGTH + 1];
    uint8_t max_pop_display = watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM ? 99 : 39;
    uint8_t popularity = festival_acts[state->curr_act].popularity;
    state->curr_screen = FESTIVAL_SCHEDULE_SCREEN_ACT;
    _text_looping = festival_acts[state->curr_act].artist;
    _text_looping_len = strlen(_text_looping);
    _is_text_looping = MAX_LENGTH < _text_looping_len;
    _text_pos = FREQ * -1;
    watch_clear_display();
    watch_display_text(WATCH_POSITION_TOP_LEFT, festival_stage[state->curr_stage]);
    sprintf(buf, "%.6s", festival_acts[state->curr_act].artist);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    if (popularity <= max_pop_display && popularity > 0) {
        sprintf(buf, "%2d", popularity);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    } else {
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    }
    loops_occurred = 0;
}

static void _display_act_genre(uint8_t act_num, bool show_weekday){
    char buf[MAX_LENGTH + 1];
    watch_clear_display();
    watch_display_text(WATCH_POSITION_TOP_RIGHT, " G");
    sprintf(buf, "%.6s", festival_genre[festival_acts[act_num].genre]);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    if (show_weekday){
        watch_date_time_t start_time = festival_acts[act_num].start_time;
        if (start_time.unit.hour < 5)
            start_time.reg = start_time.reg - (1<<17); // Subtract a day if the act starts before 5am.
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, watch_utility_get_long_weekday(start_time), watch_utility_get_weekday(start_time));
    }
}

static void _display_act_time(uint8_t act_num, bool clock_mode_24h, bool display_end){
    char buf[11];
    watch_date_time_t disp_time = display_end ? festival_acts[act_num].end_time : festival_acts[act_num].start_time;
    watch_clear_display();
    watch_set_colon();
    if (clock_mode_24h){
        watch_set_indicator(WATCH_INDICATOR_24H);

    }
    else{
        watch_clear_indicator(WATCH_INDICATOR_24H);
        // if we are in 12 hour mode, do some cleanup.
        if (disp_time.unit.hour < 12) {
            watch_clear_indicator(WATCH_INDICATOR_PM);
        } else {
            watch_set_indicator(WATCH_INDICATOR_PM);
        }
        disp_time.unit.hour %= 12;
        if (disp_time.unit.hour == 0) disp_time.unit.hour = 12;
    }
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, watch_utility_get_long_weekday(disp_time), watch_utility_get_weekday(disp_time));
    sprintf(buf, "%2d", disp_time.unit.day);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    sprintf(buf, "%2d%.2d%s", disp_time.unit.hour, disp_time.unit.minute, display_end ? "Ed" : "St");
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static watch_date_time_t _get_starting_time(void){
    watch_date_time_t date_oldest = festival_acts[0].start_time;
    for (int i = 1; i < FESTIVAL_SCHEDULE_NUM_ACTS; i++) {
        if (festival_acts[i].artist[0] == 0) continue;
        watch_date_time_t date_check = festival_acts[i].start_time;
        if (_compare_dates_times(date_check, date_oldest) < 0)
            date_oldest= date_check;
    }
    return date_oldest;
}

static watch_date_time_t _get_ending_time(void){
    watch_date_time_t date_newest = festival_acts[0].end_time;
    for (int i = 1; i < FESTIVAL_SCHEDULE_NUM_ACTS; i++) {
        watch_date_time_t date_check = festival_acts[i].end_time;
        if (_compare_dates_times(date_check, date_newest) > 0)
            date_newest= date_check;
    }
    return date_newest;
}

static void _display_festival_name_and_year() {
    char buf[3];
    sprintf(buf, "%.2s", festival_name);
    watch_display_text(WATCH_POSITION_TOP_LEFT, buf);
    sprintf(buf, "%02d", _starting_time.unit.year + 20);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
}

static bool _festival_occurring(watch_date_time_t curr_time, bool update_display){
    char buf[11];
    if (_compare_dates_times(_starting_time, curr_time) > 0){
        _display_festival_name_and_year();
        int16_t days_until = _get_days_until(_starting_time, curr_time);
        if (update_display){
            if (days_until == 0) return true;
            if (days_until <= 999){
                if (days_until > 99) sprintf(buf, "%3dday", days_until);
                else sprintf(buf, "%2d day", days_until);
                watch_display_text(WATCH_POSITION_BOTTOM, buf);
            }
            else {
                watch_display_text(WATCH_POSITION_BOTTOM, "WAIT  ");
            }
        }
        return days_until == 0;
    }
    else if (_compare_dates_times(_ending_time, curr_time) <= 0){
        if (update_display){
            _display_festival_name_and_year();
            watch_display_text(WATCH_POSITION_BOTTOM, "OVER  ");
        }
        return false;
    }
    return true;
}

static void _display_curr_day(watch_date_time_t curr_time){  // Assumes festival_occurring function was run before it.
    char buf[MAX_LENGTH + 1];
    int16_t days_until = _get_days_until(curr_time, _starting_time) + 1;
    _display_festival_name_and_year();
    if (days_until < 100 && days_until >= 0) {
        sprintf(buf, " day%2d", days_until);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    }
    else {
        watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, " Long ", " LONg ");
    }
    return;
}

static void set_ticks_purpose(festival_schedule_tick_reason purpose) {
    _ts_ticks_purpose = purpose;
    switch (purpose) {
        case FESTIVAL_SCHEDULE_TICK_NONE:
            _ts_ticks = 0;
            break;
        case FESTIVAL_SCHEDULE_TICK_SCREEN:
            _ts_ticks = TIMEOUT_SEC * FREQ;
            break;
        case FESTIVAL_SCHEDULE_TICK_LEAVE:
        case FESTIVAL_SCHEDULE_TICK_CYCLE:
            _ts_ticks = TIMEOUT_HELD_SEC * FREQ;
            break;
    }
}

static void _display_title(festival_schedule_state_t *state){
    state->curr_screen = FESTIVAL_SCHEDULE_SCREEN_TITLE;
    state->curr_act = FESTIVAL_SCHEDULE_NUM_ACTS;
    watch_clear_colon();
    watch_clear_all_indicators();
    state->cyc_through_all_acts = false;
    watch_date_time_t curr_time = movement_get_local_date_time();
    state -> prev_day = (curr_time.reg >> 17);
    state -> festival_occurring = _festival_occurring(curr_time, true);
    if (state -> festival_occurring) _display_curr_day(curr_time);
}

static void _display_screen(festival_schedule_state_t *state, bool clock_mode_24h){
    set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_SCREEN);
    if (state->curr_screen != FESTIVAL_SCHEDULE_SCREEN_START_TIME && state->curr_screen != FESTIVAL_SCHEDULE_SCREEN_END_TIME)
    {
        watch_clear_colon();
        watch_clear_indicator(WATCH_INDICATOR_24H);
        watch_clear_indicator(WATCH_INDICATOR_PM);
    }
    switch (state->curr_screen)
    {
    case FESTIVAL_SCHEDULE_SCREEN_ACT:
    case FESTIVAL_SCHEDULE_SCREENS_COUNT:
        _display_act(state);
        break;
    case FESTIVAL_SCHEDULE_SCREEN_GENRE:
        _display_act_genre(state->curr_act, state->cyc_through_all_acts);
        break;
    case FESTIVAL_SCHEDULE_SCREEN_START_TIME:
        _display_act_time(state->curr_act, clock_mode_24h, false);
        break;
    case FESTIVAL_SCHEDULE_SCREEN_END_TIME:
        _display_act_time(state->curr_act, clock_mode_24h, true);
        break;
    case FESTIVAL_SCHEDULE_SCREEN_TITLE:
        _display_title(state);
        break;
    }
}

void festival_schedule_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(festival_schedule_state_t));
        memset(*context_ptr, 0, sizeof(festival_schedule_state_t));
        festival_schedule_state_t *state = (festival_schedule_state_t *)*context_ptr;
        state->curr_act = FESTIVAL_SCHEDULE_NUM_ACTS;
        state->prev_act = FESTIVAL_SCHEDULE_NUM_ACTS + 1;
        state -> prev_day = 0;
        state->cyc_through_all_acts = false;
    }
}

static void _cyc_all_acts(festival_schedule_state_t *state, bool clock_mode_24h, bool handling_light){
    state->cyc_through_all_acts = true;
    watch_set_indicator(WATCH_INDICATOR_LAP);
    state->curr_act = _get_next_act_num(state->curr_act, handling_light);
    state->curr_stage = festival_acts[state->curr_act].stage;
    state->curr_screen = FESTIVAL_SCHEDULE_SCREEN_ACT;
    _display_screen(state, clock_mode_24h);
    return; 
}

static void _handle_btn_up(festival_schedule_state_t *state, bool clock_mode_24h, bool handling_light){
    set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_NONE);
    if (state->cyc_through_all_acts){
        _cyc_all_acts(state, clock_mode_24h, handling_light);
        return;
    }
    if (!state->festival_occurring) return;
    watch_date_time_t curr_time = movement_get_local_date_time();
    if (state->curr_screen != FESTIVAL_SCHEDULE_SCREEN_TITLE) {
        state->curr_stage = handling_light ? 
                        (state->curr_stage - 1 + FESTIVAL_SCHEDULE_STAGE_COUNT) % FESTIVAL_SCHEDULE_STAGE_COUNT :
                        (state->curr_stage + 1) % FESTIVAL_SCHEDULE_STAGE_COUNT;
    }
    if (SHOW_EMPTY_STAGES) {
        state->curr_act = _act_performing_on_stage(state->curr_stage, curr_time);
    }
    else{
        state->curr_act = _find_first_available_act(state->curr_stage, curr_time, handling_light);
        state->curr_stage = festival_acts[state->curr_act].stage;
    }
    state->curr_screen = FESTIVAL_SCHEDULE_SCREEN_ACT;
    _display_screen(state, clock_mode_24h);
}

static void start_quick_cyc(void){
    _quick_ticks_running = true;
    movement_request_tick_frequency(FREQ_FAST);
}

static int16_t _loop_text(const char *text, uint8_t text_len, int8_t curr_loc) {
    char buf[MAX_LENGTH + 1];
    const uint8_t num_spaces = 2;
    if (curr_loc == -1) curr_loc = 0; // avoid double-showing at 0
    // No scrolling needed or in delay phase
    if (MAX_LENGTH >= text_len || curr_loc < 0) {
        // Assume the text is already at the 0th position; no need to rewrite
        return (curr_loc < 0) ? curr_loc + 1 : 0;
    }
    if (curr_loc >= text_len + num_spaces)
        curr_loc = 0;

    uint8_t spaces = num_spaces;
    if (curr_loc > text_len)
        spaces = (curr_loc - text_len < num_spaces) ? curr_loc - text_len : num_spaces;
    
    uint8_t idx = 0;
    const char *src = text + curr_loc;
    while (idx < MAX_LENGTH && curr_loc + idx < text_len) {
        buf[idx++] = *src++;
    }
    while (idx < MAX_LENGTH && spaces > 0) {
        buf[idx++] = ' ';
        spaces--;
    }
    src = text; // wrap to start
    while (idx < MAX_LENGTH) {
        buf[idx++] = *src++;
    }
    buf[MAX_LENGTH] = '\0';
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    return curr_loc + 1;
}

static void handle_ts_ticks(festival_schedule_state_t *state, bool clock_mode_24h){
    static bool _light_held;
    if (_light_held){
        if (!HAL_GPIO_BTN_LIGHT_read()) _light_held = false;
        else return;
    }
    if (_ts_ticks != 0){
        --_ts_ticks;
        switch (_ts_ticks_purpose){
            case FESTIVAL_SCHEDULE_TICK_NONE:
                _ts_ticks = 0;
                break;
            case FESTIVAL_SCHEDULE_TICK_SCREEN:
                if (state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE || state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_ACT){
                    _ts_ticks = 0;
                }
                else if (_ts_ticks == 0){
                    if(HAL_GPIO_BTN_LIGHT_read()){
                        _ts_ticks = 1 * FREQ; // Give one extra second of delay when the light is on
                        _light_held = true;
                    }
                    else{
                        set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_NONE);
                        state->curr_screen = FESTIVAL_SCHEDULE_SCREEN_ACT;
                        _display_screen(state, clock_mode_24h);
                    }
                }
                break;
            case FESTIVAL_SCHEDULE_TICK_LEAVE:
                if (!HAL_GPIO_BTN_MODE_read())_ts_ticks = 0;
                else if (_ts_ticks == 0){
                    if(state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE) movement_move_to_face(0);
                    else{
                        set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_LEAVE);  // This is unneeded, but explicit that we remain in TICK_LEAVE
                        _display_title(state);
                    }
                }
                break;
            case FESTIVAL_SCHEDULE_TICK_CYCLE:
                if (_ts_ticks == 0){
                    set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_NONE);
                    start_quick_cyc();
                }
                break;
        }
    }
}

static bool handle_tick(festival_schedule_state_t *state){
    // Returns true if something on the screen changed.
    watch_date_time_t curr_time;
    if (_quick_ticks_running) {
        if (HAL_GPIO_BTN_LIGHT_read()) _handle_btn_up(state, movement_clock_mode_24h(), true);
        else if (HAL_GPIO_BTN_ALARM_read()) _handle_btn_up(state, movement_clock_mode_24h(), false);
        else{
            _quick_ticks_running = false;
            movement_request_tick_frequency(FREQ);
        }
    }
    handle_ts_ticks(state, movement_clock_mode_24h());

    if (state->cyc_through_all_acts) return false;
    curr_time = movement_get_local_date_time();
    if (curr_time.unit.second != 0) return false;
    bool newDay = ((curr_time.reg >> 17) != (state -> prev_day));
    state -> prev_day = (curr_time.reg >> 17);
    state -> festival_occurring = _festival_occurring(curr_time, (newDay && !state->cyc_through_all_acts));
    if (!state->festival_occurring) return false;
    if(state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE) {
        if (newDay) _display_curr_day(curr_time);
        return false;
    }
    if (!_act_is_playing(state->curr_act, curr_time)){
        if (SHOW_EMPTY_STAGES) {
            state->curr_act = FESTIVAL_SCHEDULE_NUM_ACTS;
        }   
        else{
            state->curr_act = _find_first_available_act(state->curr_stage, curr_time, false);
            state->curr_stage = festival_acts[state->curr_act].stage;
        } 
    }
    if ((state->curr_stage == state->prev_stage) && (state->curr_act == state->prev_act)) return false;
    state->prev_stage = state->curr_stage;
    state->prev_act = state->curr_act;
    state->curr_screen = FESTIVAL_SCHEDULE_SCREEN_ACT;
    _display_screen(state, movement_clock_mode_24h());
    return true;
}

void festival_schedule_face_activate(void *context) {
    (void) context;
    _starting_time = _get_starting_time();
    _ending_time = _get_ending_time();
    _quick_ticks_running = false;
    set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_NONE);
    movement_request_tick_frequency(FREQ);
}

bool festival_schedule_face_loop(movement_event_t event, void *context) {
    festival_schedule_state_t *state = (festival_schedule_state_t *)context;
    bool changed_from_handle_ticks;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            if (!in_le && state->curr_act == FESTIVAL_SCHEDULE_NUM_ACTS) {
                _display_title(state);
            }
            in_le = false;
            break;
        case EVENT_TICK:
            changed_from_handle_ticks = handle_tick(state);
            if (changed_from_handle_ticks) _is_text_looping = MAX_LENGTH < _text_looping_len;
            if (!changed_from_handle_ticks && state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_ACT
                && !_quick_ticks_running && _is_text_looping) {
                    int16_t prev_text_pos = _text_pos;
                    _text_pos = _loop_text(_text_looping, _text_looping_len, _text_pos);
                    if (prev_text_pos > _text_pos) loops_occurred++;
                    if (loops_occurred >= LOOP_AMOUNT) {
                        loops_occurred = 0;
                        _is_text_looping = false;
                    }
                }
                break;
        case EVENT_LOW_ENERGY_UPDATE:
            changed_from_handle_ticks = handle_tick(state);
            if (!changed_from_handle_ticks && !in_le
                && state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_ACT) {
                in_le = true;
                if (state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_ACT) {
                    // Resets the act name in LE mode so the beginning of it is shown
                    _display_screen(state, movement_clock_mode_24h());
                }
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            _handle_btn_up(state, movement_clock_mode_24h(), true);
            break;
        case EVENT_ALARM_BUTTON_UP:
            _handle_btn_up(state, movement_clock_mode_24h(), false);
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE){
                _cyc_all_acts(state, movement_clock_mode_24h(), false);
                set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_CYCLE);
            }
            else if (state->festival_occurring && !state->cyc_through_all_acts) break;
            else start_quick_cyc();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;  // To overwrite the default LED on
        case EVENT_LIGHT_LONG_PRESS:
            if (state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE){
                _cyc_all_acts(state, movement_clock_mode_24h(), true);
                set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_CYCLE);
            }
            else if (state->curr_screen != FESTIVAL_SCHEDULE_SCREEN_ACT || (state->festival_occurring && !state->cyc_through_all_acts))
                movement_illuminate_led(); // Will allow led for see acts' genre and times
            else start_quick_cyc();
            break;
        case EVENT_MODE_LONG_PRESS:

            if (state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE){
                movement_move_to_face(0);
            }
            if (state->curr_screen != FESTIVAL_SCHEDULE_SCREEN_ACT){
                state->curr_screen = FESTIVAL_SCHEDULE_SCREEN_ACT;
                _display_screen(state, movement_clock_mode_24h());
                set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_LEAVE);
            }
            else {
                set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_LEAVE);
                _display_title(state);
            }
            break;
        case EVENT_MODE_BUTTON_UP:
            if (state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE) movement_move_to_next_face();
            else if (state->curr_act == FESTIVAL_SCHEDULE_NUM_ACTS) _display_title(state);
            else if (!_is_text_looping && MAX_LENGTH < _text_looping_len) _is_text_looping = true;
            else {
                do
                {
                    state->curr_screen = (state->curr_screen + 1) % FESTIVAL_SCHEDULE_SCREENS_COUNT;
                } while (state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE);
                _display_screen(state, movement_clock_mode_24h());
            }
            break;
        case EVENT_TIMEOUT:
            if (state->cyc_through_all_acts){
                state->cyc_through_all_acts = false;
                _display_title(state);
            }
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void festival_schedule_face_resign(void *context) {
    festival_schedule_state_t *state = (festival_schedule_state_t *)context;
    state->curr_act = FESTIVAL_SCHEDULE_NUM_ACTS;
    state->cyc_through_all_acts = false;
    state->prev_act = FESTIVAL_SCHEDULE_NUM_ACTS + 1;
}

