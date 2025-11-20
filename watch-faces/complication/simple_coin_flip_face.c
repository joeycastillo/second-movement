/*
 * MIT License
 *
 * Copyright (c) 2023 Wesley Aptekar-Cassels
 * Copyright (c) 2025 Vaipex
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
#include "simple_coin_flip_face.h"
#include "delay.h"

void simple_coin_flip_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(simple_coin_flip_face_state_t));
        memset(*context_ptr, 0, sizeof(simple_coin_flip_face_state_t));
    }
}

void simple_coin_flip_face_activate(void *context) {
    (void) context;
}

static uint32_t get_random(uint32_t max) {
    #if __EMSCRIPTEN__
        return rand() % max;
    #else
        return arc4random_uniform(max);
    #endif

}

static void draw_start_face(void) {
    watch_clear_display();
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC) {
        watch_display_text(WATCH_POSITION_BOTTOM, " Flip");
    } else {
        watch_display_text(WATCH_POSITION_BOTTOM, "Flip");
    }
}

static void set_pixels(int pixels[3][4][2], int j_len) {
    for(int loopruns = 0; loopruns<2; loopruns++) {
        for(int i = 0; i<3; i++) {
            watch_clear_display();
            for(int j = 0; j<j_len; j++){
                watch_set_pixel(pixels[i][j][0], pixels[i][j][1]);
            }
            delay_ms(150);
        }
    }
}

static void load_animation(void) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC) {
        int j_len = 2;
        int pixels[3][4][2] = {
            {
                {0, 3},
                {0, 6}
            },
            {
                {1, 3},
                {1, 5}
            },
            {
                {2, 2},
                {2, 4}
            }
        };
        set_pixels(pixels, j_len);
    } else{
        int j_len = 4;
        int pixels[3][4][2] = {
            {
                {2, 22},
                {2, 15},
                {1, 2},
                {1, 4}
            },
            {
                {0, 16},
                {0, 15},
                {0, 1},
                {0, 3}
            },
            {
                {3, 16},
                {3, 14},
                {3, 1},
                {3, 3}
            }
        };
        set_pixels(pixels, j_len);
    }
}

static void _blink_face_update_lcd(simple_coin_flip_face_state_t *state) {
    (void) state;
    watch_clear_display();
    load_animation();
    watch_clear_display();
    int r = get_random(2);
    if(r){
        watch_display_text(WATCH_POSITION_BOTTOM, "Heads");
    }else{
        if (watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC) {
            watch_display_text(WATCH_POSITION_BOTTOM, " Tails");
        }else {
            watch_display_text(WATCH_POSITION_BOTTOM, "Tails");
        }
    }
}

bool simple_coin_flip_face_loop(movement_event_t event, void *context) {
    simple_coin_flip_face_state_t *state = (simple_coin_flip_face_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            draw_start_face();
            break;
        case EVENT_TICK:
            if(!state->is_start_face && !state->active){
                if(state->inactivity_ticks >= 15){
                    state->is_start_face = false;
                    state->inactivity_ticks = 0;
                    draw_start_face();
                }else{
                    state->inactivity_ticks++;
                }
            } else {
                state->inactivity_ticks = 0;
            }
            break;
        //execute same action for light and alarm button
        case EVENT_LIGHT_BUTTON_UP:
        case EVENT_ALARM_BUTTON_UP:
            if (!state->active) {
                state->active = true;
                _blink_face_update_lcd(state);
                state->active = false;
                state->is_start_face = false;
                state->inactivity_ticks = 0;
            }
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void simple_coin_flip_face_resign(void *context) {
    (void) context;
}

