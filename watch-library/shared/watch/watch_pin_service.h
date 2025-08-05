/*
 * MIT License
 *
 * Copyright (c) 2025 Alessandro Genova
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

#ifndef _WATCH_PIN_SERVICE_H_INCLUDED
#define _WATCH_PIN_SERVICE_H_INCLUDED
////< @file watch_pin_service.h

#include <stdint.h>
#include <stdbool.h>

#include "movement.h"

typedef union {
    struct {
        uint8_t digit0 : 4;     // First digit
        uint8_t digit1 : 4;     // Second digit
        uint8_t digit2 : 4;     // Third digit
        uint8_t digit3 : 4;     // Fourth digit
        uint8_t digit4 : 4;     // Fifth digit
        uint8_t digit5 : 4;     // Sixth digit
    } unit;
    uint32_t reg;               // the bit-packed value representing the pin.
} watch_pin_t;

extern const uint8_t PIN_EMPTY_DIGIT;
extern const uint8_t PIN_EMPTY_FACE;

typedef struct {
    bool enabled;                       // is the pin service enable?
    bool locked;                        // is the pin service currently locked or unlocked
    watch_pin_t pin;                    // the current pin
    uint8_t pin_face_index;             // The watch face we need to redirect to in order to enter the pin
    uint8_t requesting_face_index;      // The watch face that initiate the request for a pin, which we'll redirect to after successful unlock.
} watch_pin_service_state_t;


void watch_pin_service_enable(void);
bool watch_pin_service_is_locked(void);
void watch_pin_service_lock(void);
bool watch_pin_service_unlock(watch_pin_t pin);
bool watch_pin_service_verify(watch_pin_t pin);
bool watch_pin_service_set_pin(watch_pin_t old_pin, watch_pin_t new_pin);
uint8_t watch_pin_service_get_pin_face(void);
void watch_pin_service_set_pin_face(uint8_t face_index);
uint8_t watch_pin_service_get_requesting_face(void);
void watch_pin_service_set_requesting_face(uint8_t face_index);
bool watch_pin_service_loop(movement_event_t event, uint8_t face_index, char* face_title, char* face_title_fallback);

#endif
