# Security Review: Phase 3 PR 6 - Integration & Polish (#64)

**Date:** 2026-02-20 13:40 AKST  
**Reviewer:** Security Specialist (Subagent)  
**Branch:** phase3-pr6-integration  
**Commit:** f24eb19  
**Status:** ⚠️ **CONDITIONAL APPROVAL** — 3 security issues require fixes

---

## Executive Summary

Phase 3 PR 6 integrates the metrics engine and playlist controller into the core movement system. The integration is **architecturally sound** with proper build flag isolation and zero-cost abstraction when disabled. However, **three security issues** require fixes before merge:

1. **Type narrowing cast** without validation (movement.c:565)
2. **Improper float comparison** (movement.c:553)
3. **Missing bounds check** for temperature conversion (movement.c:554)

Additionally, documentation should include security considerations for external integrators.

---

## Security Issues Found

### 🔴 CRITICAL: Type Narrowing Cast Without Validation

**File:** `movement.c`, line 565  
**Severity:** HIGH  
**Type:** Integer Overflow

**Issue:**
```c
uint16_t phase_score = 50;  // Stubbed value, will be replaced
...
metrics_update((metrics_engine_t *)&movement_state.metrics,
              hour, minute, day_of_year,
              (uint8_t)phase_score,  // ← Narrowing cast
              ...
```

**Problem:**  
When `phase_score` is replaced with `phase_compute()` output, it's a `uint16_t` (range 0-65535) being cast to `uint8_t` (range 0-255). If `phase_compute()` returns values >255, they will silently overflow.

**Impact:**  
Metric calculations would receive incorrect phase scores, corrupting Energy and Comfort metrics. While current stub value (50) is safe, future integration is vulnerable.

**Fix:**
```c
// Clamp phase score to valid range (0-100 per specification)
uint8_t phase_score_u8 = (phase_score > 100) ? 100 : (uint8_t)phase_score;
metrics_update((metrics_engine_t *)&movement_state.metrics,
              hour, minute, day_of_year,
              phase_score_u8,
              ...
```

**Rationale:** Phase scores are documented as 0-100 range. Values >100 should be clamped, not silently wrapped.

---

### 🔴 CRITICAL: Improper Float Comparison

**File:** `movement.c`, line 553  
**Severity:** MEDIUM  
**Type:** Type Confusion

**Issue:**
```c
float temp_c = movement_get_temperature();
if (temp_c != 0xFFFFFFFF) {  // ← Comparing float to hex constant
    temp_c10 = (int16_t)(temp_c * 10.0f);
}
```

**Problem:**  
Comparing a `float` to a `uint32_t` hex constant relies on implicit type conversion. The constant `0xFFFFFFFF` converted to float is approximately `4.2949673e9`, not NaN or a typical error sentinel for floats.

**Impact:**  
- If `movement_get_temperature()` returns actual error codes (e.g., NaN, -1.0), they won't be caught
- Comparison may fail due to floating-point precision issues
- Code intent is unclear (what does 0xFFFFFFFF mean for a temperature?)

**Fix:**
```c
float temp_c = movement_get_temperature();
// Check for valid temperature range (-40°C to +85°C is typical for sensors)
if (temp_c >= -50.0f && temp_c <= 100.0f) {
    temp_c10 = (int16_t)(temp_c * 10.0f);
}
```

**Alternative:** If `movement_get_temperature()` has a documented error return (e.g., NaN), use `isnan()`:
```c
if (!isnan(temp_c)) {
    temp_c10 = (int16_t)(temp_c * 10.0f);
}
```

**Rationale:** Range checks are safer and clearer than magic constants. Validates sensor data is physically plausible.

---

### 🟡 MODERATE: Integer Overflow in Temperature Conversion

**File:** `movement.c`, line 554  
**Severity:** MEDIUM  
**Type:** Integer Overflow

**Issue:**
```c
temp_c10 = (int16_t)(temp_c * 10.0f);
```

**Problem:**  
`int16_t` ranges from -32768 to +32767, representing -3276.8°C to +3276.7°C. While this is physically implausible, a malfunctioning sensor could return extreme float values (e.g., `1e10`), causing overflow.

