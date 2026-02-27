# Security Review: Phase 3B PR #61 - Weighted Playlist Controller

**Reviewer:** Security Specialist Agent  
**Date:** 2026-02-20  
**Branch:** phase3-pr3-playlist-controller  
**Commit:** ceb0d6f (HEAD)

## Executive Summary

**APPROVED WITH MINOR RECOMMENDATIONS**

The weighted playlist controller implementation demonstrates solid defensive programming practices. The critical integer underflow vulnerability in the bubble sort was proactively fixed in commit 81bcc9b. All major security concerns have been adequately addressed.

**Findings:**
- ✅ **0 Critical Issues** (blocking)
- ✅ **0 High Issues** (should fix)
- ⚠️ **2 Medium Issues** (recommendations)
- ℹ️ **3 Low Issues** (nice-to-have)

---

## 1. Array Bounds Checking in Weight Tables

### Status: ✅ PASS

**Analysis:**
- Zone weight table: `zone_weights[4][5]` (4 zones × 5 metrics)
- All accesses use `zone` (0-3) and metric index (0-4)

**Access patterns verified:**
```c
// rebuild_rotation() - line 77-81
relevances[0] = compute_relevance(zone_weights[zone][0], metrics->sd);
relevances[1] = compute_relevance(zone_weights[zone][1], metrics->em);
relevances[2] = compute_relevance(zone_weights[zone][2], metrics->wk);
relevances[3] = compute_relevance(zone_weights[zone][3], metrics->energy);
relevances[4] = compute_relevance(zone_weights[zone][4], metrics->comfort);
```

**Protection mechanisms:**
1. `state->zone` is set only via `determine_zone()` which returns enum values (0-3)
2. Metric indices are hard-coded (0-4), not user-controlled
3. Zone enum type provides compile-time safety

**Recommendation (LOW):**
Add defensive assertion in `rebuild_rotation()`:
```c
if (zone > 3) return;  // Should never happen, but belt-and-suspenders
```

---

## 2. Integer Arithmetic Safety (Weighted Sums)

### Status: ✅ PASS

**Analysis:**
The relevance calculation in `compute_relevance()` is the only weighted arithmetic:

```c
static uint8_t compute_relevance(uint8_t weight, uint8_t metric_value) {
    int16_t deviation;  // ✅ CORRECT: Signed type prevents underflow
    if (metric_value > 50) {
        deviation = metric_value - 50;  // Range: 0-50
    } else {
        deviation = 50 - metric_value;  // Range: 0-50
    }
    
    // Relevance = weight * deviation / 50
    return (uint8_t)((weight * deviation) / 50);
}
```

**Safety verification:**
- Input range: `weight` ∈ [0, 100], `metric_value` ∈ [0, 100]
- `deviation` ∈ [0, 50] (unsigned magnitude)
- Maximum intermediate: 100 × 50 = 5,000 (fits in `int16_t`)
- Result: `relevance` ∈ [0, 100] (always fits in `uint8_t`)

**No overflow possible.** ✅

---

## 3. Division-by-Zero Protection

### Status: ✅ PASS

**Analysis:**
Only division operation in the module:
```c
return (uint8_t)((weight * deviation) / 50);
```

**Divisor is a compile-time constant (50).** No division-by-zero risk. ✅

---

## 4. Face Index Bounds Checking

### Status: ⚠️ PASS (with recommendation)

**Analysis:**

### 4a. `face_indices` array (size 6)
```c
uint8_t face_indices[6];  // Max 6 faces (5 metrics + 1 future expansion)
```

**Write operations:**
```c
// rebuild_rotation() - lines 84-88
for (uint8_t i = 0; i < 5; i++) {  // ✅ Loop bounded to 5 metrics
    if (relevances[i] >= MIN_RELEVANCE) {
        state->face_indices[state->face_count++] = i;  // ✅ Max 5 writes
    }
}
```
**Protection:** Loop limit (5) < array size (6). ✅

