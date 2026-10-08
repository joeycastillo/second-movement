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

#include "pin_face.h"

#include "movement_pin_service.h"
#include "watch_common_display.h"

#include <stdlib.h>


typedef enum {
    PIN_STATUS_MENU = 0,
    PIN_STATUS_ENTERING,
    PIN_STATUS_VALIDATING,
    PIN_STATUS_TIMEOUT,
} pin_status_t;

typedef enum {
    PIN_MENU_UNLOCK = 0,
    PIN_MENU_CHANGE,
    PIN_MENU_TIMEOUT,
} pin_menu_page_t;

typedef enum {
    PIN_ENTERING_UNLOCK = 0,
    PIN_ENTERING_OLD,
    PIN_ENTERING_NEW,
    PIN_ENTERING_NEW_CONFIRM,
} pin_entering_reason_t;

typedef enum {
    PIN_TIMEOUT_1MIN = 0,
    PIN_TIMEOUT_5MIN,
    PIN_TIMEOUT_10MIN,
    PIN_TIMEOUT_60MIN
} pin_timeout_page_t;

typedef enum {
    PIN_BUTTON_NONE = 0,
    PIN_BUTTON_MODE,
    PIN_BUTTON_LIGHT,
    PIN_BUTTON_ALARM
} pin_button_t;

typedef struct {
    void (*transition)(movement_event_t event, void* context);
    void (*display)(movement_event_t event, void* context);
} pin_face_screen_t;

typedef struct {
    uint8_t digit_index;
    watch_pin_t scratch_pin;
    watch_pin_t current_pin;
    watch_pin_t new_pin;
    watch_pin_t new_pin_confirm;
    pin_status_t status;
    uint8_t animation_tick;
    bool validating;
    bool validation_success;
    pin_menu_page_t menu_page;
    pin_entering_reason_t entering_reason;
    pin_timeout_page_t timeout_page;
    uint8_t lock_timeout;
    uint8_t unlock_elapsed;
    pin_face_screen_t screens[6];
    pin_button_t active_button;
} pin_state_t;

static uint8_t _get_pin_digit(watch_pin_t* pin, uint8_t digit_index) {
    switch (digit_index) {

        case 0:
            return pin->unit.digit0;

        case 1:
            return pin->unit.digit1;

        case 2:
            return pin->unit.digit2;

        case 3:
            return pin->unit.digit3;
        
        case 4:
            return pin->unit.digit4;

        case 5:
            return pin->unit.digit5;
        
        default:
            return PIN_EMPTY_DIGIT;
    }
}

static void _set_pin_digit(watch_pin_t* pin, uint8_t digit_index, uint8_t digit_value) {
    switch (digit_index) {

        case 0:
            pin->unit.digit0 = digit_value;
            return;

        case 1:
            pin->unit.digit1 = digit_value;
            return;

        case 2:
            pin->unit.digit2 = digit_value;
            return;

        case 3:
            pin->unit.digit3 = digit_value;
            return;
        
        case 4:
            pin->unit.digit4 = digit_value;
            return;

        case 5:
            pin->unit.digit5 = digit_value;
            return;
        
        default:
            return;
    }
}

static void _clear_pin(watch_pin_t* pin) {
    pin->reg = 0;
    for (uint8_t i = 0; i < 6; i++) {
        _set_pin_digit(pin, i, PIN_EMPTY_DIGIT);
    }
}

static void _display_pin(watch_pin_t* pin) {
    char buf[2];

    for (uint8_t i = 0; i < 6; i++) {
        uint8_t digit_value = _get_pin_digit(pin, i);

        if (digit_value == PIN_EMPTY_DIGIT) {
            buf[0] = '-';
            buf[1] = '\0';
        } else {
            snprintf(
                buf,
                sizeof(buf),
                "%1d",
                digit_value
            );
        }

        watch_display_character(buf[0], WATCH_POSITION_BOTTOM + i);
    }
}

void _pin_face_default_loop_handler(movement_event_t event) {
    switch (event.event_type) {
        case EVENT_MODE_BUTTON_UP: {
            uint8_t requesting_face = movement_pin_service_get_requesting_face();
            if (requesting_face != PIN_EMPTY_FACE) {
                movement_move_to_face(requesting_face);
            } else {
                movement_move_to_next_face();
            }
            break;
        }
        default:
            movement_default_loop_handler(event);
    }
}