**Impact:**  
Corrupted Comfort metric if sensor returns out-of-range values. Low probability but trivial to prevent.

**Fix:**
```c
if (temp_c >= -50.0f && temp_c <= 100.0f) {
    temp_c10 = (int16_t)(temp_c * 10.0f);
} else {
    // Sensor malfunction or out of range - use neutral value
    temp_c10 = 0;  // Or skip Comfort metric update
}
```

**Rationale:** Defense in depth. Prevents garbage sensor readings from corrupting metrics.

---

## Additional Findings (Non-Blocking)

### 🟢 LOW: Missing Security Documentation

**File:** `lib/metrics/README.md`, `lib/phase/README.md`  
**Severity:** LOW  
**Type:** Documentation Gap

**Issue:**  
Neither README includes a "Security Considerations" or "Input Validation" section.

**Recommendation:**  
Add sections documenting:
- Valid input ranges for all public APIs
- Behavior on invalid inputs (clamping, errors, or undefined)
- Assumptions about caller-provided data (e.g., "hour must be 0-23")

**Example:**
```markdown
## Security & Input Validation

### metrics_update()
- `hour`: Must be 0-23 (no validation, caller responsible)
- `minute`: Must be 0-59 (no validation, caller responsible)
- `phase_score`: Must be 0-100 (values >100 undefined behavior)
- `temp_c10`: Extreme values may saturate Comfort metric to 0 or 100
- `sleep_data`: NULL-safe (returns midpoint values)
```

**Impact:** Low priority for internal use, but important for external integrators or future maintainers.

---

### 🟢 INFO: Minor Data Integrity Issue (Non-Security)

**File:** `lib/metrics/metric_sd.c`, lines 45-50  
**Severity:** INFO  
**Type:** Logic Error

**Issue:**
```c
// Comment says "divide by 4 to fit in 0-100 range"
// But code doesn't actually divide:
deficits[i] = (deficit[i] > 100) ? 100 : (uint8_t)deficit[i];
```

**Problem:**  
The `deficits[]` array (used for BKUP persistence) stores raw deficit values clamped to 100, not scaled by dividing by 4 as the comment suggests. This doesn't affect the metric calculation (which uses the local `deficit[]` array), but persistence may not match the documented format.

**Impact:**  
None (calculations are correct). This is a comment/implementation mismatch.

**Fix:**  
Either update the comment to match reality, or implement the division:
```c
// Store deficit scaled to 0-100 range (max deficit 480 → 120, clamped to 100)
uint8_t scaled_deficit = (deficit[i] > 400) ? 100 : (uint8_t)(deficit[i] / 4);
deficits[i] = scaled_deficit;
```

**Recommendation:** Clarify intent and decide if scaling is needed for BKUP storage efficiency.

---

## Security Strengths

### ✅ Proper Build Flag Isolation

**Finding:** All Phase 3 code correctly guarded with `#ifdef PHASE_ENGINE_ENABLED`

**Evidence:**
- movement.h lines 59, 324-336
- movement.c lines 529, 692, 709, 1429
- All metrics and playlist includes conditional

**Test:** Verified zero-cost abstraction via test suite (symbols absent when disabled)

**Rating:** ✅ **EXCELLENT**

---

### ✅ Array Bounds Protection

**Finding:** Bubble sort in `playlist.c` guards against underflow

**Code (lines 101-103):**
```c
if (state->face_count > 1) {
    for (uint8_t i = 0; i < state->face_count - 1; i++) {
```

**Analysis:**  
Without the `face_count > 1` check, `face_count - 1` would underflow when `face_count == 0`, causing out-of-bounds array access. This was likely fixed in an earlier PR (commit 81bcc9b mentions "Fix integer underflow in bubble sort").

**Rating:** ✅ **GOOD**

---

### ✅ Input Validation in Critical Paths

**Finding:** `metrics_set_wake_onset()` validates time inputs

**Code (metrics.c lines 183-186):**
```c
if (hour >= 24) hour = 0;
if (minute >= 60) minute = 0;
```

**Analysis:**  
Ensures invalid time values are clamped to valid range. Prevents corrupted BKUP data.

