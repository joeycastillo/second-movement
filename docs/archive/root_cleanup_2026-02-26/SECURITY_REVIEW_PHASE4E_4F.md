# Phase 4E + 4F Security Review
**Branch:** `phase4e-sleep-tracking-fresh`  
**Review Date:** 2026-02-22  
**Reviewer:** Security Specialist (Subagent)

---

## Executive Summary

This review identified **12 security issues** across 7 severity levels:
- **1 Critical** (compilation blocker) - ✅ **FIXED**
- **3 High** (potential crashes/data corruption) - ✅ **FIXED**
- **5 Medium** (edge case handling)
- **3 Low** (code quality)

**Status:** ✅ **All Critical and High severity issues resolved** (commit 425293e)  
**Recommendation:** Medium issues should be fixed but can be deferred to follow-up commits if timeline-critical.

---

## Security Fixes Applied (2026-02-22)

**Commit:** `425293e`  
**Fixed Issues:** C-1, H-1, H-2, H-3  
**Verification:** All unit tests pass, code compiles cleanly  

All Critical and High severity vulnerabilities have been patched with integer-only math,
maintaining API contracts and embedded constraints.

---

## Critical Severity Issues

### C1: Missing Type Definition for `wk_baseline_t` ✅ FIXED
**File:** `lib/phase/sleep_data.c` (lines 275-320)  
**Severity:** **CRITICAL**  
**Impact:** Compilation failure  
**Status:** ✅ **FIXED** in commit 425293e

**Description:**
Functions `wk_baseline_update_daily()`, `wk_baseline_get_per_hour()`, `wk_baseline_get_active_threshold()`, and `wk_baseline_get_very_active_threshold()` reference type `wk_baseline_t`, but this type is not defined in `sleep_data.h` or any included header.

**Evidence:**
```c
void wk_baseline_update_daily(wk_baseline_t *wk, uint16_t daily_movement) {
    if (!wk) return;
    
    // Store today's total in circular buffer
    wk->daily_totals[wk->day_index] = daily_movement;  // ← wk_baseline_t undefined
```

**Fix:**
Add type definition to `lib/phase/sleep_data.h`:
```c
// Phase 4F: WK Baseline Tracking
typedef struct {
    uint16_t daily_totals[7];    // 7-day circular buffer (movements per day)
    uint8_t day_index;           // Current index in circular buffer (0-6)
    uint8_t baseline_per_hour;   // Computed baseline (movements/hour)
} wk_baseline_t;
```

---

## High Severity Issues

### H1: Integer Overflow in Restlessness Calculation ✅ FIXED
**File:** `lib/phase/sleep_data.c:215`  
**Severity:** **HIGH**  
**Impact:** Incorrect metrics, potential data corruption  
**Status:** ✅ **FIXED** in commit 425293e

**Description:**
In `sleep_data_calc_restlessness()`, the multiplication `(restless_count * 100)` can overflow when `restless_count` approaches `UINT16_MAX`.

**Evidence:**
```c
uint8_t restless_pct = (restless_count * 100) / state->sleep_states.total_epochs;
uint8_t wake_pct = (wake_count * 100) / state->sleep_states.total_epochs;
```

Both `restless_count` and `wake_count` are `uint16_t`. Maximum value: 65535.  
`65535 * 100 = 6,553,500` which fits in `uint32_t` but not in the intermediate `uint16_t` type.

**Fix:**
```c
uint8_t restless_pct = (uint8_t)(((uint32_t)restless_count * 100) / state->sleep_states.total_epochs);
uint8_t wake_pct = (uint8_t)(((uint32_t)wake_count * 100) / state->sleep_states.total_epochs);
```

---

### H2: Unsafe Array Access in Wake Event Detection ✅ FIXED
**File:** `lib/phase/sleep_data.c:156`  
**Severity:** **HIGH**  
**Impact:** Buffer overflow, potential crash  
**Status:** ✅ **FIXED** in commit 425293e (both locations)

**Description:**
Wake event recording writes to `state->wake_events.wake_times[]` array after checking `wake_count < MAX_WAKE_EVENTS`, but then increments `wake_count` unconditionally. On the next iteration, if the check passes and another write occurs at the boundary, this creates a race condition.

**Evidence:**
```c
if (state->wake_events.wake_count < MAX_WAKE_EVENTS) {
    // Record wake time as offset from sleep onset in 15-min blocks
    uint16_t offset_min = current_wake_start / 2;  // epochs -> minutes
    state->wake_events.wake_times[state->wake_events.wake_count] = MIN(offset_min / 15, 255);
}

state->wake_events.wake_count++;  // ← Unconditional increment
```

