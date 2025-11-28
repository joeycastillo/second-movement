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

#include <stdlib.h>
#include <string.h>
#include "ir_command_face.h"
#include "filesystem.h"
#include "lfs.h"

#ifdef HAS_IR_SENSOR
#include "uart.h"
#endif

// External filesystem instance from filesystem.c
extern lfs_t eeprom_filesystem;

static const char *commands[] = {"ls"};
static const uint8_t num_commands = 1;

static void list_files(ir_command_state_t *state) {
    lfs_dir_t dir;
    int err = lfs_dir_open(&eeprom_filesystem, &dir, "/");
    if (err < 0) {
        state->file_count = 0;
        return;
    }

    struct lfs_info info;
    state->file_count = 0;

    while (state->file_count < 16) {
        int res = lfs_dir_read(&eeprom_filesystem, &dir, &info);
        if (res <= 0) break;

        // Skip . and .. entries
        if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0) continue;

        // Only list files, not directories
        if (info.type == LFS_TYPE_REG) {
            strncpy(state->filenames[state->file_count], info.name, 12);
            state->filenames[state->file_count][12] = '\0';
            state->file_sizes[state->file_count] = info.size;
            state->file_count++;
        }
    }

    lfs_dir_close(&eeprom_filesystem, &dir);
}

static void display_file_list(ir_command_state_t *state) {
    watch_clear_display();

    if (state->file_count == 0) {
        watch_display_text(WATCH_POSITION_TOP, "no    ");
        watch_display_text(WATCH_POSITION_BOTTOM, "FILES ");
        return;
    }

    // Display current file number out of total
    char buf[11];
    snprintf(buf, 11, "%d/%d", state->current_file + 1, state->file_count);
    watch_display_text(WATCH_POSITION_TOP, buf);

    // Display filename (truncated to 6 chars if needed)
    char filename_display[7];
    strncpy(filename_display, state->filenames[state->current_file], 6);
    filename_display[6] = '\0';
    watch_display_text(WATCH_POSITION_BOTTOM, filename_display);
}

static void execute_command(ir_command_state_t *state, const char *cmd) {
    if (strcmp(cmd, "ls") == 0) {
        movement_force_led_on(0, 48, 0);  // Green LED for success
        list_files(state);
        state->current_file = 0;
        state->display_mode = true;
        display_file_list(state);
    } else {
        // Unknown command
        movement_force_led_on(48, 48, 0);  // Yellow LED for unknown
        watch_clear_display();
        watch_display_text(WATCH_POSITION_TOP, "UnKno ");
        watch_display_text(WATCH_POSITION_BOTTOM, "Wn Cmd");
    }
}

void ir_command_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(ir_command_state_t));
        memset(*context_ptr, 0, sizeof(ir_command_state_t));
    }
}

void ir_command_face_activate(void *context) {
    ir_command_state_t *state = (ir_command_state_t *)context;
    state->display_mode = false;
    state->current_file = 0;
    state->file_count = 0;
    state->selected_command = 0;

#ifdef HAS_IR_SENSOR
    // Initialize IR receiver on hardware
    HAL_GPIO_IR_ENABLE_out();
    HAL_GPIO_IR_ENABLE_clr();
    HAL_GPIO_IRSENSE_in();
    HAL_GPIO_IRSENSE_pmuxen(HAL_GPIO_PMUX_SERCOM_ALT);
    uart_init_instance(0, UART_TXPO_NONE, UART_RXPO_0, 900);
    uart_set_irda_mode_instance(0, true);
    uart_enable_instance(0);
#endif
}