**Rating:** ✅ **GOOD**

---

### ✅ Null Pointer Safety

**Finding:** Functions check for NULL pointers before dereferencing

**Examples:**
- `metrics_init()` line 46: `if (!engine) return;`
- `metrics_update()` line 82: `if (!engine || !engine->initialized) return;`
- `metrics_get()` line 139: `if (!out) return;`
- `metric_sd_compute()` line 32: `if (!sleep_data) { ... return 50; }`

**Rating:** ✅ **GOOD**

---

### ✅ Zero Compiler Warnings

**Finding:** Clean build with `-Wall` (verified by emulator test report)

**Evidence:** Commit 827dace fixed all 10 warnings:
- 8 volatile qualifier warnings (resolved with explicit casts)
- 1 unused function warning (marked `__attribute__((unused))`)
- 1 unused parameter warning (acknowledged with `(void)engine;`)

**Rating:** ✅ **EXCELLENT**

---

## Testing Coverage

### Automated Tests

**Test Suite:** `test_phase3_integration.sh` (270 lines)

**Coverage:**
1. ✅ Build with PHASE_ENGINE_ENABLED=1
2. ✅ Zero-cost verification (PHASE_ENGINE_ENABLED=0)
3. ✅ Flash/RAM budget checks
4. ✅ Compiler warnings scan
5. ✅ Header guard verification
6. ✅ `#ifdef` coverage verification

**Gaps:**
- ❌ No runtime tests (all tests are build-time only)
- ❌ No edge case tests (e.g., phase_score > 100, negative temps)
- ❌ No BKUP persistence tests
- ❌ No metric correctness tests

**Recommendation:** Add runtime tests in future PR:
```bash
# Example: Test metric clamping
assert_metrics_handle_invalid_inputs() {
    metrics_update(..., phase_score=300, ...);  # Should clamp to 100
    metrics_get(&snapshot);
    assert(snapshot.energy <= 100);
}
```

---

### Manual Testing

**Emulator Testing:** Comprehensive (see PHASE3_EMULATOR_TEST_REPORT.md)

**Results:**
- ✅ Firmware loads without errors
- ✅ Zone faces selectable
- ✅ Metrics display (stubbed values)
- ✅ ALARM button cycles views
- ✅ No crashes or hangs

**Note:** Stubbed sensor values mean edge cases weren't tested (e.g., extreme temperatures, phase scores >100).

---

## Resource Usage Verification

**Flash:** 138164 bytes (53.5% of 256 KB) ✅  
**RAM:** 5220 bytes BSS + 2796 data = 8016 bytes (24.4% of 32 KB) ✅  
**BKUP Registers:** 2 (5 bytes total) ✅

**Analysis:**  
All within budget. Phase 3 adds ~11.8 KB flash and ~72 bytes RAM when enabled. Zero overhead when disabled (verified by test suite).

---

## Configuration & Integration Safety

### Build Flags

**Guard:** `PHASE_ENGINE_ENABLED`

**Test:** Verified zero-cost abstraction:
```bash
# With flag enabled:
nm build/watch.elf | grep metrics_init  # ✅ Found

# With flag disabled:
nm build/watch.elf | grep metrics_init  # ✅ Not found
```

**Rating:** ✅ **SAFE**

---

### BKUP Register Claiming

**Mechanism:** `movement_claim_backup_register()` (movement.c)

**Code (metrics.c lines 49-50):**
```c
engine->bkup_reg_sd = movement_claim_backup_register();
engine->bkup_reg_wk = movement_claim_backup_register();
```

**Analysis:**  
Uses centralized allocator. Prevents conflicts with other subsystems. Registers are claimed at init, not hardcoded.

**Rating:** ✅ **SAFE**

---

### Volatile State Handling

**Issue:** Compiler warnings about discarding volatile qualifiers (fixed in commit 827dace)

**Fix:** Explicit casts when passing volatile state to functions:
```c
metrics_update((metrics_engine_t *)&movement_state.metrics, ...);
```

**Analysis:**  
Necessary because `movement_state` is declared volatile. Casts are safe because functions don't assume non-volatility. Proper solution would be to make function parameters `volatile`, but that's invasive.

