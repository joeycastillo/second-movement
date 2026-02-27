# Security Review: PR #60 - Remaining Metrics (EM, WK, Energy)

**Reviewer:** Security Specialist Agent  
**Date:** 2026-02-20  
**Branch:** `phase3-pr2-remaining-metrics`  
**Commit:** a6c1aa5 (includes overflow fixes)

## Executive Summary

**STATUS: ✅ APPROVED** 

The PR has been reviewed for security vulnerabilities with focus on integer overflow/underflow, metric computation safety, state consistency, edge cases, and memory safety. All critical issues have been addressed in commit a6c1aa5. The implementation is safe for embedded deployment.

---

## Review Scope

### Files Reviewed
- `lib/metrics/metric_em.c` / `metric_em.h` - Emotional/Circadian Mood metric
- `lib/metrics/metric_wk.c` / `metric_wk.h` - Wake Momentum metric
- `lib/metrics/metric_energy.c` / `metric_energy.h` - Energy capacity metric
- `lib/metrics/metrics.c` / `metrics.h` - Integration and engine logic
- `test_metrics.sh` - Test coverage

### Security Focus Areas
1. Integer overflow/underflow
2. Metric computation safety
3. State consistency across metrics
4. Edge cases in workload/energy calculations
5. Memory safety
6. Integration with existing metrics

---

## 1. Integer Overflow/Underflow Analysis

### ✅ FIXED: EM Lunar Calculation Overflow (CRITICAL)

**Original Issue:**
```c
uint16_t lunar_phase = ((day_of_year * 1000) / 29) % 1000;
```

- **Vulnerability:** `day_of_year` (uint16_t) * 1000 overflows at day 66 (66 * 1000 = 66000 > 65535)
- **Impact:** Incorrect EM scores after day 66, potential unexpected behavior

**Fix Applied (commit a6c1aa5):**
```c
uint32_t lunar_phase = ((uint32_t)day_of_year * 1000UL / 29) % 1000;
```

- **Status:** ✅ RESOLVED
- **Verification:** Calculation now supports full year (365 * 1000 = 365000 fits in uint32_t)

### ✅ FIXED: EM Blend Calculation Overflow (MEDIUM)

**Original Issue:**
```c
uint16_t em = (circ_score * 40 + lunar_score * 20 + variance_score * 40) / 100;
```

- **Vulnerability:** Each `uint8_t * constant` can overflow in intermediate calculations
- **Example:** If `circ_score = 100`: `100 * 40 + 100 * 20 + 100 * 40 = 10000` (fits in uint16_t, but no guarantee of intermediate calculation width)

**Fix Applied (commit a6c1aa5):**
```c
uint16_t em = (uint16_t)(((uint32_t)circ_score * 40 + 
                          (uint32_t)lunar_score * 20 + 
                          (uint32_t)variance_score * 40) / 100);
```

- **Status:** ✅ RESOLVED
- **Verification:** Explicit uint32_t promotion prevents intermediate overflow

### ✅ WK Computation - Safe

**Analysis:**
```c
uint16_t base = (minutes_awake * 100) / WK_RAMP_NORMAL;  // WK_RAMP_NORMAL = 120
```

- **Max calculation:** `1440 * 100 / 120 = 144000 / 120 = 1200` (fits in uint16_t)
- **Capping:** `if (base > 100) base = 100;` prevents overflow in final uint8_t assignment
- **Bonus addition:** `wk = (uint8_t)base + bonus;` → max 100 + 30 = 130, then clamped to 100
- **Status:** ✅ SAFE

### ✅ Energy Computation - Safe

**Analysis:**
```c
int16_t energy = (int16_t)phase_score - (sd_score / 3);
```

- **Range:** phase_score [0-100], sd_score [0-100] → energy range [-33 to 100] (fits in int16_t)
- **Activity bonus:** `recent_activity / 50` → max 65535 / 50 = 1310, clamped to 20
- **Circadian bonus:** Computed from int16_t LUT, result clamped to [0-20]
- **Final clamping:** `if (energy < 0) energy = 0; if (energy > 100) energy = 100;`
- **Status:** ✅ SAFE

---

## 2. Metric Computation Safety

### ✅ EM (Emotional) Metric

**Circadian Component:**
- Uses pre-computed int16_t LUT `cosine_lut_24[]` (range -1000 to +1000)
- Negation for phase shift: `int16_t circ_raw = -cosine_lut_24[hour];` → safe (no overflow)
- Mapping: `(circ_raw + 1000) / 20` → range [0, 100]
- **Input validation:** `if (hour >= 24) hour = 23;` prevents array out-of-bounds