**Read operations:**
```c
// playlist_get_current_face() - line 163
if (state->face_count == 0) {
    return 0;  // ✅ Guard against empty rotation
}
return state->face_indices[state->current_face];  // ⚠️ See below
```

**Potential issue:**
- `current_face` is advanced in `playlist_advance()` without bounds check against `face_count`
- If `face_count` shrinks during runtime (zone change), `current_face` could be out-of-bounds

**Actually protected by:**
```c
// playlist_advance() - lines 178-180
state->current_face++;
if (state->current_face >= state->face_count) {  // ✅ Wrap-around protection
    state->current_face = 0;
}
```

**However:**
- `rebuild_rotation()` resets `current_face = 0` on zone change (line 105) ✅
- `playlist_advance()` wraps on overflow ✅

**Edge case concern:**
What if `playlist_get_current_face()` is called between:
1. `rebuild_rotation()` reducing `face_count` (e.g., from 5 to 2)
2. Before `current_face` is reset to 0?

**Analysis of `rebuild_rotation()` (lines 100-105):**
```c
state->face_count = 0;  // Reset count
for (uint8_t i = 0; i < 5; i++) {
    if (relevances[i] >= MIN_RELEVANCE) {
        state->face_indices[state->face_count++] = i;
    }
}
state->current_face = 0;  // ✅ Immediately reset after rebuild
```

**Safe! Reset happens atomically within same function.** ✅

**Recommendation (MEDIUM):**
Add defensive bounds check in `playlist_get_current_face()`:
```c
uint8_t playlist_get_current_face(const playlist_state_t *state) {
    if (state->face_count == 0) {
        return 0;  // Default to SD metric
    }
    // Defensive: Clamp current_face to valid range
    uint8_t index = state->current_face;
    if (index >= state->face_count) {
        index = 0;  // Should never happen, but safety first
    }
    return state->face_indices[index];
}
```

---

## 5. Zone Transition Logic Correctness

### Status: ✅ PASS

**Hysteresis implementation analysis:**

```c
void playlist_update(...) {
    phase_zone_t new_zone = determine_zone(phase_score);
    
    if (new_zone != state->zone) {
        if (new_zone == state->pending_zone) {
            state->consecutive_count++;  // Same zone: increment counter
            if (state->consecutive_count >= ZONE_HYSTERESIS_COUNT) {
                state->zone = new_zone;  // Transition confirmed
                state->consecutive_count = 0;
                rebuild_rotation(state, metrics);
            }
        } else {
            state->pending_zone = new_zone;  // New zone: reset counter
            state->consecutive_count = 1;
        }
        return;  // Don't update dwell during transition
    }
    
    // Same zone: reset hysteresis
    state->pending_zone = state->zone;
    state->consecutive_count = 0;
    ...
}
```

**Correctness verification:**

| State | Input | Action | Result |
|-------|-------|--------|--------|
| Zone A, pending=A, count=0 | Zone B | pending=B, count=1 | Waiting |
| Zone A, pending=B, count=1 | Zone B | count=2 | Waiting |
| Zone A, pending=B, count=2 | Zone B | count=3 → Transition to B | ✅ Confirmed |
| Zone A, pending=B, count=2 | Zone C | pending=C, count=1 | Reset (flicker protection) ✅ |
| Zone A, pending=B, count=2 | Zone A | pending=A, count=0 | Aborted (returned to stable) ✅ |

**Logic is correct.** ✅

**Note:** `return` statement (line 138) prevents dwell timer updates during zone transitions, which is correct to avoid face rotation during unstable periods. ✅

---

## 6. Hysteresis Buffer Overflow/Underflow

### Status: ✅ PASS

**State variables:**
```c
uint8_t consecutive_count;  // Range: 0-255 (uint8_t)
```

**Increment operations:**
```c
state->consecutive_count++;  // Line 132
```

**Maximum value reached:**
- Hysteresis threshold: `ZONE_HYSTERESIS_COUNT = 3`
- Counter is reset to 0 after reaching 3 (line 135)
- Counter is reset to 1 on zone change (line 141)
- Counter is reset to 0 when stable (line 147)

