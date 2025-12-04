/*
 * MIT License
 *
 * Copyright (c) 2025 kbc-yam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include "wareki_face.h"
#include "filesystem.h"
#include "watch_utility.h"
#include "watch.h"

//Long press status flag
static bool _alarm_button_press;
static bool _light_button_press;


void wareki_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    
    //printf("wareki_setup() \n");
    
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(wareki_state_t));
        memset(*context_ptr, 0, sizeof(wareki_state_t));

        //debug code 
        // watch_date_time datetime = watch_rtc_get_date_time();
        // datetime.unit.year  = 2022 - WATCH_RTC_REFERENCE_YEAR;
        // datetime.unit.month = 12;
        // datetime.unit.day   = 31;
        // datetime.unit.hour  = 23;
        // datetime.unit.minute= 59;
        // datetime.unit.second= 30;
        // watch_rtc_set_date_time(datetime);
        // settings->bit.clock_mode_24h = true; //24時間表記
        // settings->bit.to_interval = 1;//0=60sec 1=2m 2=5m 3=30m
        // watch_store_backup_data(settings->reg, 0);
    }
}


// splash view
static void draw_wareki_splash(wareki_state_t *state) {    
    (void) state;
    
    watch_clear_colon();

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "WA ", "wa");
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    watch_display_text(WATCH_POSITION_BOTTOM, "      ");
}


//draw year and Japanese wareki
static void draw_year_and_wareki(wareki_state_t *state) {
    char buf[16];
    
    bool is_custom_lcd = watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM;

    if(state->disp_year == REIWA_GANNEN){
        //The first year of Reiwa (2019) began on May 1. The period before May 1 is Heisei 31.
        //In other words, 2019 is Heisei Year 31 and it is possible that it is Reiwa Year 1.
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "H31", " r");

        if (is_custom_lcd)
        {
            //For custom LCDs, display both Heisei and Reiwa.
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "r1");
        }
        else{
            watch_display_text(WATCH_POSITION_TOP_RIGHT, " 1");
        }
    }
    else if(state->disp_year == HEISEI_GANNEN){
        //The year 1989 could be Showa 64 or it could be Heisei 1.
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "S64", " h");

        if (is_custom_lcd)
        {
            //For custom LCDs, display both Showa and Heisei.
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "h1");
        }
        else{
            watch_display_text(WATCH_POSITION_TOP_RIGHT, " 1");
        }
    }
    else if(state->disp_year < HEISEI_GANNEN){
        //Showa
        //sprintf(buf, " h%2d%4d  ", (int)state->disp_year - HEISEI_GANNEN + 1, (int)state->disp_year);

        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "Sho", "s ");

        sprintf(buf, "%2d", (int)state->disp_year - SHOWA_GANNEN + 1);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }
    else if(state->disp_year < REIWA_GANNEN){
        //Heisei
        //sprintf(buf, " h%2d%4d  ", (int)state->disp_year - HEISEI_GANNEN + 1, (int)state->disp_year);

        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "HEI", "h ");

        sprintf(buf, "%2d", (int)state->disp_year - HEISEI_GANNEN + 1);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }
    else{
        //Reiwa
        //sprintf(buf, " r%2d%4d  ", (int)state->disp_year - REIWA_GANNEN + 1 , (int)state->disp_year);
    
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "REI", "r ");

        sprintf(buf, "%2d", (int)state->disp_year - REIWA_GANNEN + 1);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }

    sprintf(buf, "%4d  ",(int)state->disp_year);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}


void wareki_activate(void *context) {

    //printf("wareki_activate() \n");

    wareki_state_t *state = (wareki_state_t *)context;

    if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();

    state->active = false;

    _alarm_button_press = false;
    _light_button_press = false;

    state->real_year = watch_rtc_get_date_time().unit.year + WATCH_RTC_REFERENCE_YEAR;
    state->start_year = state->real_year;
    state->disp_year = state->real_year;

    movement_request_tick_frequency(1);
}


void addYear(wareki_state_t* state,int count){

    state->disp_year = state->disp_year + count;

    if(state->disp_year > REIWA_LIMIT ){
        state->disp_year = REIWA_LIMIT;
    } 
    else{
    }
}


void subYear(wareki_state_t* state,int count){

    state->disp_year = state->disp_year - count;

    if(state->disp_year < SHOWA_GANNEN ){
        state->disp_year = SHOWA_GANNEN;
    } 
    else{
    }
}


bool wareki_loop(movement_event_t event, void *context) {
    wareki_state_t *state = (wareki_state_t *)context;

    state->real_year = watch_rtc_get_date_time().unit.year + WATCH_RTC_REFERENCE_YEAR;

    if( state->real_year != state->start_year ){
        state->start_year = state->real_year;
        state->disp_year = state->real_year;
    }


    switch (event.event_type) {
        case EVENT_ACTIVATE:
            draw_wareki_splash(state);
            break;
        case EVENT_MODE_BUTTON_UP:
            movement_move_to_next_face();
            break;

        case EVENT_LOW_ENERGY_UPDATE:
        case EVENT_TICK:          

            //printf("tick %d\n",state->disp_year );

            if (_alarm_button_press && HAL_GPIO_BTN_ALARM_pin() ){
                //printf("ALARM ON\n");
            }
            else{
                //printf("ALARM OFF\n");
                _alarm_button_press = false;
            }


            if (_light_button_press && HAL_GPIO_BTN_LIGHT_pin()){
                //printf("LIGHT ON\n");
            }
            else{
                //printf("LIGHT OFF\n");
                _light_button_press = false;
            }

            if (_alarm_button_press) {
                addYear(state,1);
            }
            if (_light_button_press) {
                subYear(state,1);
            }
            
            draw_year_and_wareki(state);

            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            //printf("LIGHT DOWN\n");
            subYear(state,1);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            //printf("LIGHTPRESS \n"); 
            _light_button_press = true;
            movement_request_tick_frequency(8);
            break;
        case EVENT_LIGHT_LONG_UP:
            //printf("LIGHTPRESS UP\n");
            _light_button_press = false;
            movement_request_tick_frequency(4);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            //printf("LIGHT UP\n");
            _light_button_press = false;
            movement_request_tick_frequency(4);
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            //printf("ALARM DOWN\n");
            addYear(state,1);
            break;
        case EVENT_ALARM_LONG_PRESS:
            //printf("LONGPRESS \n");
            _alarm_button_press = true;
            movement_request_tick_frequency(8);
            break;
        case EVENT_ALARM_LONG_UP:
            //printf("LONGPRESS UP\n");
            _alarm_button_press = false;
            movement_request_tick_frequency(4);
            break;
        case EVENT_ALARM_BUTTON_UP:
            //printf("ALARM UP\n");
            movement_request_tick_frequency(4);
            break;

        case EVENT_TIMEOUT:
            //printf("time out ! \n");
            movement_move_to_face(0);

            break;
        //case EVENT_LOW_ENERGY_UPDATE:
            // If you did not resign in EVENT_TIMEOUT, you can use this event to update the display once a minute.
            // Avoid displaying fast-updating values like seconds, since the display won't update again for 60 seconds.
            // You should also consider starting the tick animation, to show the wearer that this is sleep mode:
            // watch_start_tick_animation(500);
            //break;
        default:
            // Movement's default loop handler will step in for any cases you don't handle above:
            // * EVENT_LIGHT_BUTTON_DOWN lights the LED
            // * EVENT_MODE_BUTTON_UP moves to the next watch face in the list
            // * EVENT_MODE_LONG_PRESS returns to the first watch face (or skips to the secondary watch face, if configured)
            // You can override any of these behaviors by adding a case for these events to this switch statement.
            //return movement_default_loop_handler(event, settings);
            return movement_default_loop_handler(event);
    }

    return true;
}

void wareki_resign(void *context) {
    (void) context;
}

