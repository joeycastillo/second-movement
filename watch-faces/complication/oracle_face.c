/*
 * MIT License
 * Copyright (c) 2026 Diego Perez
 *
 * Oracle Face — daily 2-word phrase
 * Word A: moon phase (what the cosmos is doing)
 * Word B: phase zone (what you bring to it)
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "oracle_face.h"
#include "watch_utility.h"
#include "circadian_score.h"

#ifdef PHASE_ENGINE_ENABLED
#include "movement.h"
#endif

// ─────────────────────────────────────────────────────────────────
// Word A: 64-word flat list, ordered by lunar archetype
// Moon phase seeds the starting chapter (8 words per chapter).
// index = (moon_phase * 8 + day_of_year) % 64
// → different word every single day, archetype-biased chapter
// → two same-moon-phase visits are 29.5 days apart, so index advances
//   by 29.5%64 ≈ 30 positions between visits → different words each time
// ─────────────────────────────────────────────────────────────────
static const char *words_a[64] = {
    // Chapter 0: New moon — void, seed, quiet inward pull
    // NULL: absence as data. GAP: the space before the signal.
    "VOID", "BROOD", "HUSH", "NULL", "SEED", "DEEP", "GAP", "STILL",
    // Chapter 1: Waxing crescent — first stir, seeking
    // BLIP: first ping on the sensor. the emergence before the pattern.
    "STIR", "LEAN", "SEEK", "REACH", "RISE", "TEND", "SPARK", "BLIP",
    // Chapter 2: First quarter — momentum, cutting, building
    // ETCH: building leaves marks — annotation texture, more precise than CUT
    "BUILD", "FORGE", "PRESS", "SHAPE", "DRIVE", "ETCH", "SNAP", "CLIMB",
    // Chapter 3: Waxing gibbous — swelling, heavy, near the peak
    // ACHE: the body knows something's building before the mind does
    // SCAR: growth that transforms the landscape. intensity with consequence.
    "SWELL", "FILL", "CREST", "GROW", "PULL", "HEAVY", "SCAR", "ACHE",
    // Chapter 4: Full moon — peak, flood, nothing hidden
    // GLOW: phosphorescent, atmospheric — the full moon doesn't just shine
    "TIDE", "PEAK", "FLOOD", "LUCK", "SURGE", "BURN", "GLOW", "BLAZE",
    // Chapter 5: Waning gibbous — after the peak, giving back
    // DONE: the peak is over. definitive. sometimes funny.
    // SEEP: entropic dispersal — slower, more insidious than pour
    "EASE", "SEEP", "SPILL", "FLOW", "GIVE", "DONE", "YIELD", "SHED",
    // Chapter 6: Last quarter — the turn, releasing what's done
    // BLUR: signal degradation as dissolution
    "TURN", "FALL", "DRIFT", "PASS", "BREAK", "SPIN", "BLUR", "SHIFT",
    // Chapter 7: Waning crescent — thinning, the final dark
    // ECHO: what resonates after the source is gone. ghost signal.
    // HUSK: the shell after the seed — organic, specific, not just empty
    "THIN", "FADE", "ECHO", "WANE", "BARE", "LIMB", "DARK", "HUSK",
};

// ─────────────────────────────────────────────────────────────────
// Word B: 55-word flat list, ordered by phase zone archetype
// Phase zone seeds the chapter offset (4 zones: 14, 14, 14, 13 words).
// index = zone_offset + (day_of_year * 7) % words_per_zone
// → day*7 with gcd(7,14)=7 and gcd(7,13)=1 → full cycling within zones
// → LCM(64 days, 55 days) = 3,520 days ≈ 9.6 years before same phrase
// Words are archetypal actions: the texture of your capacity today
// ─────────────────────────────────────────────────────────────────
static const char *words_b[55] = {
    // Zone 0 (0-13): Emergence (0-25) — waking, orienting, gentle awakening
    // HAZE: the world before it sharpens. HUM: low frequency presence.
    // WAKE/STIR: the first movements toward consciousness.
    "SLEEP", "REST", "WAKE", "STIR", "DRIFT", "TEND", "SOFT", "CALM", "EASE", "RISE", "LIGHT", "HUM", "STILL", "QUIET",
    // Zone 1 (14-27): Momentum (26-50) — building energy, steady work
    // SCAN: field observer active. DIG/GRIND: committed, earthy effort.
    // WORK/BUILD: the fundamental cadence of getting things done.
    "SCAN", "LEAN", "MOVE", "SEEK", "STEP", "PACE", "WORK", "PRESS", "BUILD", "CLIMB", "REACH", "RISK", "DIG", "GRIND",
    // Zone 2 (28-41): Active (51-75) — peak capacity, high energy
    // GO/SURGE: immediate action. LOCK: target acquired. BLAZE/FLY: peak flow.
    // AXE: declarative cut-through — sharp personality at peak energy.
    "PUSH", "DRIVE", "FORGE", "CRAFT", "SHAPE", "GO", "SURGE", "SPARK", "HUNT", "BURN", "LEAP", "LOCK", "BLAZE", "FLY",
    // Zone 3 (42-54): Descent (76-100) — winding down, settling, returning
    // EASE/YIELD: intentional release. SETTLE/CLOSE: completion, not failure.
    // HUSK: the shell that remains — organic, specific, transformation complete.
    "EASE", "YIELD", "SETTLE", "CLOSE", "DIM", "FADE", "RETURN", "GIVE", "DONE", "TURN", "PASS", "SHIFT", "HUSK",
};

// ─────────────────────────────────────────────────────────────────
// Moon phase calculation — fixed-point, no floats, fits 32-bit math
// Known new moon: 2000-01-06 18:14 UTC ≈ JDN 2451550
// Synodic month: 29.53059 days → scaled ×100 = 2953
// Returns 0-7 (0=new, 2=first quarter, 4=full, 6=last quarter)
// ─────────────────────────────────────────────────────────────────
static uint8_t _moon_phase(int year, int month, int day) {
    if (month < 3) { year--; month += 12; }
    // Julian Day Number (integer approximation)
    long jdn = 365L * year + year / 4 - year / 100 + year / 400
               + (306 * (month + 1)) / 10 + day - 428;
    // Days since known new moon (JDN 2451550), ×100 for fixed-point precision
    long days_x100 = (jdn - 2451550L) * 100L;
    // Position within synodic month (scaled ×100), handle negative (dates before 2000)
    long cycle_x100 = days_x100 % 2953L;
    if (cycle_x100 < 0) cycle_x100 += 2953L;
    // Map 0-2952 → 0-7
    // Note: (2952 * 8) / 2953 = 8 via integer division — clamp to prevent
    // word_a_idx = moon_phase * 8 + inner from exceeding words_a[63]
    uint8_t phase = (uint8_t)((cycle_x100 * 8L) / 2953L);
    return (phase > 7) ? 7 : phase;
}

// ─────────────────────────────────────────────────────────────────
// Moon phase short name (4 chars + null)
// ─────────────────────────────────────────────────────────────────
static const char *_moon_name(uint8_t phase) {
    static const char *names[] = {
        "NEW ", "WXC ", "FQ  ", "WXG ",
        "FULL", "WNG ", "LQ  ", "WNC "
    };
    return names[phase < 8 ? phase : 0];
}

// ─────────────────────────────────────────────────────────────────
// Compute phrase from current inputs
// ─────────────────────────────────────────────────────────────────
// year_val: watch RTC year field (0-63, relative to WATCH_RTC_REFERENCE_YEAR)
static void _compute_oracle(oracle_face_state_t *state, uint8_t year_val) {
    // Word A: moon phase chapter (8 words), doy + year-salt cycles daily.
    // year_val % 8 shifts starting word each year — no same-date annual repeat.
    uint8_t inner_a = (state->day_of_year + year_val % 8) % 8;
    state->word_a_idx = state->moon_phase * 8 + inner_a;

    // Word B: phase zone (4 zones with 14, 14, 14, 13 words)
    // Get phase score (0-100) from Phase Engine or fallback to circadian midpoint
#ifdef PHASE_ENGINE_ENABLED
    extern movement_state_t movement_state;
    uint16_t phase_score = movement_state.phase.last_phase_score;
#else
    // Fallback: use circadian score as phase proxy when Phase Engine disabled
    uint16_t phase_score = state->circadian_score;
#endif

    // Map phase score to zone (0-3)
    // Emergence (0-25), Momentum (26-50), Active (51-75), Descent (76-100)
    uint8_t zone;
    if      (phase_score <= 25) zone = 0;  // Emergence
    else if (phase_score <= 50) zone = 1;  // Momentum
    else if (phase_score <= 75) zone = 2;  // Active
    else                        zone = 3;  // Descent

    // Calculate zone offset and words per zone
    static const uint8_t words_per_zone[4] = {14, 14, 14, 13};
    uint8_t zone_offset = (zone == 0) ? 0 :
                         (zone == 1) ? 14 :
                         (zone == 2) ? 28 : 42;
    
    // Index within zone: day_of_year * 7 cycles through zone words
    // gcd(7,14)=7 → visits 2 words in 14-word zones before repeating
    // gcd(7,13)=1 → visits all 13 words in Descent zone
    uint8_t inner_b = (state->day_of_year * 7) % words_per_zone[zone];
    state->word_b_idx = zone_offset + inner_b;

    // Reading mode: 85% full reading, ~7.5% A only, ~7.5% B only
    // % 13: 0=A_only(7.7%), 1=B_only(7.7%), 2-12=BOTH(84.6%)
    uint8_t mode_seed = (year_val * 7 + state->day_of_year * 3 + state->moon_phase) % 13;
    if      (mode_seed == 0) state->mode = ORACLE_MODE_A_ONLY;
    else if (mode_seed == 1) state->mode = ORACLE_MODE_B_ONLY;
    else                     state->mode = ORACLE_MODE_BOTH;
}

// ─────────────────────────────────────────────────────────────────
// Full refresh — load inputs, compute
// ─────────────────────────────────────────────────────────────────
static void _refresh_oracle(oracle_face_state_t *state) {
    watch_date_time_t now = movement_get_local_date_time();
    int year  = now.unit.year + WATCH_RTC_REFERENCE_YEAR;
    int month = now.unit.month;
    int day   = now.unit.day;

    state->moon_phase  = _moon_phase(year, month, day);
    state->day_of_year = (uint16_t)watch_utility_days_since_new_year(year, month, day);

    circadian_data_t circ = {0};
    circadian_data_load_from_flash(&circ);
    state->circadian_score = circadian_score_calculate(&circ);

    // Birthday check (compile-time defines, zero overhead)
#if defined(ORACLE_BIRTH_MONTH) && defined(ORACLE_BIRTH_DAY)
    state->is_birthday_today = (month == ORACLE_BIRTH_MONTH && day == ORACLE_BIRTH_DAY);
#else
    state->is_birthday_today = false;
#endif

    _compute_oracle(state, (uint8_t)now.unit.year);
    state->needs_update = false;
}

// ─────────────────────────────────────────────────────────────────
// Display update
// ─────────────────────────────────────────────────────────────────
static void _update_display(oracle_face_state_t *state) {
    char buf[16];

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "OR", "Oracle");

    // Birthday view — bell on, show BDAY message
    if (state->is_birthday_today && state->view == ORACLE_VIEW_BDAY) {
        watch_display_text(WATCH_POSITION_BOTTOM, " BDAY");
        watch_set_indicator(WATCH_INDICATOR_BELL);
        return;
    }

    // Not birthday or past BDAY view — clear bell
    watch_clear_indicator(WATCH_INDICATOR_BELL);

    // Effective view (skip BDAY on non-birthday)
    oracle_view_t v = state->view;
    if (!state->is_birthday_today && v == ORACLE_VIEW_BDAY) {
        v = ORACLE_VIEW_WORD_A;
    }

    switch (v) {
        case ORACLE_VIEW_WORD_A:
            watch_display_text(WATCH_POSITION_BOTTOM, words_a[state->word_a_idx]);
            break;

        case ORACLE_VIEW_WORD_B:
            watch_display_text(WATCH_POSITION_BOTTOM, words_b[state->word_b_idx]);
            break;

        case ORACLE_VIEW_INFO:
            snprintf(buf, sizeof(buf), "%s%3d", _moon_name(state->moon_phase), state->circadian_score);
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
            break;

        case ORACLE_VIEW_STATS: {
#ifdef PHASE_ENGINE_ENABLED
            extern movement_state_t movement_state;
            uint16_t streak = movement_state.phase.zone_check_streak;
            snprintf(buf, sizeof(buf), "Str %d", streak);
#else
            snprintf(buf, sizeof(buf), "Str  0");
#endif
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
            break;
        }

        default:
            break;
    }
}

// ─────────────────────────────────────────────────────────────────
// Face lifecycle
// ─────────────────────────────────────────────────────────────────
void oracle_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(oracle_face_state_t));
        if (*context_ptr == NULL) return;
        memset(*context_ptr, 0, sizeof(oracle_face_state_t));
        ((oracle_face_state_t *)*context_ptr)->needs_update = true;
    }
}

void oracle_face_activate(void *context) {
    oracle_face_state_t *state = (oracle_face_state_t *)context;

    // Recompute once per day
    watch_date_time_t now = movement_get_local_date_time();
    int year = now.unit.year + WATCH_RTC_REFERENCE_YEAR;
    uint16_t today = (uint16_t)watch_utility_days_since_new_year(year, now.unit.month, now.unit.day);

    if (state->needs_update || today != state->day_of_year) {
        _refresh_oracle(state);
    }

    // Open on BDAY if birthday, else first word based on today's reading mode
    if (state->is_birthday_today)       state->view = ORACLE_VIEW_BDAY;
    else if (state->mode == ORACLE_MODE_B_ONLY) state->view = ORACLE_VIEW_WORD_B;
    else                                state->view = ORACLE_VIEW_WORD_A;
    _update_display(state);
}

bool oracle_face_loop(movement_event_t event, void *context) {
    oracle_face_state_t *state = (oracle_face_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            _update_display(state);
            break;

        case EVENT_ALARM_BUTTON_UP: {
            // Cycle respects today's reading mode:
            //   BOTH:   BDAY → Word A → Word B → Info → loop
            //   A only: BDAY → Word A → Info → loop
            //   B only: BDAY → Word B → Info → loop
            oracle_view_t next;
            switch (state->view) {
                case ORACLE_VIEW_BDAY:
                    // After birthday message, go to first oracle word
                    next = (state->mode == ORACLE_MODE_B_ONLY)
                           ? ORACLE_VIEW_WORD_B : ORACLE_VIEW_WORD_A;
                    break;
                case ORACLE_VIEW_WORD_A:
                    // A-only and BOTH diverge here
                    next = (state->mode == ORACLE_MODE_BOTH)
                           ? ORACLE_VIEW_WORD_B : ORACLE_VIEW_INFO;
                    break;
                case ORACLE_VIEW_WORD_B:
                    next = ORACLE_VIEW_INFO;
                    break;
                case ORACLE_VIEW_INFO:
                    // Loop back to first view
                    if (state->is_birthday_today)
                        next = ORACLE_VIEW_BDAY;
                    else if (state->mode == ORACLE_MODE_B_ONLY)
                        next = ORACLE_VIEW_WORD_B;
                    else
                        next = ORACLE_VIEW_WORD_A;
                    break;
                default:
                    next = ORACLE_VIEW_WORD_A;
                    break;
            }
            state->view = next;
            _update_display(state);
            break;
        }

        case EVENT_ALARM_LONG_PRESS:
            // Force refresh (after sleep score updates in the morning)
            _refresh_oracle(state);
            state->view = ORACLE_VIEW_WORD_A;
            _update_display(state);
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            movement_illuminate_led();
            break;

        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void oracle_face_resign(void *context) {
    (void)context;
}