void _pin_face_menu_transition(movement_event_t event, void* context) {
    pin_state_t *state = (pin_state_t *)context;

    switch (event.event_type) {
        case EVENT_LIGHT_BUTTON_DOWN:
            state->menu_page = (state->menu_page + 1) % 3;
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            switch (state->menu_page) {
                case PIN_MENU_UNLOCK:
                    if (movement_pin_service_is_locked()) {
                        state->status = PIN_STATUS_ENTERING;
                        state->entering_reason = PIN_ENTERING_UNLOCK;
                    } else {
                        movement_pin_service_lock();
                    }
                    break;
                case PIN_MENU_CHANGE:
                    state->status = PIN_STATUS_ENTERING;
                    state->entering_reason = PIN_ENTERING_OLD;
                    break;
                case PIN_MENU_TIMEOUT:
                    state->status = PIN_STATUS_TIMEOUT;
                    break;
                default:
                    return;
            }
            break;
        
        default:
            _pin_face_default_loop_handler(event);
            break;
    }
}

void _pin_face_menu_display(movement_event_t event, void* context) {
    pin_state_t *state = (pin_state_t *)context;

    watch_clear_display();
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "Pin", "Pn");
    switch (state->menu_page) {
        case PIN_MENU_UNLOCK:
            if (movement_pin_service_is_locked()) {
                watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Unlock", " Unloc");
            } else {
                watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Lock", " LOCK");
            }
            break;
        case PIN_MENU_CHANGE:
            watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Change", "Edit");
            break;
        case PIN_MENU_TIMEOUT:
            watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "TiMER", " TIMER");
            break;
        default:
            break;
    }
}

void _pin_face_advance_digit(pin_state_t* state) {
    state->digit_index += 1;

    if (state->digit_index == 6) {
        state->digit_index = 0;
        state->animation_tick = 0;
        switch (state->entering_reason) {
            case PIN_ENTERING_UNLOCK:
            case PIN_ENTERING_OLD:
                state->current_pin.reg = state->scratch_pin.reg;
                _clear_pin(&state->scratch_pin);
                state->status = PIN_STATUS_VALIDATING;
                state->validating = true;
                break;
            case PIN_ENTERING_NEW:
                state->new_pin.reg = state->scratch_pin.reg;
                _clear_pin(&state->scratch_pin);
                state->status = PIN_STATUS_ENTERING;
                state->entering_reason = PIN_ENTERING_NEW_CONFIRM;
                break;
            case PIN_ENTERING_NEW_CONFIRM:
                state->new_pin_confirm.reg = state->scratch_pin.reg;
                _clear_pin(&state->scratch_pin);
                state->status = PIN_STATUS_VALIDATING;
                state->validating = true;
                break;
        }
    }
}

