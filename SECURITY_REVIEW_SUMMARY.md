# Phase 4D Security Review - Executive Summary

**Status:** 🟡 CONDITIONAL APPROVAL  
**Critical Issues:** 3  
**High Issues:** 2  
**Medium Issues:** 1

---

## Critical Issues (MUST FIX)

### C-1: Sleep Debt Trend Calculation - Signed/Unsigned Mismatch
**All zone faces** store Sleep Debt (int8_t, range -60 to +120) in uint8_t prev_value[], causing trend calculation errors.

**Fix:** Store prev_sd as int8_t separately
```c
typedef struct {
    uint8_t view_index;
    int8_t prev_sd;         // NEW: separate storage
    uint8_t prev_value[2];  // Other metrics
} emergence_face_state_t;
```

---

### C-2: Phase Engine Cumulative Update - Logic Error
Underflow check in `phase_engine.c:128-136` is backwards, causing cumulative_phase to clamp to 0 incorrectly during first 24h.

**Fix:** Proper saturating arithmetic
```c
uint16_t new_cumulative = state->cumulative_phase - old_score;
if (state->cumulative_phase < old_score) {
    new_cumulative = 0;
}
if (new_cumulative > UINT16_MAX - score) {
    state->cumulative_phase = UINT16_MAX;
} else {
    state->cumulative_phase = new_cumulative + score;
}
```

---

### C-3: Active Hours Conversion - Missing Bounds Check
`movement.c:587-588` divides quarter_hours by 4 without validating < 96, risking hour > 23.

**Fix:** Clamp before division
```c
uint8_t active_start = (active_hours.bit.start_quarter_hours > 95) 
                      ? 23 : (active_hours.bit.start_quarter_hours / 4);
```

---

## High Issues (SHOULD FIX)

### H-1: View Index Validation
Add defensive check at start of `_update_display()`:
```c
if (state->view_index >= 3) state->view_index = 0;
```

### H-2: View Index Mapping Fragility
Use explicit enums instead of magic numbers (0, 1, 2) for maintainability.

---

## Passes ✅

- ✅ Conditional compilation (`#ifdef PHASE_ENGINE_ENABLED` everywhere)
- ✅ No memory leaks (malloc in setup only, movement framework manages lifecycle)
- ✅ Input validation in `phase_compute()` (hour, day_of_year bounds checked)
- ✅ Embedded constraints (integer-only math, no floats, no hot-path malloc)
- ✅ Memory budget (70-80 bytes added, << 32 KB limit)
- ✅ Build success (140,700 bytes, 54.9% flash)

---

## Recommendation

**MERGE AFTER FIXES:** Resolve C-1, C-2, C-3 before merging to main. H-1 and H-2 recommended but not blocking.

Full report: `PHASE_4D_SECURITY_REVIEW.md`
