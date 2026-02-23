/*
 * MIT License
 * Copyright (c) 2026 Diego Perez
 *
 * ORACLE FACE
 *
 * A daily 2-word phrase. What the moon is doing. What you bring to it.
 *
 * Word A — Moon phase (the cosmic position):
 *   8 phase pools, 4 words each. Day-of-year drifts the selection so
 *   the reading is different each day even within the same phase.
 *     New:    SEED / VOID / PULL / STILL
 *     WxC:    RISE / PATH / REACH / LEAN
 *     1Q:     BUILD / PUSH / FORGE / PRESS
 *     WxG:    SWELL / FILL / GROW / DRAW
 *     Full:   TIDE / PEAK / BLOOM / FORCE
 *     WnG:    EASE / POUR / FLOW / HOLD
 *     LQ:     TURN / FALL / DRIFT / SHED
 *     WnC:    THIN / FADE / HUSH / WANE
 *
 * Word B — Circadian score tier (personal, actionable energy):
 *   5 tiers, 4 words each. Day-of-year drift here too.
 *     0-20 depleted:  REST / HOLD / STILL / WAIT
 *     21-40 low:      TEND / SLOW / KEEP / SOFT
 *     41-60 average:  MOVE / SEEK / WORK / TEND
 *     61-80 good:     DRIVE / SHAPE / BUILD / PUSH
 *     81-100 sharp:   FORGE / SURGE / BOLD / LEAD
 *
 * Birthday (compile-time):
 *   Set ORACLE_BIRTH_MONTH and ORACLE_BIRTH_DAY in movement_config.h.
 *   On your birthday the face opens on a BDAY view before the phrase.
 *
 * Display (ALARM cycles):
 *   [birthday] BDAY → Word A → Word B → Info (moon + score) → Stats (streak)
 *   [normal]   Word A → Word B → Info → Stats
 */

#pragma once

#include "movement.h"

typedef enum {
    ORACLE_VIEW_BDAY   = 0,   // Birthday message (birthday only)
    ORACLE_VIEW_WORD_A,       // Moon phase word
    ORACLE_VIEW_WORD_B,       // Circadian energy word
    ORACLE_VIEW_INFO,         // Moon name + CS score
    ORACLE_VIEW_STATS,        // Streak counter
    ORACLE_VIEW_COUNT
} oracle_view_t;

typedef enum {
    ORACLE_MODE_BOTH   = 0,   // Full reading: Word A then Word B
    ORACLE_MODE_A_ONLY = 1,   // Only Word A — no personal frame
    ORACLE_MODE_B_ONLY = 2,   // Only Word B — no cosmic frame
} oracle_mode_t;

typedef struct {
    oracle_view_t view;        // Current display view
    oracle_mode_t mode;        // Today's reading mode (set daily)
    uint8_t moon_phase;        // 0-7 (0=new, 4=full)
    uint8_t circadian_score;   // 0-100
    uint8_t word_a_idx;        // Index into words_a[]
    uint8_t word_b_idx;        // Index into words_b[]
    uint16_t day_of_year;      // 1-365, full range (daily drift)
    bool is_birthday_today;    // True on configured birthday
    bool needs_update;         // Recompute on next activate
} oracle_face_state_t;

void oracle_face_setup(uint8_t watch_face_index, void **context_ptr);
void oracle_face_activate(void *context);
bool oracle_face_loop(movement_event_t event, void *context);
void oracle_face_resign(void *context);

#define oracle_face ((const watch_face_t){ \
    oracle_face_setup, \
    oracle_face_activate, \
    oracle_face_loop, \
    oracle_face_resign, \
    NULL \
})