void _pin_face_entering_transition(movement_event_t event, void* context) {
    pin_state_t *state = (pin_state_t *)context;

    switch (event.event_type) {
        case EVENT_TICK:
            state->animation_tick += 1;
            break;
        case EVENT_MODE_BUTTON_DOWN:
            if (state->active_button == PIN_BUTTON_NONE) {
                state->active_button = PIN_BUTTON_MODE;
                _set_pin_digit(&state->scratch_pin, state->digit_index, 0);
            }
            break;
        case EVENT_MODE_LONG_PRESS:
            if (state->active_button == PIN_BUTTON_MODE) {
                _set_pin_digit(&state->scratch_pin, state->digit_index, 1);
            }
            break;
        case EVENT_MODE_BUTTON_UP:
        case EVENT_MODE_LONG_UP:
            if (state->active_button == PIN_BUTTON_MODE) {
                state->active_button = PIN_BUTTON_NONE;
                _pin_face_advance_digit(state);
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            if (state->active_button == PIN_BUTTON_NONE) {
                state->active_button = PIN_BUTTON_LIGHT;
                _set_pin_digit(&state->scratch_pin, state->digit_index, 2);
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (state->active_button == PIN_BUTTON_LIGHT) {
                _set_pin_digit(&state->scratch_pin, state->digit_index, 3);
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
        case EVENT_LIGHT_LONG_UP:
            if (state->active_button == PIN_BUTTON_LIGHT) {
                state->active_button = PIN_BUTTON_NONE;
                _pin_face_advance_digit(state);
            }
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            if (state->active_button == PIN_BUTTON_NONE) {
                state->active_button = PIN_BUTTON_ALARM;
                _set_pin_digit(&state->scratch_pin, state->digit_index, 4);
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (state->active_button == PIN_BUTTON_ALARM) {
                _set_pin_digit(&state->scratch_pin, state->digit_index, 5);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
        case EVENT_ALARM_LONG_UP:
            if (state->active_button == PIN_BUTTON_ALARM) {
                state->active_button = PIN_BUTTON_NONE;
                _pin_face_advance_digit(state);
            }
            break;
        default:
            _pin_face_default_loop_handler(event);
    }
}

void _pin_face_entering_display(movement_event_t event, void* context) {
    pin_state_t *state = (pin_state_t *)context;

    watch_clear_display();
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "Pin", "Pn");

    if (state->entering_reason == PIN_ENTERING_UNLOCK) {
        _display_pin(&state->scratch_pin);
        return;
    }
    
    if (state->animation_tick < 2) {
        if (state->entering_reason == PIN_ENTERING_OLD) {
            watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Old PN", " Old");
        } else if (state->entering_reason == PIN_ENTERING_NEW) {
            watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "New PN", " New");
        } else if (state->entering_reason == PIN_ENTERING_NEW_CONFIRM) {
            watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "CONFRM", " Conf");
        }
    } else {
        _display_pin(&state->scratch_pin);
    }
}

void _pin_face_validating_transition(movement_event_t event, void* context) {
    pin_state_t *state = (pin_state_t *)context;

    switch (event.event_type) {
        case EVENT_TICK:
            state->animation_tick += 1;
            if (state->animation_tick == 1) {
                state->validating = false;
                switch (state->entering_reason) {
                    case PIN_ENTERING_UNLOCK:
                        state->validation_success = movement_pin_service_unlock(state->current_pin);
                        if (state->validation_success) {
                            state->unlock_elapsed = 0;
                        }
                        break;
                    case PIN_ENTERING_OLD:
                        state->validation_success = movement_pin_service_verify(state->current_pin);
                        break;
                    case PIN_ENTERING_NEW_CONFIRM:
                        state->validation_success = state->new_pin.reg == state->new_pin_confirm.reg;
                        if (state->validation_success) {
                            movement_pin_service_set_pin(state->current_pin, state->new_pin);
                        }
                        break;
                    default:
                        break;
                }
            } else if (state->animation_tick == 2) {
                switch (state->entering_reason) {
                    case PIN_ENTERING_UNLOCK:
                        if (state->validation_success) {
                            uint8_t requesting_face = movement_pin_service_get_requesting_face();
                            if (requesting_face != PIN_EMPTY_FACE) {
                                movement_move_to_face(requesting_face);
                            } else {
                                state->digit_index = 0;
                                state->animation_tick = 0;
                                state->status = PIN_STATUS_MENU;
                            }
                        }
                        break;
                    case PIN_ENTERING_OLD:
                        if (state->validation_success) {
                            state->digit_index = 0;
                            state->animation_tick = 0;
                            state->status = PIN_STATUS_ENTERING;
                            state->entering_reason = PIN_ENTERING_NEW;
                        }
                        break;
                    case PIN_ENTERING_NEW_CONFIRM:
                        if (state->validation_success) {
                            state->digit_index = 0;
                            state->animation_tick = 0;
                            state->status = PIN_STATUS_MENU;
                        } else {
                            state->entering_reason = PIN_ENTERING_NEW;
                        }
                        break;
                    default:
                        break;
                }
            }
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            if (!state->validating && !state->validation_success) {
                state->status = PIN_STATUS_ENTERING;
                state->digit_index = 0;
                state->animation_tick = 0;
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            if (!state->validating && !state->validation_success) {
                state->status = PIN_STATUS_MENU;
                state->digit_index = 0;
                state->animation_tick = 0;
            }
            break;
        default:
            if (!state->validating) {
                _pin_face_default_loop_handler(event);
            }
            break;
    }
}

void _pin_face_validating_display(movement_event_t event, void* context) {
    pin_state_t *state = (pin_state_t *)context;

    watch_clear_display();
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "Pin", "Pn");

    if (state->validating) {
        watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "testin", " test ");
    } else {
        if (state->validation_success) {
            watch_display_text(WATCH_POSITION_BOTTOM, "SUCCES");
        } else {
            watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "ERROR", " ERROR");
        }
    }
}

