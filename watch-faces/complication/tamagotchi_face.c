/*
 * MIT License
 *
 * Copyright (c) 2025 Mitch
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
#include "tamagotchi_face.h"#

void tamagotchi_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(tamagotchi_state_t));
        memset(*context_ptr, 0, sizeof(tamagotchi_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.

        tamagotchi_state_t *state = (tamagotchi_state_t *) *context_ptr;

        tamagotchi_face_init_segment(state, 0, 0, 0,   2, -1, -1, -1);
        tamagotchi_face_init_segment(state, 1, 0, 0,   4, -1, -1, -1);
        tamagotchi_face_init_segment(state, 2, 1, 0,   7, 8, -1, -1);
        tamagotchi_face_init_segment(state, 3, 1, 0,   1, -1, -1, -1);
        tamagotchi_face_init_segment(state, 4, 0, 1,   6, 10, -1, -1);
        tamagotchi_face_init_segment(state, 5, 0, 1,   0, -1, -1, -1);
        tamagotchi_face_init_segment(state, 6, 1, 1,   3, 8, 18, 18); //18 18
        tamagotchi_face_init_segment(state, 7, 1, 1,   10, 5, -1, -1);
        tamagotchi_face_init_segment(state, 8, 2, 0,   13, -1, -1, -1);
        tamagotchi_face_init_segment(state, 9, 2, 0,   7, 3, -1, -1);
        tamagotchi_face_init_segment(state, 10, 2, 10, 12, -1, -1, -1);
        tamagotchi_face_init_segment(state, 11, 2, 10, 5, 6, -1, -1);
        tamagotchi_face_init_segment(state, 12, 2, 1,  9, -1, -1, -1);
        tamagotchi_face_init_segment(state, 13, 2, 1,  11, -1, -1, -1);

        tamagotchi_face_init_segment(state, 14, 1, 22,  -1, -1, -1, -1);
        tamagotchi_face_init_segment(state, 15, 1, 22,  24, -1, -1, -1);
        tamagotchi_face_init_segment(state, 16, 0, 23,  -1, -1, -1, -1);
        tamagotchi_face_init_segment(state, 17, 0, 23,  1, -1, -1, -1);
        tamagotchi_face_init_segment(state, 18, 1, 23,  30, -1, -1, -1);
        tamagotchi_face_init_segment(state, 19, 1, 23,   7, -1, -1, -1);
        tamagotchi_face_init_segment(state, 20, 2, 22,  36, -1, -1, -1);
        tamagotchi_face_init_segment(state, 21, 2, 22,  -1, -1, -1, -1);
        tamagotchi_face_init_segment(state, 22, 2, 23,  13, -1, -1, -1);
        tamagotchi_face_init_segment(state, 23, 2, 23,  -1, -1, -1, -1);

        tamagotchi_face_init_segment(state, 24, 0, 21,   2+24, -1, -1, -1);
        tamagotchi_face_init_segment(state, 25, 0, 21,   4+24, -1, -1, -1);
        tamagotchi_face_init_segment(state, 26, 0, 20,   7+24, 8+24, -1, -1);
        tamagotchi_face_init_segment(state, 27, 0, 20,   1+24, -1, -1, -1);
        tamagotchi_face_init_segment(state, 28, 1, 21,   6+24, 10+24, -1, -1);
        tamagotchi_face_init_segment(state, 29, 1, 21,   0+24, -1, -1, -1);
        tamagotchi_face_init_segment(state, 30, 1, 20,   3+24, 8+24, -1, -1);
        tamagotchi_face_init_segment(state, 31, 1, 20,   10+24, 5+24, 19, 19); //19 19
        tamagotchi_face_init_segment(state, 32, 1, 17,   13+24, -1, -1, -1);
        tamagotchi_face_init_segment(state, 33, 1, 17,   7+24, 3+24, -1, -1);
        tamagotchi_face_init_segment(state, 34, 2, 21,   12+24, -1, -1, -1);
        tamagotchi_face_init_segment(state, 35, 2, 21,   5+24, 6+24, -1, -1);
        tamagotchi_face_init_segment(state, 36, 2, 20,   9+24, -1, -1, -1);
        tamagotchi_face_init_segment(state, 37, 2, 20,   11+24, -1, -1, -1);

        tamagotchi_face_init_smiley_segment(state, 0, 0, 3);
        tamagotchi_face_init_smiley_segment(state, 1, 0, 2);
        tamagotchi_face_init_smiley_segment(state, 2, 0, 4);
        tamagotchi_face_init_smiley_segment(state, 3, 1, 3);
        tamagotchi_face_init_smiley_segment(state, 4, 1, 2);
        tamagotchi_face_init_smiley_segment(state, 5, 2, 3);
        tamagotchi_face_init_smiley_segment(state, 6, 2, 2);
        tamagotchi_face_init_smiley_segment(state, 7, 0, 6);
        tamagotchi_face_init_smiley_segment(state, 8, 0, 5);
        tamagotchi_face_init_smiley_segment(state, 9, 1, 6);
        tamagotchi_face_init_smiley_segment(state, 10, 1, 5);
        tamagotchi_face_init_smiley_segment(state, 11, 1, 4);
        tamagotchi_face_init_smiley_segment(state, 12, 2, 5);
        tamagotchi_face_init_smiley_segment(state, 13, 2, 4);

        state->length = 1;
        state->caterpillar = true;
        state->indexes[0] = 25;
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

static uint32_t tamagotchi_face_get_random(uint32_t max) {
    #if __EMSCRIPTEN__
        return rand() % max;
    #else
        return arc4random_uniform(max);
    #endif
}

static void tamagotchi_face_init_segment(tamagotchi_state_t * state, uint8_t segment_index, uint8_t com, uint8_t seg, uint8_t adj1, uint8_t adj2, uint8_t adj3, uint8_t adj4) {
    state->segments[segment_index].com = com;
    state->segments[segment_index].seg = seg;
    state->segments[segment_index].adjacent[0] = adj1;
    state->segments[segment_index].adjacent[1] = adj2;
    state->segments[segment_index].adjacent[2] = adj3;
    state->segments[segment_index].adjacent[3] = adj4;
}

static void tamagotchi_face_init_smiley_segment(tamagotchi_state_t * state, uint8_t segment_index, uint8_t com, uint8_t seg) {
    state->smiley_segments[segment_index].com = com;
    state->smiley_segments[segment_index].seg = seg;
}

static void tamagotchi_face_draw_door(tamagotchi_state_t * state) {
    if (!state->door_is_open) {
        watch_clear_pixel(0, 22);
        watch_set_pixel(1, 22);
        watch_set_pixel(2, 22);

        //watch_set_pixel(0, 22);
    } else {
        watch_set_pixel(0, 22);
        watch_clear_pixel(1, 22);
        watch_clear_pixel(2, 22);
    }
}

static void tamagotchi_face_set_smiley_pixel(tamagotchi_state_t * state, uint8_t index) {
    watch_set_pixel(state->smiley_segments[index].com, state->smiley_segments[index].seg);
}

static void tamagotchi_face_clear_smiley_pixel(tamagotchi_state_t * state, uint8_t index) {
    watch_clear_pixel(state->smiley_segments[index].com, state->smiley_segments[index].seg);
}

static void tamagotchi_face_draw_smiley(tamagotchi_state_t * state, bool clear, tamagotchi_smiley_eyes eyes, tamagotchi_smiley_mouth mouth) {
    if (clear) {
        for (int i=0; i < 14; i++) {
            watch_clear_pixel(state->smiley_segments[i].com, state->smiley_segments[i].seg);
        }    
    }

    switch (eyes) {
        case TAMAGOTCHI_EYES_BLINK:
            tamagotchi_face_set_smiley_pixel(state, 6);
            tamagotchi_face_set_smiley_pixel(state, 13);
            break;
        case TAMAGOTCHI_EYES_NORMAL:
            tamagotchi_face_set_smiley_pixel(state, 5);
            tamagotchi_face_set_smiley_pixel(state, 11);
            break;
    }
    
    switch (mouth) {
        case TAMAGOTCHI_MOUTCH_SMILE:
            tamagotchi_face_set_smiley_pixel(state, 1);
            tamagotchi_face_set_smiley_pixel(state, 0);
            tamagotchi_face_set_smiley_pixel(state, 7);
            tamagotchi_face_set_smiley_pixel(state, 9);
            break;
    }
}

void tamagotchi_face_activate(void *context) {
    tamagotchi_state_t *state = (tamagotchi_state_t *)context;

    // Handle any tasks related to your watch face coming on screen.
}

    uint8_t freq = 16; 
    uint8_t speed = 4;

bool tamagotchi_face_loop(movement_event_t event, void *context) {
    tamagotchi_state_t *state = (tamagotchi_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            movement_request_tick_frequency(freq);
            state->door_is_open = !state->door_is_open; //TODO: for fallthrough
            //fallthrough
        case EVENT_ALARM_BUTTON_UP:       
            state->door_is_open = !state->door_is_open;
            if (!state->door_is_open) {
                state->segments[18].adjacent[0] = -1;
                state->segments[31].adjacent[2] = -1;
                state->segments[31].adjacent[3] = -1;
            } else {
                state->segments[18].adjacent[0] = 30;
                state->segments[31].adjacent[2] = 19; //19 19
                state->segments[31].adjacent[3] = 19; //19 19
            }
            tamagotchi_face_draw_door(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            //speed = speed == 1 ? 1 : (speed / 2);
            speed = speed == freq ? 1 : speed * 2;
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if(!state->food_set) {
                tamagotchi_face_set_smiley_pixel(state, 0);
                tamagotchi_face_set_smiley_pixel(state, 1);
                tamagotchi_face_set_smiley_pixel(state, 2);
                tamagotchi_face_set_smiley_pixel(state, 3);
                state->food_set = true;
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (state->length > 2)
                state->length--;
            break;
        case EVENT_TICK: 
            state->ticks = (state->ticks + 1) % freq;  

            if(state->ticks % (freq / speed) == 0) {         
                //eat food
                if (state->food_set && (state->indexes[0] == 1 || state->indexes[0] == 5)) {
                    tamagotchi_face_clear_smiley_pixel(state, 0);
                    tamagotchi_face_clear_smiley_pixel(state, 1);
                    tamagotchi_face_clear_smiley_pixel(state, 2);
                    tamagotchi_face_clear_smiley_pixel(state, 3);

                    if (state->length < 10)
                        state->length++;
                    state->food_set = false;
                    break;
                }

                //check if snake is stuck
                bool stuck = true;
                for (int i = 0; i < 4; i++) {
                    if (state->segments[state->indexes[0]].adjacent[i] != -1) {
                        stuck = false;
                        break;
                    }
                }

                if (stuck) {
                    //revert snake          
                    for (int i = 0; i < state->length/2; i++) {
                        uint8_t temp = state->indexes[i];
                        state->indexes[i] = state->indexes[state->length - 1 - i];
                        state->indexes[state->length - 1 - i] = temp;
                    }
                    //revert direction of new head
                    state->indexes[0] = state->indexes[0] % 2 == 0 ? state->indexes[0] + 1 : state->indexes[0] - 1;

                } else {   
                    //update body segments
                    for (int i = state->length - 1; i > 0; i--) {
                        state->indexes[i] = state->indexes[i - 1];
                    }               
                    //update head segment (move forward)
                    int8_t head_index = state->indexes[0];
                    do {
                        state->indexes[0] = state->segments[head_index].adjacent[tamagotchi_face_get_random(4)];
                    } while (state->indexes[0] == -1);                  
                }

                watch_clear_display();
                //tamagotchi_face_draw_smiley(state, false, state->door_is_open ? TAMAGOTCHI_EYES_NORMAL : TAMAGOTCHI_EYES_BLINK, TAMAGOTCHI_MOUTCH_SMILE);
                
                //draw food
                if (state->food_set) {
                    tamagotchi_face_set_smiley_pixel(state, 0);
                    tamagotchi_face_set_smiley_pixel(state, 1);
                    tamagotchi_face_set_smiley_pixel(state, 2);
                    tamagotchi_face_set_smiley_pixel(state, 3);
                }
                
                //draw door
                tamagotchi_face_draw_door(state);

                //draw snake
                for (int i = state->length - 1; i >= 0; i--) {
                    watch_set_pixel(state->segments[state->indexes[i]].com, state->segments[state->indexes[i]].seg);
                }   
            } else if(state->caterpillar && state->length > 2 && state->ticks % (freq / (2 * speed)) == 0) {
                //caterpillar effect
                watch_clear_pixel(state->segments[state->indexes[state->length - 1]].com, state->segments[state->indexes[state->length - 1]].seg);
            }

            char buf[8];
            sprintf(buf, "%2d", state->length);
            watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
            
            break;
        // case EVENT_LIGHT_BUTTON_UP:
        //      watch_clear_display();

        //     //  watch_set_pixel(0, 0);
        //     //  watch_set_pixel(1, 0);
        //     //  watch_set_pixel(2, 0);
        //     //  watch_set_pixel(0, 1);
        //     //  watch_set_pixel(1, 1);
        //     //  watch_set_pixel(2, 1);

        //     for (int i=0; i<38; i++) {
        //         watch_set_pixel(state->segments[i].com, state->segments[i].seg);
        //     }

        //     //  if(state->seg == 0) watch_set_pixel(0, 18);
        //     //  if(state->seg == 1) watch_set_pixel(1, 18);
        //     //  if(state->seg == 2) watch_set_pixel(2, 18);
        //     //  if(state->seg == 3) watch_set_pixel(0, 19);
        //     //  if(state->seg == 4) watch_set_pixel(1, 19);
        //     //  if(state->seg == 5) watch_set_pixel(2, 19);

            
        //      if(state->seg == 0) watch_set_pixel(0, 6);
        //      if(state->seg == 1) watch_set_pixel(1, 6);
        //      if(state->seg == 2) watch_set_pixel(2, 6);
        //      if(state->seg == 3) watch_set_pixel(0, 7);
        //      if(state->seg == 4) watch_set_pixel(1, 7);
        //      if(state->seg == 5) watch_set_pixel(2, 7);

        //       char buf4[8];
        //     sprintf(buf4, "%2d", state->seg);
        //     watch_display_text(WATCH_POSITION_TOP_RIGHT, buf4);

        //      state->seg= (state->seg+1)%6;

        //     // state->seg++;
        //     // if(state->seg == 24) {
        //     //     state->seg = 0;
        //     // }
        //     // watch_set_pixel(state->com, state->seg);

        //     //state->seg= 10;

        //     //}
        //     break;
        // case EVENT_LIGHT_LONG_PRESS:
        //     state->com++;
        //     if(state->com == 3)
        //         state->com = 0;
        //     watch_set_pixel(state->com, state->seg);
        //     break;
        case EVENT_TIMEOUT:
            // Your watch face will receive this event after a period of inactivity. If it makes sense to resign,
            // you may uncomment this line to move back to the first watch face in the list:
            // movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            // If you did not resign in EVENT_TIMEOUT, you can use this event to update the display once a minute.
            // Avoid displaying fast-updating values like seconds, since the display won't update again for 60 seconds.
            // You should also consider starting the tick animation, to show the wearer that this is sleep mode:
            // watch_start_sleep_animation(500);
            break;
        default:
            // Movement's default loop handler will step in for any cases you don't handle above:
            // * EVENT_LIGHT_BUTTON_DOWN lights the LED
            // * EVENT_MODE_BUTTON_UP moves to the next watch face in the list
            // * EVENT_MODE_LONG_PRESS returns to the first watch face (or skips to the secondary watch face, if configured)
            // You can override any of these behaviors by adding a case for these events to this switch statement.
            return movement_default_loop_handler(event);
    }

    // return true if the watch can enter standby mode. Generally speaking, you should always return true.
    return true;
}

void tamagotchi_face_resign(void *context) {
    (void) context;

    // handle any cleanup before your watch face goes off-screen.
}

