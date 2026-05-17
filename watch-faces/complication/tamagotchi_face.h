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

#pragma once

#include "movement.h"

/*
 * A DESCRIPTION OF YOUR WATCH FACE
 *
 * and a description of how use it
 *
 */

typedef enum {
  TAMAGOTCHI_EYES_NORMAL,
  TAMAGOTCHI_EYES_BLINK,
} tamagotchi_smiley_eyes;

typedef enum {
  TAMAGOTCHI_MOUTCH_SMILE
} tamagotchi_smiley_mouth;

typedef struct tamagotchi_segment_mapping_t {
    uint8_t com;
    uint8_t seg;
    int8_t adjacent[4];
} tamagotchi_segment_mapping_t;

typedef struct tamagotchi_smiley_segment_mapping_t {
    uint8_t com;
    uint8_t seg;
} tamagotchi_smiley_segment_mapping_t;

typedef struct {
    tamagotchi_segment_mapping_t segments[38];
    tamagotchi_smiley_segment_mapping_t smiley_segments[14];

    int8_t indexes[10]; //current placement of snake
    uint8_t length; //current length of snake
    bool caterpillar; //true to enable smooth movement
    bool door_is_open; //true if door is open
    bool food_set; //true if food is placed
    uint8_t shit_set; //numbers of shits currently placed
    uint32_t next_shit_ts; //next time to shit
    uint8_t ticks; 
    int8_t seg;
    int8_t com;
} tamagotchi_state_t;

void tamagotchi_face_setup(uint8_t watch_face_index, void ** context_ptr);
void tamagotchi_face_activate(void *context);
bool tamagotchi_face_loop(movement_event_t event, void *context);
void tamagotchi_face_resign(void *context);
movement_watch_face_advisory_t tamagotchi_face_advise(void *context);

#define tamagotchi_face ((const watch_face_t){ \
    tamagotchi_face_setup, \
    tamagotchi_face_activate, \
    tamagotchi_face_loop, \
    tamagotchi_face_resign, \
    tamagotchi_face_advise, \
})