**Rating:** ✅ **ACCEPTABLE** (pragmatic workaround)

---

## Recommendations

### 🔴 MUST FIX (Before Merge)

1. **Add bounds checking for phase_score cast** (movement.c:565)
   ```c
   uint8_t phase_score_u8 = (phase_score > 100) ? 100 : (uint8_t)phase_score;
   ```

2. **Fix temperature comparison** (movement.c:553)
   ```c
   if (temp_c >= -50.0f && temp_c <= 100.0f) {
   ```

3. **Add temperature overflow protection** (movement.c:554)
   ```c
   if (temp_c >= -50.0f && temp_c <= 100.0f) {
       temp_c10 = (int16_t)(temp_c * 10.0f);
   }
   ```

### 🟡 SHOULD FIX (Before Stable Release)

4. **Add security documentation** to README files
   - Document valid input ranges
   - Clarify error handling behavior
   - Specify caller responsibilities

5. **Add runtime edge case tests**
   - Test extreme sensor values
   - Test phase_score > 100
   - Verify BKUP persistence

6. **Clarify deficit storage logic** in metric_sd.c
   - Fix comment or implement scaling

### 🟢 NICE TO HAVE (Future Work)

7. **Consider adding assert()** macros for debug builds
   - Catch invalid inputs during development
   - Remove in release builds

8. **Add input validation layer** for sensor data
   - Centralized sanity checks for all sensors
   - Reject physically impossible values

---

## Conclusion

**Overall Assessment:** ⚠️ **CONDITIONAL APPROVAL**

Phase 3 PR 6 demonstrates **solid engineering** with proper abstraction, clean compilation, and comprehensive documentation. The integration architecture is sound and the zero-cost abstraction is correctly implemented.

However, **three security issues** require fixes:
1. Phase score narrowing cast
2. Temperature float comparison
3. Temperature overflow protection

These are **straightforward fixes** (5-10 lines total) and should not delay the PR significantly.

**Recommendation:**
1. ✅ Apply the three fixes listed in "MUST FIX"
2. ✅ Verify clean build after fixes
3. ✅ Re-run test suite
4. ✅ **APPROVE for merge**

Once fixed, this PR is **READY FOR MERGE**. The remaining recommendations (documentation, runtime tests) can be addressed in follow-up PRs without blocking integration.

---

## Files Reviewed

**Core Integration:**
- [x] `movement.c` (3 issues found)
- [x] `movement.h` (no issues)

**Metrics Engine:**
- [x] `lib/metrics/metrics.c` (no security issues)
- [x] `lib/metrics/metrics.h` (no issues)
- [x] `lib/metrics/metric_sd.c` (1 minor comment issue)
- [x] `lib/metrics/metric_em.c` (no issues)
- [x] `lib/metrics/metric_wk.c` (no issues)
- [x] `lib/metrics/metric_energy.c` (no issues)
- [x] `lib/metrics/metric_comfort.c` (no issues)
- [x] `lib/metrics/README.md` (missing security section)

**Playlist Controller:**
- [x] `lib/phase/playlist.c` (bounds check ✅)
- [x] `lib/phase/playlist.h` (no issues)
- [x] `lib/phase/README.md` (missing security section)

**Zone Faces:**
- [x] `watch-faces/complication/emergence_face.c` (no issues)
- [x] `watch-faces/complication/momentum_face.c` (no issues)
- [x] `watch-faces/complication/active_face.c` (no issues)
- [x] `watch-faces/complication/descent_face.c` (no issues)

**Test Suite:**
- [x] `test_phase3_integration.sh` (comprehensive build tests)

**Documentation:**
- [x] `PHASE3_BUDGET_REPORT.md` (thorough)
- [x] `PHASE3_EMULATOR_TEST_REPORT.md` (excellent)
- [x] `TASK_COMPLETE_PR6.md` (complete)

**Total Files Reviewed:** 25

---

**Reviewer:** Security Specialist (Subagent a6a3fb93)  
**Review Date:** 2026-02-20 13:40 AKST  
**Approval:** ⚠️ CONDITIONAL (pending 3 fixes)
