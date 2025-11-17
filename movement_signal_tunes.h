/*
 * MIT License
 *
 * Copyright (c) 2023 Jeremy O'Brien
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

/// NULL-terminated array of available signal tunes
extern const int8_t * const movement_signal_tunes[];

typedef enum {
    MOVEMENT_TUNE_MODE_CHIME = 0,    /// Hourly chime signal tune
    MOVEMENT_TUNE_MODE_ALARM,        /// Alarm signal tune
    MOVEMENT_TUNE_MODE_TIMER,        /// Timer signal tune
    MOVEMENT_NUM_TUNE_MODES,
} movement_tune_mode_t;

/// User-selected signal tunes for each mode, indexed by movement_tune_mode_t.
extern const int8_t *movement_selected_signal_tunes[MOVEMENT_NUM_TUNE_MODES];