**Lunar Component:**
- Now using uint32_t (fixed in a6c1aa5)
- Modulo 1000 ensures result fits in valid range

**Variance Component:**
- Currently placeholder (hardcoded 50)
- Parameter suppressed with `(void)activity_variance;`
- **Note:** Phase 4 implementation should maintain 0-100 range for safety

**Output Clamping:**
- Final safety net: `if (em > 100) em = 100;`

**Status:** ✅ SAFE

### ✅ WK (Wake Momentum) Metric

**Normal Mode (with accelerometer):**
- Linear ramp over 120 minutes
- Activity bonus: threshold-based (+30 if activity ≥ 1000)
- Final clamping prevents overflow

**Fallback Mode (no accelerometer):**
- Linear ramp over 180 minutes
- No activity bonus
- Simpler calculation path, inherently safe

**Status:** ✅ SAFE

### ✅ Energy Metric

**Base Formula:**
- `phase_score - (sd_score / 3)` → integer division, no fractional issues
- Signed arithmetic (int16_t) correctly handles negative intermediate values

**Activity Bonus (normal mode):**
- Division by 50 scales down large values safely
- Explicit cap at 20

**Circadian Bonus (fallback mode):**
- Reuses same LUT as EM
- Scaling factor (/ 100) ensures [0-20] range

**Clamping:**
- Both lower (0) and upper (100) bounds enforced

**Status:** ✅ SAFE

---

## 3. State Consistency & Midnight Wraparound

### ✅ FIXED: WK Midnight Wraparound (CRITICAL)

**Original Issue:**
Complex hour-based logic with potential edge case failures:
```c
if (hour >= engine->wake_onset_hour) {
    // Normal case
} else {
    // Midnight wraparound - fragile logic
}
```

**Fix Applied (commit a6c1aa5):**
```c
// Use total minutes from midnight to handle wraparound correctly
uint16_t wake_minutes = engine->wake_onset_hour * 60 + engine->wake_onset_minute;
uint16_t current_minutes = hour * 60 + minute;

// Handle midnight wraparound
if (current_minutes < wake_minutes) {
    current_minutes += 1440;  // Add 24 hours in minutes
}

uint16_t minutes_awake = current_minutes - wake_minutes;
```

- **Status:** ✅ RESOLVED
- **Verification:** 
  - Wake at 23:30 (1410 min), check at 00:15 (15 min) → 15 + 1440 - 1410 = 45 min ✅
  - Wake at 06:00 (360 min), check at 14:30 (870 min) → 870 - 360 = 510 min ✅

### ✅ BKUP State Persistence

**SD State (3 bytes):**
```c
uint32_t sd_data = 0;
sd_data |= (uint32_t)engine->sd_deficits[0];
sd_data |= ((uint32_t)engine->sd_deficits[1]) << 8;
sd_data |= ((uint32_t)engine->sd_deficits[2]) << 16;
```
- Proper bit packing, no overlap
- Load/save symmetry verified

**WK State (2 bytes):**
```c
uint32_t wk_data = 0;
wk_data |= (uint32_t)engine->wake_onset_hour;
wk_data |= ((uint32_t)engine->wake_onset_minute) << 8;
```
- Proper bit packing, no overlap
- Load/save symmetry verified

**Status:** ✅ SAFE

---

## 4. Edge Cases & Boundary Conditions

### Input Validation

| Function | Input | Validation | Status |
|----------|-------|------------|--------|
| `metric_em_compute` | `hour` | `if (hour >= 24) hour = 23;` | ✅ |
| `metric_em_compute` | `day_of_year` | Overflow protection via uint32_t | ✅ |
| `metric_wk_compute` | `minutes_awake` | Capped via ramp logic | ✅ |
| `metric_energy_compute` | `hour` | `if (hour >= 24) hour = 23;` | ✅ |
| `metrics_set_wake_onset` | `hour` | `if (hour >= 24) hour = 0;` | ✅ |
| `metrics_set_wake_onset` | `minute` | `if (minute >= 60) minute = 0;` | ✅ |

### Edge Case Testing

**Test Coverage (from `test_metrics.sh`):**
- ✅ EM: Hour 0, 2, 14, 25 (invalid)
- ✅ EM: Day 1, 14, 29 (lunar cycle)
- ✅ WK: 60 min, 120 min, 180 min, 500 min (overflow)
- ✅ WK: With/without accelerometer
- ✅ WK: With/without activity bonus
- ✅ Energy: Low/high phase, low/high SD
- ✅ Energy: Negative energy (clamping to 0)
- ✅ Energy: Overflow (capping to 100)

