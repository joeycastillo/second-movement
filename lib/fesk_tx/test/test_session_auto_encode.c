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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "../fesk_session.h"
#include "../fesk_tx.h"
#include "unity.h"

// Mock watch functions
void watch_display_text(uint8_t position, const char *text) {
    (void)position;
    (void)text;
}

void watch_set_indicator(uint8_t indicator) {
    (void)indicator;
}

void watch_clear_indicator(uint8_t indicator) {
    (void)indicator;
}

void watch_buzzer_abort_sequence(void) {
}

void watch_set_buzzer_off(void) {
}

void watch_buzzer_play_raw_source(bool (*raw_source)(uint16_t, void*, uint16_t*, uint16_t*),
                                   void* userdata,
                                   void (*done_callback)(void)) {
    (void)raw_source;
    (void)userdata;
    (void)done_callback;
}

void watch_buzzer_play_sequence(int8_t *sequence, void (*done_callback)(void)) {
    (void)sequence;
    (void)done_callback;
}

void setUp(void) {
}

void tearDown(void) {
}

// Test that auto_base32_encode defaults to true
void test_session_auto_encode_defaults_true(void) {
    fesk_session_config_t config = fesk_session_config_defaults();
    TEST_ASSERT_TRUE(config.auto_base32_encode);
}

// Test session with emoji - should auto-encode
void test_session_auto_encode_emoji(void) {
    fesk_session_t session;
    fesk_session_config_t config = fesk_session_config_defaults();

    config.static_message = "🎵🎶";  // Musical notes emojis
    config.enable_countdown = false;

    fesk_session_init(&session, &config);
    bool started = fesk_session_start(&session);

    TEST_ASSERT_TRUE(started);
    TEST_ASSERT_NOT_NULL(session.encoded_payload);

    // Verify encoded payload only contains valid fesk characters
    const char *payload = session.encoded_payload;
    for (size_t i = 0; i < strlen(payload); i++) {
        uint8_t code;
        bool is_valid = fesk_lookup_char_code((unsigned char)payload[i], &code);
        TEST_ASSERT_TRUE_MESSAGE(is_valid, "Auto-encoded character is not valid for fesk");
    }

    fesk_session_dispose(&session);
}

// Test session with unicode characters - should auto-encode
void test_session_auto_encode_unicode(void) {
    fesk_session_t session;
    fesk_session_config_t config = fesk_session_config_defaults();

    config.static_message = "hello™ world€ 2025©";
    config.enable_countdown = false;

    fesk_session_init(&session, &config);
    bool started = fesk_session_start(&session);

    TEST_ASSERT_TRUE(started);
    TEST_ASSERT_NOT_NULL(session.encoded_payload);

    // Verify all characters are valid
    const char *payload = session.encoded_payload;
    for (size_t i = 0; i < strlen(payload); i++) {
        uint8_t code;
        bool is_valid = fesk_lookup_char_code((unsigned char)payload[i], &code);
        TEST_ASSERT_TRUE(is_valid);
    }

    fesk_session_dispose(&session);
}

// Test session with valid-only characters - should NOT auto-encode
void test_session_no_encode_for_valid_chars(void) {
    fesk_session_t session;
    fesk_session_config_t config = fesk_session_config_defaults();

    config.static_message = "hello world 123";  // All valid
    config.enable_countdown = false;

    fesk_session_init(&session, &config);
    bool started = fesk_session_start(&session);

    TEST_ASSERT_TRUE(started);
    TEST_ASSERT_NULL(session.encoded_payload);  // Should NOT be encoded

    fesk_session_dispose(&session);
}

// Global flag for error callback test
static bool g_error_called = false;
static fesk_result_t g_last_error = FESK_OK;

static void test_error_callback(fesk_result_t error, void *user_data) {
    (void)user_data;
    g_error_called = true;
    g_last_error = error;
}

// Test disabling auto-encoding - should fail with invalid chars
void test_session_auto_encode_disabled(void) {
    fesk_session_t session;
    fesk_session_config_t config = fesk_session_config_defaults();

    g_error_called = false;
    g_last_error = FESK_OK;

    config.static_message = "hello™";  // Contains invalid character
    config.auto_base32_encode = false;  // Disable auto encoding
    config.enable_countdown = false;
    config.on_error = test_error_callback;

    fesk_session_init(&session, &config);
    bool started = fesk_session_start(&session);

    TEST_ASSERT_FALSE(started);
    TEST_ASSERT_TRUE(g_error_called);
    TEST_ASSERT_EQUAL(FESK_ERR_UNSUPPORTED_CHARACTER, g_last_error);

    fesk_session_dispose(&session);
}

// Test payload callback with emojis
static fesk_result_t test_emoji_payload_callback(const char **out_text, size_t *out_length, void *user_data) {
    (void)user_data;
    *out_text = "🚀🌟";  // Rocket and star emojis
    *out_length = 0;
    return FESK_OK;
}

void test_session_callback_with_emoji(void) {
    fesk_session_t session;
    fesk_session_config_t config = fesk_session_config_defaults();

    config.provide_payload = test_emoji_payload_callback;
    config.enable_countdown = false;

    fesk_session_init(&session, &config);
    bool started = fesk_session_start(&session);

    TEST_ASSERT_TRUE(started);
    TEST_ASSERT_NOT_NULL(session.encoded_payload);

    fesk_session_dispose(&session);
}

// Test various emoji types
void test_session_multiple_emoji_types(void) {
    const char *test_messages[] = {
        "🎉🎊🎈",      // Party emojis
        "Test@#$%",   // Special chars
        "日本語",      // Japanese
        "Hello™ World©",  // Unicode symbols
    };

    for (size_t i = 0; i < sizeof(test_messages) / sizeof(test_messages[0]); i++) {
        fesk_session_t session;
        fesk_session_config_t config = fesk_session_config_defaults();

        config.static_message = test_messages[i];
        config.enable_countdown = false;

        fesk_session_init(&session, &config);
        bool started = fesk_session_start(&session);

        TEST_ASSERT_TRUE_MESSAGE(started, test_messages[i]);
        TEST_ASSERT_NOT_NULL_MESSAGE(session.encoded_payload, test_messages[i]);

        // Verify all encoded characters are valid
        const char *payload = session.encoded_payload;
        for (size_t j = 0; j < strlen(payload); j++) {
            uint8_t code;
            bool is_valid = fesk_lookup_char_code((unsigned char)payload[j], &code);
            TEST_ASSERT_TRUE(is_valid);
        }

        fesk_session_dispose(&session);
    }
}

// Test that encoded payload is cleaned up properly
void test_session_encoded_payload_cleanup(void) {
    fesk_session_t session;
    fesk_session_config_t config = fesk_session_config_defaults();

    config.static_message = "emoji: 😀";
    config.enable_countdown = false;

    fesk_session_init(&session, &config);
    fesk_session_start(&session);

    TEST_ASSERT_NOT_NULL(session.encoded_payload);

    // Dispose should free the encoded payload
    fesk_session_dispose(&session);

    // Test passes if no memory leaks (verified with valgrind separately)
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_session_auto_encode_defaults_true);
    RUN_TEST(test_session_auto_encode_emoji);
    RUN_TEST(test_session_auto_encode_unicode);
    RUN_TEST(test_session_no_encode_for_valid_chars);
    RUN_TEST(test_session_auto_encode_disabled);
    RUN_TEST(test_session_callback_with_emoji);
    RUN_TEST(test_session_multiple_emoji_types);
    RUN_TEST(test_session_encoded_payload_cleanup);

    return UNITY_END();
}
