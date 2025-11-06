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
#include <string.h>
#include "../fesk_tx.h"
#include "unity.h"

void setUp(void) {
}

void tearDown(void) {
}

// Test basic encoding
void test_encode_simple_text() {
    int8_t *sequence = NULL;
    size_t entries = 0;

    fesk_result_t result = fesk_encode("a", &sequence, &entries);

    TEST_ASSERT_EQUAL(FESK_OK, result);
    TEST_ASSERT_NOT_NULL(sequence);
    TEST_ASSERT_GREATER_THAN(0, entries);

    // Clean up
    fesk_free_sequence(sequence);
}

// Test encoding with mixed case (should be case-insensitive)
void test_encode_case_insensitive() {
    int8_t *seq_lower = NULL;
    int8_t *seq_upper = NULL;
    size_t entries_lower = 0;
    size_t entries_upper = 0;

    fesk_result_t result1 = fesk_encode("hello", &seq_lower, &entries_lower);
    fesk_result_t result2 = fesk_encode("HELLO", &seq_upper, &entries_upper);

    TEST_ASSERT_EQUAL(FESK_OK, result1);
    TEST_ASSERT_EQUAL(FESK_OK, result2);
    TEST_ASSERT_EQUAL(entries_lower, entries_upper);

    // Sequences should be identical (case-insensitive)
    TEST_ASSERT_EQUAL_INT8_ARRAY(seq_lower, seq_upper, entries_lower);

    fesk_free_sequence(seq_lower);
    fesk_free_sequence(seq_upper);
}

// Test all supported characters
void test_encode_all_characters() {
    int8_t *sequence = NULL;
    size_t entries = 0;

    const char *test_str = "abcdefghijklmnopqrstuvwxyz0123456789 ,:'\"\n";
    fesk_result_t result = fesk_encode(test_str, &sequence, &entries);

    TEST_ASSERT_EQUAL(FESK_OK, result);
    TEST_ASSERT_NOT_NULL(sequence);

    fesk_free_sequence(sequence);
}

// Test unsupported character
void test_encode_unsupported_character() {
    int8_t *sequence = NULL;
    size_t entries = 0;

    // '@' is not supported
    fesk_result_t result = fesk_encode("hello@world", &sequence, &entries);

    TEST_ASSERT_EQUAL(FESK_ERR_UNSUPPORTED_CHARACTER, result);
    TEST_ASSERT_NULL(sequence);
}

// Test NULL pointer handling
void test_encode_null_pointer() {
    int8_t *sequence = NULL;
    size_t entries = 0;

    fesk_result_t result = fesk_encode(NULL, &sequence, &entries);
    TEST_ASSERT_EQUAL(FESK_ERR_INVALID_ARGUMENT, result);

    result = fesk_encode("hello", NULL, &entries);
    TEST_ASSERT_EQUAL(FESK_ERR_INVALID_ARGUMENT, result);
}

// Test empty string
void test_encode_empty_string() {
    int8_t *sequence = NULL;
    size_t entries = 0;

    fesk_result_t result = fesk_encode("", &sequence, &entries);
    TEST_ASSERT_EQUAL(FESK_ERR_INVALID_ARGUMENT, result);
}

// Test sequence structure (should have start/end markers)
void test_sequence_structure() {
    int8_t *sequence = NULL;
    size_t entries = 0;

    fesk_result_t result = fesk_encode("a", &sequence, &entries);

    TEST_ASSERT_EQUAL(FESK_OK, result);
    TEST_ASSERT_NOT_NULL(sequence);

    // Sequence should be null-terminated
    TEST_ASSERT_EQUAL(0, sequence[entries]);

    // Each bit is encoded as: [TONE, TICKS, REST, TICKS]
    // So entries should be a multiple of 4
    // Format: START(6bit) + 'a'(6bit) + CRC(8bit) + END(6bit) = 26 bits
    // 26 bits * 4 entries/bit = 104 entries
    TEST_ASSERT_EQUAL(104, entries);

    fesk_free_sequence(sequence);
}

// Test digits encoding
void test_encode_digits() {
    int8_t *sequence = NULL;
    size_t entries = 0;

    fesk_result_t result = fesk_encode("0123456789", &sequence, &entries);

    TEST_ASSERT_EQUAL(FESK_OK, result);
    TEST_ASSERT_NOT_NULL(sequence);

    fesk_free_sequence(sequence);
}