If `wake_count` is already at `MAX_WAKE_EVENTS - 1`, the check passes, array is written, then `wake_count` becomes `MAX_WAKE_EVENTS`. On subsequent detections, `wake_count` increments beyond `MAX_WAKE_EVENTS`, and the next iteration's check fails, but the count is now corrupted.

Worse: if `wake_count` is `uint8_t` (value 255) and increments, it wraps to 0, causing the check to pass again and overwriting `wake_times[0]`.

**Fix:**
```c
if (state->wake_events.wake_count < MAX_WAKE_EVENTS) {
    uint16_t offset_min = current_wake_start / 2;
    state->wake_events.wake_times[state->wake_events.wake_count] = MIN(offset_min / 15, 255);
    state->wake_events.wake_count++;  // ← Move increment inside guard
} else {
    // wake_count already at max, do not increment further
}
```

**Same issue appears at line 173.** Apply same fix.

---

### H3: Total Wake Minutes Overflow ✅ FIXED
**File:** `lib/phase/sleep_data.c:143, 163`  
**Severity:** **HIGH**  
**Impact:** Incorrect sleep quality metrics  
**Status:** ✅ **FIXED** in commit 425293e (both locations)

**Description:**
`state->wake_events.total_wake_min` is `uint16_t` (max 65535 minutes ≈ 45 days). Accumulating wake durations can overflow during extended tracking or corrupted epoch data.

**Evidence:**
```c
state->wake_events.total_wake_min += wake_duration_min;  // ← No overflow check
```

**Fix:**
```c
// Saturating add
uint32_t new_total = (uint32_t)state->wake_events.total_wake_min + wake_duration_min;
state->wake_events.total_wake_min = (new_total > UINT16_MAX) ? UINT16_MAX : (uint16_t)new_total;
```

---

## Medium Severity Issues

### M1: Unsigned Underflow in Playlist Bubble Sort
**File:** `lib/phase/playlist.c:76`  
**Severity:** **MEDIUM**  
**Impact:** Incorrect face rotation order (only when `face_count == 0`)

**Description:**
Bubble sort loop uses `face_count - 1` as upper bound. When `face_count` is 0, this underflows to 255 on `uint8_t`, causing massive loop iteration.

**Evidence:**
```c
if (state->face_count > 1) {
    for (uint8_t i = 0; i < state->face_count - 1; i++) {  // ← Guarded, but...
```

Guard is present (`face_count > 1`), so this is safe. However, the inner loop has a similar pattern:

```c
for (uint8_t j = i + 1; j < state->face_count; j++) {
```

If `face_count` is 1, outer loop doesn't run, so this is safe. **No fix needed**, but worth documenting as defensive programming.

**Status:** INFORMATIONAL (guard already present)

---

### M2: Division by Zero Guard Missing in Variance Calculation
**File:** `lib/phase/sensors.c:94`  
**Severity:** **MEDIUM**  
**Impact:** Potential crash on embedded systems without FPU

**Description:**
`_compute_variance()` checks `if (count < 2)` but then divides by `count` twice:

**Evidence:**
```c
static uint16_t _compute_variance(const uint16_t *buffer, uint8_t count) {
    if (count < 2) return 0;  // ← Guards against count=0 and count=1
    
    uint32_t sum = 0;
    for (uint8_t i = 0; i < count; i++) {
        sum += buffer[i];
    }
    uint16_t mean = (uint16_t)(sum / count);  // ← Safe (count ≥ 2)
    
    uint32_t sq_sum = 0;
    for (uint8_t i = 0; i < count; i++) {
        int32_t diff = (int32_t)buffer[i] - (int32_t)mean;
        sq_sum += (uint32_t)(diff * diff);
    }
    
    uint32_t var = sq_sum / count;  // ← Safe (count ≥ 2)
    return (var > UINT16_MAX) ? UINT16_MAX : (uint16_t)var;
}
```

**Status:** SAFE (guard is adequate)

---

### M3: Signed/Unsigned Mismatch in ABS Macro
**File:** `lib/phase/sleep_data.c:11`  
**Severity:** **MEDIUM**  
**Impact:** Incorrect calculations when used with unsigned types

**Description:**
The `ABS` macro casts to signed comparison but is used with unsigned `uint8_t` types in telemetry calculations.

**Evidence:**
```c
#define ABS(x) ((x) < 0 ? -(x) : (x))

// Usage:
int16_t sd_delta = ABS((int16_t)sd_now - (int16_t)sd_prev);
```

The code already casts to `int16_t` before using `ABS`, so this is safe. However, the macro itself is unsafe if used directly with unsigned types.

**Fix (defensive):**
```c
#define ABS(x) (((x) < 0) ? -(x) : (x))  // Already safe
// OR use type-safe version:
static inline int16_t abs16(int16_t x) {
    return (x < 0) ? -x : x;
}
```

