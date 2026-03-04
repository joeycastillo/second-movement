/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 *
 * Zone Word Arrays - Implementation
 */

#include "zone_words.h"
#include <string.h>

#ifdef PHASE_ENGINE_ENABLED

// EM (Emergence/Balance) - 48 words
const char* const zone_em_words[WORD_LEVEL_COUNT][WORDS_PER_LEVEL] = {
    // Level 0 (0-25)
    {"SLEEP", "REST", "HUSH", "STILL", "VOID", "DARK", "NULL", "FADE", "EMPTY", "QUIET", "NUMB", "LOST"},
    // Level 1 (26-50)
    {"ALIGN", "TUNE", "PULSE", "MESH", "WEAVE", "LINK", "TIDE", "BREATH", "SWAY", "ORBIT", "DIAL", "TETHER"},
    // Level 2 (51-75)
    {"SURGE", "GLIDE", "SPARK", "POUR", "BLOOM", "WIND", "WAVE", "RUSH", "SOAR", "DANCE", "MELT", "BURN"},
    // Level 3 (76-100)
    {"PEAK", "BLAZE", "APEX", "CROWN", "FULL", "NOON", "ZENITH", "CORE", "FORGE", "TOTAL", "FUSE", "MERGE"}
};

// WK (Activity) - 48 words
const char* const zone_wk_words[WORD_LEVEL_COUNT][WORDS_PER_LEVEL] = {
    // Level 0 (0-25)
    {"IDLE", "PAUSE", "WAIT", "STILL", "SETTLE", "STOP", "DRIFT", "HUSH", "STALL", "HOLD", "SLACK", "REPOSE"},
    // Level 1 (26-50)
    {"WALK", "SHIFT", "SCAN", "TEND", "STEP", "REACH", "LEAN", "PROWL", "ROAM", "CREEP", "PACE", "CREST"},
    // Level 2 (51-75)
    {"BUILD", "CRAFT", "DIG", "PRESS", "PUSH", "CLIMB", "GRIND", "HAUL", "SHAPE", "FORGE", "HEAVE", "DRIVE"},
    // Level 3 (76-100)
    {"SPRINT", "BLAZE", "SURGE", "HUNT", "FIGHT", "CHARGE", "SMASH", "STRIKE", "BURST", "LAUNCH", "CLASH", "WRENCH"}
};

// RS (Recovery) - 48 words
const char* const zone_rs_words[WORD_LEVEL_COUNT][WORDS_PER_LEVEL] = {
    // Level 0 (0-25)
    {"ALERT", "WIRED", "JOLT", "BUZZ", "SHOCK", "SPIKE", "FLASH", "SNAP", "STIR", "CRACK", "SURGE", "SPARK"},
    // Level 1 (26-50)
    {"EASE", "SOFT", "SMOOTH", "STILL", "LEVEL", "STEADY", "COOL", "SETTLE", "MILD", "STABLE", "SAFE", "CLEAR"},
    // Level 2 (51-75)
    {"SINK", "DREAM", "FLOAT", "DRIFT", "FALL", "DIVE", "MELT", "FADE", "DISSOLVE", "PEACE", "HUSH", "BLANK"},
    // Level 3 (76-100)
    {"STONE", "VOID", "NUMB", "BLACK", "DEAD", "GONE", "NULL", "ZERO", "HOLLOW", "LOST", "DARK", "FROZEN"}
};

// CM (Circadian) - 48 words
const char* const zone_cm_words[WORD_LEVEL_COUNT][WORDS_PER_LEVEL] = {
    // Level 0 (0-25)
    {"DRIFT", "LOST", "OFF", "SPLIT", "BROKE", "FRACTURE", "TWISTED", "SKEWED", "BACKWARD", "INVERTED", "REVERSED", "FLIPPED"},
    // Level 1 (26-50)
    {"DIAL", "TUNE", "SHIFT", "ADJUST", "CALIBRAT", "SYNC", "MATCH", "SETTLE", "CONVERGE", "APPROACH", "NEAR", "CLOSE"},
    // Level 2 (51-75)
    {"FOLLOW", "PULSE", "RHYTHM", "CYCLE", "BEAT", "PATTERN", "WAVE", "TIDE", "ORBIT", "CIRCLE", "LOOP", "FLOW"},
    // Level 3 (76-100)
    {"PEAK", "ZENITH", "NOON", "MIDNIGHT", "LOCKED", "FIXED", "TOTAL", "PERFECT", "ABSOLUTE", "COMPLETE", "FULL", "PURE"}
};

