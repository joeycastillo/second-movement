/*
 * MIT License
 *
 * Copyright (c) 2024 Joey Castillo
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
#include "light_sensor_face.h"
#include "tc.h"
#include "eic.h"
#include "usb.h"

void light_sensor_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(light_sensor_state_t));
        memset(*context_ptr, 0, sizeof(light_sensor_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }    
}

void light_sensor_face_activate(void *context) {
    light_sensor_state_t *state = (light_sensor_state_t *)context;
    (void) state;
    HAL_GPIO_IR_ENABLE_out();
    HAL_GPIO_IR_ENABLE_clr();
    HAL_GPIO_IRSENSE_in();
    eic_configure_pin(HAL_GPIO_IRSENSE_pin(), INTERRUPT_TRIGGER_FALLING);
    eic_enable_event(HAL_GPIO_IRSENSE_pin());
    EVSYS->USER[EVSYS_ID_USER_TC1_EVU].reg = EVSYS_USER_CHANNEL(1 + 1);
    // Set up channel 1:
    EVSYS->CHANNEL[1].reg = EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_EIC_EXTINT_4) | // External interrupt 4 generates events on channel 1...
                            EVSYS_CHANNEL_RUNSTDBY |                         // even in standby mode...
                            EVSYS_CHANNEL_PATH_ASYNCHRONOUS;                 // on the asynchronous path (event drives action directly)
    tc_init(1, GENERIC_CLOCK_0, usb_is_enabled() ? TC_PRESCALER_DIV16 : TC_PRESCALER_DIV8);
    tc_set_counter_mode(1, TC_COUNTER_MODE_16BIT);
    TC1->COUNT16.CTRLA.bit.CAPTEN0 = 1;
    TC1->COUNT16.CTRLA.bit.CAPTEN1 = 1;
    TC1->COUNT16.EVCTRL.reg = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_PWP;   // Event should capture pulse width and period

    tc_set_counter_mode(1, TC_COUNTER_MODE_16BIT);
    tc_enable(1);

    movement_request_tick_frequency(64);
}

bool light_sensor_face_loop(movement_event_t event, void *context) {
    light_sensor_state_t *state = (light_sensor_state_t *)context;
    (void) state;

    bool light_level = HAL_GPIO_IRSENSE_read();
    uint16_t pulsewidth = TC1->COUNT16.CC[0].reg;
    uint16_t period = TC1->COUNT16.CC[1].reg;
    if (period > 10000) printf("Light %d, %d, %d\n", light_level, pulsewidth >> 3, period >> 3);

    switch (event.event_type) {
        case EVENT_NONE:
        case EVENT_ACTIVATE:
            // Show your initial UI here.
            break;
        case EVENT_TICK:
            // If needed, update your display here.
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // You can use the Light button for your own purposes. Note that by default, Movement will also
            // illuminate the LED in response to EVENT_LIGHT_BUTTON_DOWN; to suppress that behavior, add an
            // empty case for EVENT_LIGHT_BUTTON_DOWN.
            break;
        case EVENT_ALARM_BUTTON_UP:
            // Just in case you have need for another button.
            break;
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

    return false;
}

void light_sensor_face_resign(void *context) {
    (void) context;

    // handle any cleanup before your watch face goes off-screen.
}