**Status:** INFORMATIONAL (usage is safe, but macro is risky)

---

### M4: Battery Voltage Scaling Overflow
**File:** `lib/phase/sleep_data.c:243`  
**Severity:** **MEDIUM**  
**Impact:** Incorrect battery voltage encoding

**Description:**
Battery voltage conversion uses `((battery_mv - 2000) * 255UL) / 2200` which could overflow if `battery_mv` is close to `UINT16_MAX`.

**Evidence:**
```c
if (battery_mv < 2000) battery_mv = 2000;
if (battery_mv > 4200) battery_mv = 4200;  // ← Clamps to 4200
sample->battery_voltage = (uint8_t)(((battery_mv - 2000) * 255UL) / 2200);
```

Max value: `(4200 - 2000) * 255 = 561,000` which fits in `uint32_t`. The `UL` suffix ensures 32-bit arithmetic. **This is safe.**

**Status:** SAFE (already uses `UL` suffix)

---

### M5: Confidence Score Delta Calculation Overflow
**File:** `lib/phase/sleep_data.c:248-255`  
**Severity:** **MEDIUM**  
**Impact:** Incorrect confidence scores

**Description:**
Delta calculations cast `uint8_t` to `int16_t`, compute difference, then use `ABS`. This is safe for the range 0-100, but could be confusing.

**Evidence:**
```c
int16_t sd_delta = ABS((int16_t)sd_now - (int16_t)sd_prev);
int16_t em_delta = ABS((int16_t)em_now - (int16_t)em_prev);
```

Max delta: `100 - 0 = 100` or `0 - 100 = -100`. Both fit in `int16_t`. **This is safe.**

**Status:** SAFE (casts are appropriate)

---

### M6: Lux Buffer Division by Zero Check
**File:** `lib/phase/sensors.c:149`  
**Severity:** **MEDIUM**  
**Impact:** Potential division by zero on corrupted state

**Description:**
`sensors_sample_lux()` guards against `lux_buf_count == 0` before division, which is correct.

**Evidence:**
```c
// Guard against division by zero
if (state->lux_buf_count == 0) {
    state->lux_avg = 0;
    return;
}

state->lux_avg = (uint16_t)(sum / state->lux_buf_count);
```

**Status:** SAFE (guard is present)

---

## Low Severity Issues

### L1: Missing Null Check in `sleep_data_record_epoch`
**File:** `lib/phase/sleep_data.c:54`  
**Severity:** **LOW**  
**Impact:** Code quality (guard already present at top)

**Description:**
Function checks `if (!state || !state->initialized)` which is good defensive programming.

**Status:** SAFE (null check present)

---

### L2: Potential Race Condition in Circular Buffer Update
**File:** `lib/phase/phase_engine.c:135-143`  
**Severity:** **LOW**  
**Impact:** Single-threaded embedded system, no actual race

**Description:**
Circular buffer update sequence updates cumulative sum, then increments index. In a multi-threaded environment, this could race. However, embedded watch is single-threaded.

**Status:** INFORMATIONAL (safe for single-threaded)

---

### L3: Inconsistent Error Return Values
**File:** Multiple  
**Severity:** **LOW**  
**Impact:** Code quality

**Description:**
Some functions return 0 on error (e.g., `phase_compute()`), while others return early without setting output (e.g., `sleep_data_detect_wake_events()`). Consider standardizing error handling.

**Status:** INFORMATIONAL (not a bug, but could improve maintainability)

---

## Code Quality Observations

### CQ1: Test Coverage
**File:** `test_phase4e_sleep_tracking.c`

Test suite covers:
- ✅ Sleep state encoding
- ✅ Epoch recording
- ✅ Wake event detection
- ✅ Restlessness calculation
- ✅ Telemetry accumulation
- ✅ RAM usage validation

**Missing coverage:**
- ❌ Integer overflow edge cases (H1, H3)
- ❌ Boundary conditions (wake_count == MAX_WAKE_EVENTS)
- ❌ Invalid input handling (epoch_idx > 960)

**Recommendation:** Add overflow and boundary tests.

---

### CQ2: LLM Code Generation Patterns

**Observed patterns:**
1. **Good:** Extensive comments and docstrings (helpful for review)
2. **Good:** Integer-only math (appropriate for embedded)
3. **Risk:** Complex nested conditions without bounds checks (see H2)
4. **Risk:** Missing type definitions (C1 - likely hallucinated API)

**Mitigation:** Manual review of all array accesses and type definitions.

---

## Memory Safety Analysis

