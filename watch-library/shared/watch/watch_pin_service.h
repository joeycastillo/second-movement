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

/** @addtogroup PIN Service
  * @brief The PIN service provides a basic central API for faces to check if the service is LOCKED or UNLOCKED
  * 
  * The PIN service defaults to UNLOCKED, and is only enabled if the pin_face is added to the build.
  * 
  * The only part of the API that is relevant for a regular face is watch_pin_service_is_locked() and watch_pin_service_loop().
  * If a face contains sensitive information that it should only display when the watch is unlocked, they can add the following
  * at the top of the face loop function:
  * 
  * if (watch_pin_service_is_locked()) {
  *     return watch_pin_service_loop(event, state->face_index, "totp", "2f");
  * }
  * 
  * Where the two strings are the title and fallback title for the face.
  * 
  * The face will then only display the message "Unlock", and if the user presses the alarm button it will be redirected
  * to the pin_face where they can enter the PIN. Upon entering the correct PIN they will be automatically redirected back
  * to the original face, which will now display the sensitive content previously hidden.
  * 
  * A PIN is a 6 digit sequence. Each digit is a button press (either down or longpress), giving us 6 options per digit,
  * with the following encoding:
  * MODE_DOWN   ->  0
  * MODE_LONG   ->  1
  * LIGHT_DOWN  ->  2
  * LIGHT_LONG  ->  3
  * ALARM_DOWN  ->  4
  * ALARM_LONG  ->  5
  * The total number of possible PINs is then 6 ^ 6 = 46,656
  * 
  * The remaining part of the API is essentially reserved for the pin_face and is used to lock/unlock the service,
  * modify the PIN, and setting up the automatic face redirect.
  */

#include <stdint.h>
#include <stdbool.h>

#include "movement.h"

/*
 * PUBLIC API
 */

/** @brief Check whether the PIN service is in locked or unlocked state */
bool watch_pin_service_is_locked(void);

/** @brief Default face loop that any sensitive face should delegate to while the PIN service is locked
  *
  * Place this snippet at the top of a face loop function to automatically protect the face:
  * 
  * if (watch_pin_service_is_locked()) {
  *     return watch_pin_service_loop(event, state->face_index, "totp", "2f");
  * }
  *
  * @param event The event coming from the face loop.
  * @param face_index The index of the face that we will redirect to after successfully unlocking.
  * @param face_title The title of the face (custom LCD) that will be displayed at the top of the screen.
  * @param face_title_fallback The name of the face (classic LCD) that will be displayed at the top of the screen.
  * 
  * @returns Always returns true.
  */
bool watch_pin_service_loop(movement_event_t event, uint8_t face_index, char* face_title, char* face_title_fallback);

/*
 * PRIVATE API
 */

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

/** @brief Enables the PIN service with the default PIN 000000
  *
  * This function should only be called from the pin_face setup method.
  * Once the PIN service is enabled it can't be disabled.
  */
void watch_pin_service_enable(void);

/** @brief Lock the PIN service
  *
  * This function should only be called from the pin_face.
  */
void watch_pin_service_lock(void);

/** @brief Unlock the PIN service with the provided PIN
  *
  * This function should only be called from the pin_face.
  */
bool watch_pin_service_unlock(watch_pin_t pin);

/** @brief Test if the provided PIN is correct
  *
  * This function should only be called from the pin_face.
  */
bool watch_pin_service_verify(watch_pin_t pin);

/** @brief Change the current PIN
  *
  * This function should only be called from the pin_face.
  */
bool watch_pin_service_set_pin(watch_pin_t old_pin, watch_pin_t new_pin);

/** @brief Get/Set the index of the PIN face that we need to redirect to for unlocking.
  *
  * This function should only be called from the pin_face.
  */
uint8_t watch_pin_service_get_pin_face(void);
void watch_pin_service_set_pin_face(uint8_t face_index);

/** @brief Get/Set the index of the face we will redirect to after successful unlocking.
  *
  * This function should only be called from the pin_face.
  */
uint8_t watch_pin_service_get_requesting_face(void);
void watch_pin_service_set_requesting_face(uint8_t face_index);

#endif
