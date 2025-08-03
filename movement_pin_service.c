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

#include "movement_pin_service.h"

const uint8_t PIN_EMPTY_DIGIT = 15;
const uint8_t PIN_EMPTY_FACE = 255;

movement_pin_service_state_t pin_service_state = {
    .enabled = false,
    .locked = false,
};

void movement_pin_service_enable(void) {
    if (pin_service_state.enabled) {
        return;
    }

    pin_service_state.enabled = true;
    pin_service_state.locked = true;
    pin_service_state.pin.reg = 0;
    pin_service_state.pin_face_index = PIN_EMPTY_FACE;
    pin_service_state.requesting_face_index = PIN_EMPTY_FACE;
}

bool movement_pin_service_is_locked() {
    if (!pin_service_state.enabled) {
        return false;
    }

    return pin_service_state.locked;
}

void movement_pin_service_lock(void) {
    pin_service_state.locked = true;
}

bool movement_pin_service_unlock(watch_pin_t pin) {
    if (movement_pin_service_verify(pin)) {
        pin_service_state.locked = false;
        return true;
    } else {
        return false;
    }
}

bool movement_pin_service_verify(watch_pin_t pin) {
    return pin.reg == pin_service_state.pin.reg;
}

bool movement_pin_service_set_pin(watch_pin_t old_pin, watch_pin_t new_pin) {
    if (movement_pin_service_verify(old_pin)) {
        pin_service_state.pin.reg = new_pin.reg;
        return true;
    } else {
        return false;
    }
}

uint8_t movement_pin_service_get_pin_face(void) {
    return pin_service_state.pin_face_index;
}

void movement_pin_service_set_pin_face(uint8_t face_index) {
    pin_service_state.pin_face_index = face_index;
}

uint8_t movement_pin_service_get_requesting_face(void) {
    return pin_service_state.requesting_face_index;
}

void movement_pin_service_set_requesting_face(uint8_t face_index) {
    pin_service_state.requesting_face_index = face_index;
}

bool movement_pin_service_loop(movement_event_t event, uint8_t face_index, char* face_title, char* face_title_fallback) {
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            watch_clear_display();
            watch_display_text_with_fallback(WATCH_POSITION_TOP, face_title, face_title_fallback);
            watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Unlock", " Unloc");
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            movement_pin_service_set_requesting_face(face_index);
            movement_move_to_face(movement_pin_service_get_pin_face());
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}
