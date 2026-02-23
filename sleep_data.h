/*
 * MIT License
 *
 * Copyright (c) 2026 dlorp
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

#include <stdint.h>
#include <stdbool.h>

// Sleep orientation states (2 bits per bin)
#define SLEEP_ORIENTATION_UNKNOWN   0  // 00
#define SLEEP_ORIENTATION_FACE_UP   1  // 01
#define SLEEP_ORIENTATION_FACE_DOWN 2  // 10
#define SLEEP_ORIENTATION_TILTED    3  // 11

// Sleep tracking configuration
#define SLEEP_BINS_PER_NIGHT 32   // 32 bins of 15 minutes each = 8 hours
#define SLEEP_BYTES_PER_NIGHT 8   // 32 bins * 2 bits = 64 bits = 8 bytes
#define SLEEP_NIGHTS_STORED 7     // Circular buffer of 7 nights
#define SLEEP_BIN_MINUTES 15      // Each bin represents 15 minutes
#define SLEEP_STORAGE_ROW 30      // Use row 30 of flash storage (rows 0-31 available)

// Sleep window: Dynamically calculated from active hours
// Sleep start = end of active hours, sleep end = start of active hours
#define SLEEP_END_HOUR 7  // Kept for backward compatibility, overridden by active hours

// Data structure for one night of sleep (8 bytes + 2 bytes timestamp = 10 bytes)
typedef struct {
    uint8_t night_data[SLEEP_BYTES_PER_NIGHT];  // Packed orientation data
    uint16_t date_code;  // Encoded date: ((year-2024) << 9) | (month << 5) | day
} sleep_night_t;

// Global sleep tracking data (70 bytes total: 7 nights * 10 bytes each)
typedef struct {
    sleep_night_t nights[SLEEP_NIGHTS_STORED];  // Circular buffer
    uint8_t current_index;  // Which night we're currently writing (0-6)
    uint8_t _padding;  // Align to even boundary
} sleep_data_t;

// Function prototypes
void sleep_tracking_init(void);
void sleep_tracking_log_orientation(uint8_t orientation);
bool sleep_tracking_get_night_data(uint8_t days_ago, sleep_night_t *out_night);
uint8_t sleep_tracking_get_current_bin(void);
uint8_t sleep_tracking_count_orientation_changes(const sleep_night_t *night);
void sleep_tracking_save_to_flash(void);
void sleep_tracking_load_from_flash(void);