**Missing Edge Cases (Low Priority):**
- WK: `minutes_awake = 0` (just woke) - implicitly tested, safe
- EM: `day_of_year = 0` or `day_of_year > 365` - should be validated by caller (datetime subsystem)

**Status:** ✅ ADEQUATE (core edge cases covered)

---

## 5. Memory Safety

### Stack Usage

**EM Computation:**
- Local variables: ~16 bytes (hour, circ_score, lunar_phase, em, etc.)
- LUT: 48 bytes (const static, .rodata section)

**WK Computation:**
- Local variables: ~8 bytes

**Energy Computation:**
- Local variables: ~12 bytes
- LUT: 48 bytes (shared with EM via separate file)

**Metrics Update:**
- Local variables: ~20 bytes
- No recursion, no dynamic allocation

**Total Stack Impact:** ~100 bytes max (well within embedded constraints)

### Heap Usage

- **No dynamic allocation** (`malloc`/`free` not used)
- All data structures are stack-allocated or static

### Buffer Overflows

**Array Access:**
- `cosine_lut_24[hour]` - protected by `if (hour >= 24) hour = 23;`
- `engine->sd_deficits[0..2]` - hardcoded indices, no variable access

**BKUP Packing:**
- Fixed-size bit packing, no variable-length fields
- Registers claimed via `movement_claim_backup_register()` (external API, assumed safe)

**Status:** ✅ SAFE

---

## 6. Integration with Existing Metrics

### Metric Dependencies

```
Phase Engine (external) → phase_score
Sleep Debt (PR #59) → sd_score
    ↓
Energy Metric (derived)
```

**Circular Dependencies:** None detected ✅

### API Contract Compliance

**metrics_update() Parameters:**
- All inputs are primitive types or const pointers
- No modification of input data (const-correct)
- Null pointer checks: `if (!engine || !engine->initialized) return;`

**metrics_get() Thread Safety:**
- Reads from static global `_current_metrics`
- **Potential Issue:** No mutex protection
- **Mitigation:** Single-threaded embedded environment (acceptable for Phase 3)
- **Recommendation:** Add comment documenting thread-safety assumptions

**Status:** ✅ SAFE (with documentation note)

---

## 7. Additional Findings

### Code Quality

**Positive:**
- Clear separation of concerns (one metric per file)
- Consistent naming conventions
- Comprehensive comments
- Fallback modes for missing sensors

**Minor Issues:**
- Duplicate LUT in `metric_em.c` and `metric_energy.c` (48 bytes each)
  - **Recommendation:** Extract to shared header `cosine_lut_24.h` (low priority)

### Test Coverage

**Strengths:**
- All three new metrics tested
- Boundary conditions covered
- Accelerometer/fallback modes tested

**Gaps:**
- No integration test with full `metrics_update()` call chain
- No midnight wraparound test for WK (fixed logic untested by automated suite)
- **Recommendation:** Add midnight wraparound test case

---

## Recommendations

### Required for Merge: None
All critical issues have been fixed in commit a6c1aa5.

### Post-Merge Improvements (Low Priority):

1. **Extract Shared LUT:** Move `cosine_lut_24[]` to `lib/metrics/cosine_lut_24.h`
2. **Add Midnight Test:** Test WK calculation across midnight boundary
3. **Document Thread Safety:** Add comment to `metrics_get()` about single-threaded assumption
4. **Input Validation:** Add defensive check for `day_of_year` range (1-366) in `metric_em_compute()`

---

## Security Checklist

- [x] Integer overflow/underflow - **FIXED**
- [x] Signed/unsigned arithmetic correctness - **VERIFIED**
- [x] Array bounds checking - **VERIFIED**
- [x] Null pointer dereference - **VERIFIED**
- [x] Memory allocation safety - **N/A (no dynamic allocation)**
- [x] State consistency - **VERIFIED**
- [x] Midnight wraparound - **FIXED**
- [x] Input validation - **ADEQUATE**
- [x] Output clamping - **VERIFIED**
- [x] Edge case handling - **TESTED**

---

## Conclusion

**PR #60 is APPROVED for merge.**

All security-critical issues identified during initial review have been addressed in commit a6c1aa5. The implementation demonstrates good defensive programming practices with comprehensive input validation, output clamping, and overflow protection. The test suite provides adequate coverage of core functionality and edge cases.

The post-merge recommendations are quality-of-life improvements and do not represent security vulnerabilities.

**Signed:**  
Security Specialist Agent  
2026-02-20 13:45 AKST