**Maximum possible value:** 3 (well within `uint8_t` range of 255)

**No overflow risk.** ✅

**Underflow check:**
No decrement operations exist. No underflow risk. ✅

---

## 7. Memory Safety in Playlist State

### Status: ✅ PASS

**State structure analysis:**
```c
typedef struct {
    uint8_t zone;                    // 1 byte
    uint8_t face_count;              // 1 byte
    uint8_t face_indices[6];         // 6 bytes
    uint8_t current_face;            // 1 byte
    uint16_t dwell_ticks;            // 2 bytes
    uint16_t dwell_limit;            // 2 bytes
    uint8_t pending_zone;            // 1 byte
    uint8_t consecutive_count;       // 1 byte
} playlist_state_t;  // Total: 15 bytes
```

**Initialization:**
```c
void playlist_init(playlist_state_t *state) {
    state->zone = ZONE_EMERGENCE;
    state->face_count = 0;
    state->current_face = 0;
    state->dwell_ticks = 0;
    state->dwell_limit = DEFAULT_DWELL_LIMIT;
    state->pending_zone = ZONE_EMERGENCE;
    state->consecutive_count = 0;
}
```

**Note:** `face_indices` array is not explicitly zeroed, but this is safe because:
1. `face_count = 0` marks the array as empty
2. All reads check `face_count` first
3. Array is fully populated before use

**Memory access patterns:**
- All struct accesses use valid field names (compile-time checked)
- No pointer arithmetic on the struct
- No buffer copies that could overrun

**Safe.** ✅

**Recommendation (LOW):**
Document assumption in header that caller zeros struct before `playlist_init()`:
```c
/**
 * Initialize playlist controller.
 * Call once at startup.
 * 
 * @param state Playlist state (should be zeroed by caller before first call)
 */
```

---

## Additional Observations

### Positive Security Practices Found:

1. **Proactive fix:** Integer underflow in bubble sort was caught and fixed (commit 81bcc9b) ✅
2. **Const correctness:** Weight tables stored in flash (`const`) ✅
3. **Defensive checks:** Empty rotation checks in multiple functions ✅
4. **Type safety:** Enum types used for zones ✅
5. **Bounded loops:** All loops use compile-time or verified runtime bounds ✅

### Code Quality:

- Clear comments explaining logic
- Consistent naming conventions
- Reasonable function sizes
- Good separation of concerns

---

## Recommendations Summary

### MEDIUM Priority:

1. **Add defensive bounds check in `playlist_get_current_face()`** (Section 4)
   - Prevents potential out-of-bounds read if state becomes inconsistent
   - Low probability but high impact

### LOW Priority:

2. **Add zone bounds assertion in `rebuild_rotation()`** (Section 1)
   - Belt-and-suspenders for weight table access
   
3. **Document struct initialization requirement** (Section 7)
   - Clarify caller responsibility for zeroing state

---

## Test Coverage Assessment

Reviewed `test_playlist.sh`:
- ✅ Zone determination logic
- ✅ Hysteresis behavior
- ✅ Relevance calculation
- ✅ Face rotation ordering
- ✅ Auto-advance timing
- ✅ Compilation test

**Note:** Tests are manual verification scripts, not automated unit tests. Consider adding runtime assertions in debug builds for continuous validation.

---

## Conclusion

**SECURITY APPROVAL GRANTED** ✅

The weighted playlist controller implementation is **safe for merge** with the current code. The two medium-priority recommendations are defensive enhancements that further reduce theoretical risk, but are not blockers.

**Suggested actions:**
1. Consider implementing MEDIUM recommendation #1 (defensive bounds check)
2. Merge PR #61
3. Address LOW recommendations in a follow-up cleanup commit (optional)

**Risk assessment:** **LOW**  
**Confidence level:** **HIGH**  

The code demonstrates mature embedded safety practices and is suitable for deployment to production hardware.

---

## Sign-off

Reviewed by: Security Specialist Agent  
Date: 2026-02-20 13:30 AKST  
Status: **APPROVED**
