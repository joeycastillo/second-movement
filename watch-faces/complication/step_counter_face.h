/*
 * MIT License
 *
 * Copyright (c) 2025 David Volovskiy
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

#ifndef STEP_COUNTER_FACE_H_
#define STEP_COUNTER_FACE_H_

#define STEP_COUNTER_NUM_DATA_POINTS (36)

/*
 * STEP_COUNTER face
 * ======================
 * This face displays the current step count seen for the current day.
 * Holding the ALARM button will reset the count.
 * 
 * The step count gets reset at 11:59PM every night.
 * To see the steps of previous days, you can cycle through them
 * by using the ALARM and LIGHT buttons. The top-right displays the day of
 * the month that you're currently viewing.
 * You can see up to STEP_COUNTER_NUM_DATA_POINTS days.
 * 
 * Step counting works by setting the accelerometer to read at 12.5Hz
 * and every second, it takes the X,Y,Z data out of the FIFO buffer
 * of the accelerometer. We then run the samples through a modified
 * stepcount library found in the Espruino prject.
 * (https://github.com/espruino/Espruino/blob/master/libs/misc/stepcount.c)
 * The accelerometer's FIFO can hold 32 reads based on the LISDW12 datasheet,
 * so we shouldn't be missing data if we pull every second.
 * 
 * The steps counter is designed to count steps on all faces that
 * don't use the accelerometer.
 * With the LIS2DW accelerometer, about 1.5uA are used when the 
 * watch isn't seeing movement, and 10uA extra are used when movement is seen.
 * Due to this, there is a setting in the Settings face to choose when to
 * disable the step count. The Step Counter face will always count steps when you're
 * on it, but that setting affects counting steps on other faces.
 * See MOVEMENT_DEFAULT_COUNT_STEPS to find the options.
 * To avoid unnecissarily turning the counter on and off when scrolling through faces, 
 * the step counter does not turn off immedietly. For example, if you have the setting set to
 * only count between 5am and 10pm, and you're scrolling past the step-counter face, it will not
 * start counting steps for STEP_COUNTER_MINUTES_SEC_BEFORE_START seconds.
 * Also, if you are counting steps and scroll to a face that isn't using the accelerometer,
 * but not during a time that counts steps, the step counter will not turn off
 * for movement_step_count_disable_delay_sec seconds.
 * 
 * With the LIS2DW, the threshold to when to count steps is done via an interrupt
 * pin that triggers when a certain G-force is seen. If you are not seeing steps trigger
 * from movmeent, please check threshold variable in movement_enable_step_count.
 */


#include "movement.h"

typedef struct {
    uint8_t day;
    uint32_t step_count;
} step_counter_face_data_point_t;

typedef struct {
    uint32_t step_count_prev;
    uint16_t sec_inactivity : 14;
    uint16_t can_sleep : 1;
    uint16_t sensor_seen : 1;
    uint8_t sec_before_starting;
    uint8_t display_index;  // the index we are displaying on screen
    int32_t data_points;    // the absolute number of data points logged
    step_counter_face_data_point_t data[STEP_COUNTER_NUM_DATA_POINTS];
} step_counter_state_t;

void step_counter_face_setup(uint8_t watch_face_index, void ** context_ptr);
void step_counter_face_activate(void *context);
bool step_counter_face_loop(movement_event_t event, void *context);
void step_counter_face_resign(void *context);
movement_watch_face_advisory_t step_counter_face_advise(void *context);

#define step_counter_face ((const watch_face_t){ \
    step_counter_face_setup, \
    step_counter_face_activate, \
    step_counter_face_loop, \
    step_counter_face_resign, \
    step_counter_face_advise, \
})

#endif // STEP_COUNTER_FACE_H_
