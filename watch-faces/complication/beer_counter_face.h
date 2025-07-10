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

#pragma once
#ifndef BEER_COUNTER_FACE_H_
#define BEER_COUNTER_FACE_H_

#include "movement.h"

/*
 * A DESCRIPTION OF YOUR WATCH FACE
 *
 * and a description of how use it
 *
 */

typedef enum {
    bc_screen,
    weight_screen,
    height_screen,
    sex_screen,
    bac_screen,
    sober_screen,
    drink_vol_screen,
    alc_cont_screen
} beer_counter_mode_t;

typedef struct {
    uint8_t beer_count:7;
    uint8_t old_beer_count:7;
    uint8_t weight;
    uint8_t height;
    uint8_t sex:1; // 0 for male, 1 for female
    uint16_t drink_vol:10;
    uint16_t alc_cont:7;
    uint8_t reserved:5; // Padding to align to 4 bytes boundary
    uint32_t last_time; // Store last time of beer consumption
    uint32_t last_time_bac;
    beer_counter_mode_t mode;
    float old_bac;
} beer_counter_state_t;

void beer_counter_face_setup(uint8_t watch_face_index, void ** context_ptr);
void beer_counter_face_activate(void *context);
bool beer_counter_face_loop(movement_event_t event, void *context);
void beer_counter_face_resign(void *context);

#define beer_counter_face ((const watch_face_t){ \
    beer_counter_face_setup, \
    beer_counter_face_activate, \
    beer_counter_face_loop, \
    beer_counter_face_resign, \
    NULL, \
})

#endif // BEER_COUNTER_FACE_H_