// Test punctuation
void test_encode_punctuation() {
    int8_t *sequence = NULL;
    size_t entries = 0;

    const char *test_str = "hello, world: 'test' \"quote\"";
    fesk_result_t result = fesk_encode(test_str, &sequence, &entries);

    TEST_ASSERT_EQUAL(FESK_OK, result);
    TEST_ASSERT_NOT_NULL(sequence);

    fesk_free_sequence(sequence);
}

// Test newline character
void test_encode_newline() {
    int8_t *sequence = NULL;
    size_t entries = 0;

    const char *test_str = "line1\nline2";
    fesk_result_t result = fesk_encode(test_str, &sequence, &entries);

    TEST_ASSERT_EQUAL(FESK_OK, result);
    TEST_ASSERT_NOT_NULL(sequence);

    fesk_free_sequence(sequence);
}

// Test maximum length string
void test_encode_max_length() {
    int8_t *sequence = NULL;
    size_t entries = 0;

    // Create a 1024 character string (max allowed)
    char long_str[1025];
    memset(long_str, 'a', 1024);
    long_str[1024] = '\0';

    fesk_result_t result = fesk_encode(long_str, &sequence, &entries);

    TEST_ASSERT_EQUAL(FESK_OK, result);
    TEST_ASSERT_NOT_NULL(sequence);

    fesk_free_sequence(sequence);
}

// Test over maximum length string (should fail)
void test_encode_over_max_length() {
    int8_t *sequence = NULL;
    size_t entries = 0;

    // Create a 1025 character string (over max)
    char long_str[1026];
    memset(long_str, 'a', 1025);
    long_str[1025] = '\0';

    fesk_result_t result = fesk_encode(long_str, &sequence, &entries);

    TEST_ASSERT_EQUAL(FESK_ERR_INVALID_ARGUMENT, result);
    TEST_ASSERT_NULL(sequence);
}

// Test tone mapping
void test_tone_mapping() {
    // Verify tone map is correctly defined
    TEST_ASSERT_EQUAL(FESK_TONE_LOW_NOTE, fesk_tone_map[FESK_TONE_ZERO]);
    TEST_ASSERT_EQUAL(FESK_TONE_HIGH_NOTE, fesk_tone_map[FESK_TONE_ONE]);
}

// Test free with NULL (should be safe)
void test_free_null_sequence() {
    fesk_free_sequence(NULL);  // Should not crash
    TEST_PASS();
}

// Test encoding produces different sequences for different inputs
void test_different_inputs_different_sequences() {
    int8_t *seq1 = NULL;
    int8_t *seq2 = NULL;
    size_t entries1 = 0;
    size_t entries2 = 0;

    fesk_encode("abc", &seq1, &entries1);
    fesk_encode("xyz", &seq2, &entries2);

    TEST_ASSERT_EQUAL(entries1, entries2);  // Same length

    // But sequences should differ (comparing some middle portion to avoid start/end markers)
    int differences = 0;
    for (size_t i = 20; i < entries1 - 20; i++) {
        if (seq1[i] != seq2[i]) {
            differences++;
        }
    }
    TEST_ASSERT_GREATER_THAN(0, differences);

    fesk_free_sequence(seq1);
    fesk_free_sequence(seq2);
}

int main(void) {
    UNITY_BEGIN();

    // Basic functionality
    RUN_TEST(test_encode_simple_text);
    RUN_TEST(test_encode_case_insensitive);
    RUN_TEST(test_encode_all_characters);

    // Character support
    RUN_TEST(test_encode_digits);
    RUN_TEST(test_encode_punctuation);
    RUN_TEST(test_encode_newline);

    // Error cases
    RUN_TEST(test_encode_unsupported_character);
    RUN_TEST(test_encode_null_pointer);
    RUN_TEST(test_encode_empty_string);

    // Edge cases
    RUN_TEST(test_encode_max_length);
    RUN_TEST(test_encode_over_max_length);
    RUN_TEST(test_free_null_sequence);

    // Structure and correctness
    RUN_TEST(test_sequence_structure);
    RUN_TEST(test_tone_mapping);
    RUN_TEST(test_different_inputs_different_sequences);

    return UNITY_END();
}
