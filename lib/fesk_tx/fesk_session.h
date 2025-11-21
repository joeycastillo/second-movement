/*
 * MIT License
 *
 * Copyright (c) 2025 Eirik S. Morland
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

/**
 * @file fesk_session.h
 * @brief FESK Session Management
 *
 * High-level API for managing FESK transmissions with countdown timers,
 * lifecycle callbacks, and visual feedback.
 *
 * Features:
 *   - Optional countdown with configurable duration and beeps
 *   - Comprehensive callback system for all transmission phases
 *   - Bell indicator management during transmission
 *   - Singleton session to prevent buzzer conflicts
 *
 * IMPORTANT: Only one session can transmit at a time due to hardware
 * limitations (single piezo buzzer). Starting a new session will abort
 * any currently active transmission.
 *
 * Callback Lifecycle (normal flow):
 *   1. on_countdown_begin (if countdown enabled)
 *   2. on_countdown_tick (countdown_seconds times, counting down to 0)
 *   3. on_countdown_complete (when countdown reaches 0)
 *   4. on_sequence_ready (after encoding, before playback)
 *   5. on_transmission_start (when buzzer starts)
 *   6. on_transmission_end (when buzzer finishes)
 *
 * Cancelled flow:
 *   - on_cancelled (if fesk_session_cancel called during countdown/transmission)
 *
 * Error flow:
 *   - on_error (if encoding fails or invalid payload provided)
 *
 * Example Usage:
 *   fesk_session_t session;
 *   fesk_session_config_t config = fesk_session_config_defaults();
 *   config.static_message = "Hello";
 *   config.on_transmission_end = my_done_callback;
 *   fesk_session_init(&session, &config);
 *   fesk_session_start(&session);     // Starts countdown -> transmission
 *   // ... wait for callbacks ...
 *   fesk_session_dispose(&session);
 */

#ifndef FESK_SESSION_H
#define FESK_SESSION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fesk_tx.h"
#include "movement.h"

/** Callback to dynamically provide payload text at transmission time */
typedef fesk_result_t (*fesk_session_payload_cb)(const char **out_text,
                                                 size_t *out_length,
                                                 void *user_data);

/** Simple lifecycle event callback */
typedef void (*fesk_session_simple_cb)(void *user_data);

/** Error event callback */
typedef void (*fesk_session_error_cb)(fesk_result_t error, void *user_data);

/** Countdown tick callback (receives seconds remaining) */
typedef void (*fesk_session_countdown_cb)(uint8_t seconds_remaining, void *user_data);

/** Sequence ready callback (provides encoded sequence before transmission) */
typedef void (*fesk_session_sequence_cb)(const int8_t *sequence,
                                         size_t entries,
                                         void *user_data);

/** Configuration for FESK session behavior and callbacks */
typedef struct {
    bool enable_countdown;                      /**< Enable countdown timer before transmission */
    uint8_t countdown_seconds;                  /**< Countdown duration (default: 3 seconds) */
    bool countdown_beep;                        /**< Play beep during countdown */
    bool show_bell_indicator;                   /**< Show bell icon during countdown/transmission */
    const char *static_message;                 /**< Static message to transmit (or NULL if using provide_payload) */
    fesk_session_payload_cb provide_payload;    /**< Dynamic payload callback (overrides static_message) */
    fesk_session_simple_cb on_countdown_begin;  /**< Called when countdown starts */
    fesk_session_countdown_cb on_countdown_tick;/**< Called each countdown second */
    fesk_session_simple_cb on_countdown_complete;/**< Called when countdown reaches 0 */
    fesk_session_simple_cb on_transmission_start;/**< Called when buzzer starts playing */
    fesk_session_sequence_cb on_sequence_ready; /**< Called after encoding, before playback */
    fesk_session_simple_cb on_transmission_end; /**< Called when transmission completes */
    fesk_session_simple_cb on_cancelled;        /**< Called if session cancelled */
    fesk_session_error_cb on_error;             /**< Called on encoding or validation errors */
    void *user_data;                            /**< User data passed to all callbacks */
} fesk_session_config_t;

/** Session phase states */
typedef enum {
    FESK_SESSION_IDLE = 0,          /**< Not active, ready to start */
    FESK_SESSION_COUNTDOWN,         /**< Countdown in progress */
    FESK_SESSION_TRANSMITTING,      /**< Transmitting audio */
} fesk_session_phase_t;

/** Session state structure */
typedef struct fesk_session_s {
    fesk_session_config_t config;   /**< Session configuration */
    fesk_session_phase_t phase;     /**< Current phase */
    uint8_t seconds_remaining;      /**< Countdown seconds remaining */
    int8_t *sequence;               /**< Encoded sequence (managed internally) */
    size_t sequence_entries;        /**< Number of sequence entries */
} fesk_session_t;

/**
 * @brief Get default session configuration
 * @return Default configuration with 3-second countdown, beeps enabled
 */
fesk_session_config_t fesk_session_config_defaults(void);

/**
 * @brief Initialize session with configuration
 * @param session Session to initialize (must be valid)
 * @param config Configuration to use (NULL = use defaults)
 */
void fesk_session_init(fesk_session_t *session, const fesk_session_config_t *config);

/**
 * @brief Dispose session and free resources
 * @param session Session to dispose (NULL-safe)
 */
void fesk_session_dispose(fesk_session_t *session);

/**
 * @brief Start transmission (begins countdown if enabled, then transmits)
 * @param session Session to start (must be idle)
 * @return true if started, false if already active
 */
bool fesk_session_start(fesk_session_t *session);

/**
 * @brief Cancel active transmission or countdown
 * @param session Session to cancel
 */
void fesk_session_cancel(fesk_session_t *session);

/**
 * @brief Check if session is idle (not counting down or transmitting)
 * @param session Session to check (NULL = considered idle)
 * @return true if idle, false otherwise
 */
bool fesk_session_is_idle(const fesk_session_t *session);

#endif
