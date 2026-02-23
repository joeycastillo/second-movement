/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 *
 * Zone Word Arrays - Chronomantic word display system for zone faces
 *
 * Each zone has 48 words across 4 levels (12 words per level)
 * Shared wildcard pool of 96 words available to all zones
 *
 * Word selection algorithm:
 * 1. Pick 2 random words from current level's 12-word array
 * 2. ~20% chance: replace one word with word from different zone
 * 3. ~10% chance: replace one word with wildcard
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef PHASE_ENGINE_ENABLED

// Zone identifiers (matches metric prefixes)
typedef enum {
    ZONE_EM = 0,  // Emergence/Balance
    ZONE_WK = 1,  // Work/Activity
    ZONE_RS = 2,  // Recovery
    ZONE_CM = 3,  // Circadian
    ZONE_COUNT = 4
} zone_id_t;

// Word level (0-3, maps to score ranges 0-25, 26-50, 51-75, 76-100)
typedef enum {
    WORD_LEVEL_0 = 0,  // 0-25
    WORD_LEVEL_1 = 1,  // 26-50
    WORD_LEVEL_2 = 2,  // 51-75
    WORD_LEVEL_3 = 3,  // 76-100
    WORD_LEVEL_COUNT = 4
} word_level_t;

// Words per level
#define WORDS_PER_LEVEL 12
#define WILDCARD_COUNT 96

// Word arrays - all words ≤8 characters for 7-segment LCD
// EM (Emergence/Balance) - 48 words
extern const char* const zone_em_words[WORD_LEVEL_COUNT][WORDS_PER_LEVEL];

// WK (Activity) - 48 words
extern const char* const zone_wk_words[WORD_LEVEL_COUNT][WORDS_PER_LEVEL];

// RS (Recovery) - 48 words
extern const char* const zone_rs_words[WORD_LEVEL_COUNT][WORDS_PER_LEVEL];

// CM (Circadian) - 48 words
extern const char* const zone_cm_words[WORD_LEVEL_COUNT][WORDS_PER_LEVEL];

// Wildcard pool - 96 words shared across all zones
extern const char* const wildcard_words[WILDCARD_COUNT];

/**
 * Get word level from metric score (0-100)
 * @param score Metric score (0-100)
 * @return Word level (0-3)
 */
word_level_t zone_words_get_level(uint8_t score);

/**
 * Select 2 random words for a zone based on current level
 * 
 * Algorithm:
 * 1. Pick 2 random words from current level's 12-word array
 * 2. ~20% chance: replace one word with word from different zone
 * 3. ~10% chance: replace one word with wildcard
 *
 * @param zone_id Zone identifier (0-3)
 * @param level Word level (0-3)
 * @param seed Random seed (e.g., from RTC timestamp)
 * @param word1_out Output: first selected word (6-char buffer minimum)
 * @param word2_out Output: second selected word (6-char buffer minimum)
 */
void zone_words_select(zone_id_t zone_id, word_level_t level, uint32_t seed,
                       char *word1_out, char *word2_out);

/**
 * Get zone letter code for display
 * @param zone_id Zone identifier (0-3)
 * @return Single character zone code ('E', 'W', 'R', 'C')
 */
char zone_words_get_letter(zone_id_t zone_id);

#endif // PHASE_ENGINE_ENABLED