bool ir_command_face_loop(movement_event_t event, void *context) {
    ir_command_state_t *state = (ir_command_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_NONE:
            watch_clear_display();
            if (state->display_mode) {
                display_file_list(state);
            } else {
#ifdef HAS_IR_SENSOR
                watch_display_text(WATCH_POSITION_TOP, "IR    ");
                watch_display_text(WATCH_POSITION_BOTTOM, "Cmd   ");
#else
                // In simulator, show selected command
                watch_display_text(WATCH_POSITION_TOP, "Cmd   ");
                watch_display_text(WATCH_POSITION_BOTTOM, (char *)commands[state->selected_command]);
#endif
            }
            break;

        case EVENT_TICK:
        {
#ifdef HAS_IR_SENSOR
            // Hardware mode: read from IR sensor
            char data[64];
            size_t bytes_read = uart_read_instance(0, data, 63);

            if (bytes_read > 0) {
                data[bytes_read] = '\0';

                // Trim whitespace/newlines
                while (bytes_read > 0 && (data[bytes_read-1] == '\n' || data[bytes_read-1] == '\r' || data[bytes_read-1] == ' ')) {
                    data[bytes_read-1] = '\0';
                    bytes_read--;
                }

                if (bytes_read > 0) {
                    execute_command(state, data);
                }
            } else {
                movement_force_led_off();
                if (!state->display_mode) {
                    // Blink indicator to show we're waiting
                    if (watch_rtc_get_date_time().unit.second % 2 == 0) {
                        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
                    } else {
                        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
                    }
                }
            }
#else
            // Simulator mode: blink to show we're active
            if (!state->display_mode) {
                if (watch_rtc_get_date_time().unit.second % 2 == 0) {
                    watch_set_indicator(WATCH_INDICATOR_BELL);
                } else {
                    watch_clear_indicator(WATCH_INDICATOR_BELL);
                }
            }
#endif
        }
            break;

        case EVENT_LIGHT_BUTTON_UP:
#ifndef HAS_IR_SENSOR
            // In simulator mode: cycle through available commands
            if (!state->display_mode) {
                state->selected_command = (state->selected_command + 1) % num_commands;
                watch_display_text(WATCH_POSITION_BOTTOM, (char *)commands[state->selected_command]);
            }
#endif
            break;

        case EVENT_LIGHT_LONG_PRESS:
#ifndef HAS_IR_SENSOR
            // In simulator mode: execute selected command
            if (!state->display_mode) {
                execute_command(state, commands[state->selected_command]);
            }
#endif
            break;

        case EVENT_ALARM_BUTTON_UP:
            if (state->display_mode && state->file_count > 0) {
                // Navigate through file list
                state->current_file = (state->current_file + 1) % state->file_count;
                display_file_list(state);
            }
#ifndef HAS_IR_SENSOR
            else if (!state->display_mode) {
                // In simulator mode: also allow cycling commands with ALARM button
                state->selected_command = (state->selected_command + 1) % num_commands;
                watch_display_text(WATCH_POSITION_BOTTOM, (char *)commands[state->selected_command]);
            }
#endif
            break;

        case EVENT_ALARM_LONG_PRESS:
            // Return to command mode
            if (state->display_mode) {
                state->display_mode = false;
                watch_clear_display();
#ifdef HAS_IR_SENSOR
                watch_display_text(WATCH_POSITION_TOP, "IR    ");
                watch_display_text(WATCH_POSITION_BOTTOM, "Cmd   ");
#else
                watch_display_text(WATCH_POSITION_TOP, "Cmd   ");
                watch_display_text(WATCH_POSITION_BOTTOM, (char *)commands[state->selected_command]);
#endif
            }
#ifndef HAS_IR_SENSOR
            else {
                // In simulator mode: execute command on long press
                execute_command(state, commands[state->selected_command]);
            }
#endif
            break;

        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;

        case EVENT_LOW_ENERGY_UPDATE:
            watch_display_text(WATCH_POSITION_TOP_RIGHT, " <");
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void ir_command_face_resign(void *context) {
    (void) context;
#ifdef HAS_IR_SENSOR
    uart_disable_instance(0);
    HAL_GPIO_IRSENSE_pmuxdis();
    HAL_GPIO_IRSENSE_off();
    HAL_GPIO_IR_ENABLE_off();
#endif
}

#ifdef HAS_IR_SENSOR
void irq_handler_sercom0(void);
void irq_handler_sercom0(void) {
    uart_irq_handler(0);
}
#endif