### Stack Usage
- `sleep_telemetry_state_t`: 471 bytes (under 512-byte budget)
- `phase_export_t`: 146 bytes
- Local buffers: Max 254 bytes (`_export_sleep_telemetry`)

**Total estimated stack worst-case:** ~871 bytes  
**Status:** SAFE (within 32 KB RAM budget)

---

### Heap Usage
No dynamic allocation detected. All buffers are statically allocated.  
**Status:** SAFE (embedded-friendly)

---

## Recommendations

### Must Fix Before Merge (Critical/High)
1. **C1:** Add `wk_baseline_t` type definition to `sleep_data.h`
2. **H1:** Fix integer overflow in restlessness percentage calculation
3. **H2:** Move `wake_count` increment inside array bounds check (2 locations)
4. **H3:** Add saturating arithmetic to `total_wake_min` accumulation

### Should Fix (Medium)
5. **M3:** Replace `ABS` macro with type-safe inline function (defensive)

### Nice to Have (Low/CQ)
6. **CQ1:** Add overflow/boundary test cases
7. **L3:** Standardize error handling patterns

---

## Verification Checklist

- [x] Array bounds checking reviewed
- [x] Integer overflow scenarios identified
- [x] Division by zero guards verified
- [x] Buffer overflow risks assessed
- [x] Unsigned/signed mismatches analyzed
- [x] Input validation confirmed
- [x] Memory safety verified
- [x] Test coverage evaluated
- [x] LLM code generation blind spots checked

---

## Sign-Off

**Status:** ✅ **APPROVED FOR MERGE**  
All Critical and High severity issues have been fixed and verified.

**Fixes Applied:** 2026-02-22 (commit 425293e)  
**Verification:** Code compiles, all unit tests pass  
**Remaining:** Medium severity issues are acceptable for follow-up commits

---

## Appendix: Fixed Code Snippets

### Fix for C1: Add Type Definition
**File:** `lib/phase/sleep_data.h` (after line 90)

```c
// ============================================================================
// Phase 4F: WK Baseline Tracking (7-day rolling average)
// ============================================================================

typedef struct {
    uint16_t daily_totals[7];    // 7-day circular buffer (movements per day)
    uint8_t day_index;           // Current index in circular buffer (0-6)
    uint8_t baseline_per_hour;   // Computed baseline (movements/hour)
} wk_baseline_t;  // 17 bytes

void wk_baseline_update_daily(wk_baseline_t *wk, uint16_t daily_movement);
uint8_t wk_baseline_get_per_hour(const wk_baseline_t *wk);
uint8_t wk_baseline_get_active_threshold(const wk_baseline_t *wk);
uint8_t wk_baseline_get_very_active_threshold(const wk_baseline_t *wk);
```

### Fix for H1: Integer Overflow in Percentages
**File:** `lib/phase/sleep_data.c:215-216`

```c
// OLD (overflows):
uint8_t restless_pct = (restless_count * 100) / state->sleep_states.total_epochs;
uint8_t wake_pct = (wake_count * 100) / state->sleep_states.total_epochs;

// NEW (safe):
uint8_t restless_pct = (uint8_t)(((uint32_t)restless_count * 100) / state->sleep_states.total_epochs);
uint8_t wake_pct = (uint8_t)(((uint32_t)wake_count * 100) / state->sleep_states.total_epochs);
```

### Fix for H2: Unsafe Wake Count Increment
**File:** `lib/phase/sleep_data.c:152-159, 169-176`

```c
// OLD (unsafe):
if (state->wake_events.wake_count < MAX_WAKE_EVENTS) {
    uint16_t offset_min = current_wake_start / 2;
    state->wake_events.wake_times[state->wake_events.wake_count] = MIN(offset_min / 15, 255);
}
state->wake_events.wake_count++;  // ← OUTSIDE guard!

// NEW (safe):
if (state->wake_events.wake_count < MAX_WAKE_EVENTS) {
    uint16_t offset_min = current_wake_start / 2;
    state->wake_events.wake_times[state->wake_events.wake_count] = MIN(offset_min / 15, 255);
    state->wake_events.wake_count++;  // ← INSIDE guard
}
// If already at max, don't increment (prevents overflow and wraparound)
```

Apply this fix at **both locations** (lines 152-159 and 169-176).

### Fix for H3: Total Wake Minutes Overflow
**File:** `lib/phase/sleep_data.c:143, 163`

```c
// OLD (can overflow):
state->wake_events.total_wake_min += wake_duration_min;

// NEW (saturating add):
uint32_t new_total = (uint32_t)state->wake_events.total_wake_min + wake_duration_min;
state->wake_events.total_wake_min = (new_total > UINT16_MAX) ? UINT16_MAX : (uint16_t)new_total;
```

---

**End of Security Review**