void _pin_face_timeout_transition(movement_event_t event, void* context) {
    pin_state_t *state = (pin_state_t *)context;

    switch (event.event_type) {
        case EVENT_LIGHT_BUTTON_DOWN:
            state->timeout_page = (state->timeout_page + 1) % 4;
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            switch (state->timeout_page) {
                case PIN_TIMEOUT_1MIN:
                    state->lock_timeout = 1;
                    break;
                case PIN_TIMEOUT_5MIN:
                    state->lock_timeout = 5;
                    break;
                case PIN_TIMEOUT_10MIN:
                    state->lock_timeout = 10;
                    break;
                case PIN_TIMEOUT_60MIN:
                    state->lock_timeout = 60;
                    break;
                default:
                    break;
            }

            state->status = PIN_STATUS_MENU;

            break;
        
        default:
            _pin_face_default_loop_handler(event);
            break;
    }
}

void _pin_face_timeout_display(movement_event_t event, void* context) {
    pin_state_t *state = (pin_state_t *)context;

    watch_clear_display();
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "Pin", "Pn");

    switch (state->timeout_page) {
        case PIN_TIMEOUT_1MIN:
            watch_display_text(WATCH_POSITION_BOTTOM, "1  min");
            break;
        case PIN_TIMEOUT_5MIN:
            watch_display_text(WATCH_POSITION_BOTTOM, "5  min");
            break;
        case PIN_TIMEOUT_10MIN:
            watch_display_text(WATCH_POSITION_BOTTOM, "10 min");
            break;
        case PIN_TIMEOUT_60MIN:
            watch_display_text(WATCH_POSITION_BOTTOM, "60 min");
            break;
        default:
            break;
    }
}

static void _pin_face_reset_state(pin_state_t* state) {
    state->digit_index = 0;
    _clear_pin(&state->scratch_pin);
    _clear_pin(&state->current_pin);
    _clear_pin(&state->new_pin);
    _clear_pin(&state->new_pin_confirm);
    state->status = PIN_STATUS_MENU;
    state->animation_tick = 0;
    state->active_button = PIN_BUTTON_NONE;
    state->validating = false;
    state->validation_success = false;
    state->menu_page = 0;
    state->entering_reason = PIN_ENTERING_UNLOCK;
}

void pin_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(pin_state_t));
        pin_state_t *state = (pin_state_t *)*context_ptr;

        state->timeout_page = PIN_TIMEOUT_5MIN;
        state->lock_timeout = 5;
        state->active_button = PIN_BUTTON_NONE;

        movement_pin_service_enable();
        movement_pin_service_set_pin_face(watch_face_index);

        state->screens[PIN_STATUS_MENU].transition = _pin_face_menu_transition;
        state->screens[PIN_STATUS_MENU].display = _pin_face_menu_display;
        state->screens[PIN_STATUS_ENTERING].transition = _pin_face_entering_transition;
        state->screens[PIN_STATUS_ENTERING].display = _pin_face_entering_display;
        state->screens[PIN_STATUS_VALIDATING].transition = _pin_face_validating_transition;
        state->screens[PIN_STATUS_VALIDATING].display = _pin_face_validating_display;
        state->screens[PIN_STATUS_TIMEOUT].transition = _pin_face_timeout_transition;
        state->screens[PIN_STATUS_TIMEOUT].display = _pin_face_timeout_display;
    }
}

void pin_face_activate(void *context) {
    pin_state_t *state = (pin_state_t *)context;
    _pin_face_reset_state(state);
    if (movement_pin_service_get_requesting_face() == PIN_EMPTY_FACE) {
        state->status = PIN_STATUS_MENU;
    } else {
        // If we got redirected by another page, go straight to unlocking screen
        state->status = PIN_STATUS_ENTERING;
        state->entering_reason = PIN_ENTERING_UNLOCK;
    }
}

bool pin_face_loop(movement_event_t event, void *context) {
    pin_state_t *state = (pin_state_t *)context;

    pin_face_screen_t* screen = &state->screens[state->status];
    screen->transition(event, state);

    // status may have changed during the transition phase
    screen = &state->screens[state->status];
    screen->display(event, state);

    return true;
}

void pin_face_resign(void *context) {
    (void) context;
    movement_pin_service_set_requesting_face(PIN_EMPTY_FACE);
}

movement_watch_face_advisory_t pin_face_advise(void *context) {
    movement_watch_face_advisory_t retval = { 0 };
    pin_state_t *state = (pin_state_t *)context;

    if (!movement_pin_service_is_locked()) {
        if (state->unlock_elapsed >= state->lock_timeout) {
            movement_pin_service_lock();
        } else {
            state->unlock_elapsed += 1;
        }
    }

    return retval;
}