// Wildcard pool - 96 words (shared across all zones)
const char* const wildcard_words[WILDCARD_COUNT] = {
    "GHOST", "ECHO", "SHADE", "WRAITH", "SPECTR", "PHANTOM", "ETHER", "VEIL",
    "BONE", "ASH", "MOSS", "RUST", "DUST", "STONE", "THORN", "ROOT", "SHELL", "SHARD", "CRYSTAL", "GRAVEL",
    "COIL", "SPIRAL", "HELIX", "KNOT", "THREAD", "WIRE", "MESH", "GRID", "LATTICE", "WEB", "NET", "WEAVE",
    "SPARK", "EMBER", "FLAME", "SMOKE", "VAPOR", "MIST", "FOG", "DEW", "FROST", "ICE", "SNOW", "STORM", "WIND", "GALE", "RAIN",
    "CALM", "STILL", "HUSH", "BREATH", "SIGH", "WHISPER", "MURMUR", "HUM", "BUZZ", "TOLL", "CHIME", "ECHO",
    "VOID", "NULL", "ZERO", "ONE", "PRIME", "CHAOS", "ORDER", "FLUX", "SHIFT", "DELTA", "PHASE", "STATE", "QUANTUM", "WAVE", "PARTICLE",
    "FIELD", "FORCE", "ENERGY", "MASS", "TIME", "SPACE", "VECTOR", "SURGE", "PULL", "PUSH",
    "CYCLE", "EPOCH", "ERA", "AGE", "MOMENT", "INSTANT", "PULSE", "TICK", "TURN", "ROUND",
    "FRAGMENT", "SPLINTER", "RUNE", "GLYPH", "SIGIL", "MARK", "SIGN", "SYMBOL", "TOKEN", "CIPHER", "CODE", "BREAK",
    "SEED", "BLOOM", "WILT", "DECAY", "REBIRTH", "VINE", "CELL", "ORGANISM",
    "STAR", "MOON", "SUN", "ORBIT", "APOGEE", "PERIGEE", "ECLIPSE", "SOLSTICE",
    "AETHER", "WYRD", "KAIROS", "ANIMA", "OSMOSIS", "NOVAE", "OMEN", "ROTA"
};

// Simple LCG (Linear Congruential Generator) for deterministic randomness
static uint32_t _prng_state = 0;

static void _prng_seed(uint32_t seed) {
    _prng_state = seed;
}

static uint32_t _prng_next(void) {
    // LCG parameters from Numerical Recipes
    _prng_state = (1103515245u * _prng_state + 12345u) & 0x7FFFFFFFu;
    return _prng_state;
}

static uint8_t _prng_range(uint8_t max) {
    return (uint8_t)(_prng_next() % max);
}

word_level_t zone_words_get_level(uint8_t score) {
    if (score <= 25) return WORD_LEVEL_0;
    if (score <= 50) return WORD_LEVEL_1;
    if (score <= 75) return WORD_LEVEL_2;
    return WORD_LEVEL_3;
}

void zone_words_select(zone_id_t zone_id, word_level_t level, uint32_t seed,
                       char *word1_out, char *word2_out) {
    // Seed PRNG
    _prng_seed(seed);
    
    // Get the appropriate word array for this zone
    const char* const (*zone_array)[WORDS_PER_LEVEL] = NULL;
    switch (zone_id) {
        case ZONE_EM: zone_array = zone_em_words; break;
        case ZONE_WK: zone_array = zone_wk_words; break;
        case ZONE_RS: zone_array = zone_rs_words; break;
        case ZONE_CM: zone_array = zone_cm_words; break;
        default: zone_array = zone_em_words; break;
    }
    
    // Pick 2 random words from current level
    uint8_t idx1 = _prng_range(WORDS_PER_LEVEL);
    uint8_t idx2 = _prng_range(WORDS_PER_LEVEL);
    // Ensure different words
    while (idx2 == idx1) {
        idx2 = _prng_range(WORDS_PER_LEVEL);
    }
    
    const char *selected1 = zone_array[level][idx1];
    const char *selected2 = zone_array[level][idx2];
    
    // ~20% chance: replace one word with word from different zone
    if (_prng_range(100) < 20) {
        // Pick a different zone
        zone_id_t other_zone = (zone_id_t)_prng_range(ZONE_COUNT);
        while (other_zone == zone_id) {
            other_zone = (zone_id_t)_prng_range(ZONE_COUNT);
        }
        
        const char* const (*other_array)[WORDS_PER_LEVEL] = NULL;
        switch (other_zone) {
            case ZONE_EM: other_array = zone_em_words; break;
            case ZONE_WK: other_array = zone_wk_words; break;
            case ZONE_RS: other_array = zone_rs_words; break;
            case ZONE_CM: other_array = zone_cm_words; break;
            default: other_array = zone_em_words; break;
        }
        
        uint8_t other_idx = _prng_range(WORDS_PER_LEVEL);
        // Replace word 1 or 2 randomly
        if (_prng_range(2) == 0) {
            selected1 = other_array[level][other_idx];
        } else {
            selected2 = other_array[level][other_idx];
        }
    }
    
    // ~10% chance: replace one word with wildcard
    if (_prng_range(100) < 10) {
        uint8_t wildcard_idx = _prng_range(WILDCARD_COUNT);
        // Replace word 1 or 2 randomly
        if (_prng_range(2) == 0) {
            selected1 = wildcard_words[wildcard_idx];
        } else {
            selected2 = wildcard_words[wildcard_idx];
        }
    }
    
    // Copy to output buffers (8 chars + null terminator = 9 bytes max)
    strncpy(word1_out, selected1, 8);
    word1_out[8] = '\0';
    strncpy(word2_out, selected2, 8);
    word2_out[8] = '\0';
}

char zone_words_get_letter(zone_id_t zone_id) {
    switch (zone_id) {
        case ZONE_EM: return 'E';
        case ZONE_WK: return 'W';
        case ZONE_RS: return 'R';
        case ZONE_CM: return 'C';
        default: return '?';
    }
}

#endif // PHASE_ENGINE_ENABLED
