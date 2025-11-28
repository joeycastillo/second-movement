/*
 * MIT License
 *
 * Copyright (c) 2025
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
 * IR COMMAND FACE
 *
 * Watch face that can receive IR commands and execute them.
 * Currently supports:
 * - ls: List files in the filesystem
 *
 * In hardware mode: Receives commands via IR sensor
 * In simulator mode: Use ALARM button to cycle through commands,
 *                    LIGHT long-press to execute
 */

typedef struct {
    uint8_t file_count;
    uint8_t current_file;
    char filenames[16][13];  // Up to 16 files, 12 chars + null terminator
    int32_t file_sizes[16];
    bool display_mode;  // false = showing command prompt, true = showing results
    uint8_t selected_command;  // For simulator: which command is selected
} ir_command_state_t;

void ir_command_face_setup(uint8_t watch_face_index, void ** context_ptr);
void ir_command_face_activate(void *context);
bool ir_command_face_loop(movement_event_t event, void *context);
void ir_command_face_resign(void *context);

#define ir_command_face ((const watch_face_t){ \
    ir_command_face_setup, \
    ir_command_face_activate, \
    ir_command_face_loop, \
    ir_command_face_resign, \
    NULL, \
})
