# Security Review: PR #59 - Metric Engine Core

**Reviewer:** Security Specialist Agent  
**Date:** 2026-02-20  
**Branch:** phase3-pr1-metrics-core  
**PR:** https://github.com/dlorp/second-movement/pull/59

## Executive Summary

**Status:** ⚠️ **CONDITIONAL APPROVAL WITH REQUIRED FIXES**

This review identified **3 CRITICAL** and **2 HIGH** severity security issues that must be addressed before merge. The metric engine core implementation is generally well-structured with good defensive programming practices, but several arithmetic overflow vulnerabilities and input validation gaps pose potential stability and correctness risks.

---

## Critical Issues (Must Fix)

### 🔴 CRITICAL-1: Integer Overflow in Wake Momentum Calculation

**File:** `lib/metrics/metric_wk.c`  
**Lines:** 39-40, 51-52  
**Severity:** CRITICAL

**Issue:**
```c
uint16_t base = (minutes_awake * 100) / WK_RAMP_NORMAL;
```

When `minutes_awake` approaches the upper bound of `uint16_t` (65535), the multiplication `minutes_awake * 100` can overflow:
- Example: `minutes_awake = 32768` → `32768 * 100 = 3,276,800` (exceeds uint16_t max of 65535)
- This causes wraparound and produces incorrect WK scores

**Attack Vector:**
If a user doesn't call `metrics_set_wake_onset()` for an extended period (e.g., wearing watch continuously for days), `minutes_awake` will grow unbounded and eventually overflow.

**Fix:**
Promote to 32-bit arithmetic before multiplication:
```c
uint32_t base = ((uint32_t)minutes_awake * 100) / WK_RAMP_NORMAL;
if (base > 100) base = 100;
wk = (uint8_t)base + bonus;
```

Apply the same fix to the fallback mode calculation.

---

### 🔴 CRITICAL-2: Signed Integer Overflow in Energy Computation

**File:** `lib/metrics/metric_energy.c`  
**Lines:** 52-53  
**Severity:** CRITICAL

**Issue:**
```c
int16_t energy = (int16_t)phase_score - (sd_score / 3);
// ... later ...
energy += circ_bonus;  // or activity_bonus
```

The `energy` variable uses `int16_t`, which can overflow if:
1. Initial calculation: `phase_score` (max 100) - `sd_score/3` (max ~33) = max 100 (safe)
2. After adding bonus: `energy + bonus` (max 100 + 20 = 120) (safe)

However, the issue is that `phase_score` is `uint16_t`, not `uint8_t`. If an invalid value > 255 is passed, overflow occurs:
- Example: `phase_score = 32000`, `sd_score = 0` → `energy = 32000` (exceeds int16_t max of 32767)

**Root Cause:**
The function accepts `uint16_t phase_score` but expects values 0-100. No validation is performed.

**Fix:**
Add input validation:
```c
uint8_t metric_energy_compute(uint16_t phase_score, uint8_t sd_score, uint16_t recent_activity,
                              uint8_t hour, bool has_accelerometer) {
    // Validate phase_score is in expected range
    if (phase_score > 100) phase_score = 100;
    
    int16_t energy = (int16_t)phase_score - (sd_score / 3);
    // ... rest of function
}
```

---

### 🔴 CRITICAL-3: Lunar Phase Calculation Integer Overflow

**File:** `lib/metrics/metric_em.c`  
**Lines:** 65-66  
**Severity:** CRITICAL

**Issue:**
```c
uint16_t lunar_phase = ((day_of_year * 1000) / 29) % 1000;
```

When `day_of_year` is near its maximum (365), the multiplication overflows:
- `365 * 1000 = 365,000` (exceeds uint16_t max of 65535)
- This wraps around and produces incorrect lunar phase values

**Fix:**
Promote to 32-bit arithmetic:
```c
uint32_t lunar_phase = ((uint32_t)day_of_year * 1000 / 29) % 1000;
```

---

## High Severity Issues (Should Fix)

### 🟠 HIGH-1: Missing Wake Onset Validation on Load from BKUP

**File:** `lib/metrics/metrics.c`  
**Lines:** 158-162  
**Severity:** HIGH

**Issue:**
When loading wake onset time from BKUP registers, no validation is performed:
```c
engine->wake_onset_hour = (uint8_t)(wk_data & 0xFF);
engine->wake_onset_minute = (uint8_t)((wk_data >> 8) & 0xFF);
```

If BKUP memory is corrupted or uninitialized:
- `wake_onset_hour` could be > 23
- `wake_onset_minute` could be > 59

This leads to incorrect `minutes_awake` calculations in `metrics_update()`.

**Fix:**
Add validation after loading:
```c
void metrics_load_bkup(metrics_engine_t *engine) {
    if (!engine || engine->bkup_reg_sd == 0 || engine->bkup_reg_wk == 0) return;
    
    // Load SD state from BKUP
    uint32_t sd_data = watch_get_backup_data(engine->bkup_reg_sd);
    engine->sd_deficits[0] = (uint8_t)(sd_data & 0xFF);
    engine->sd_deficits[1] = (uint8_t)((sd_data >> 8) & 0xFF);
    engine->sd_deficits[2] = (uint8_t)((sd_data >> 16) & 0xFF);
    
    // Load WK state from BKUP
    uint32_t wk_data = watch_get_backup_data(engine->bkup_reg_wk);
    engine->wake_onset_hour = (uint8_t)(wk_data & 0xFF);
    engine->wake_onset_minute = (uint8_t)((wk_data >> 8) & 0xFF);
    
    // Validate loaded values
    if (engine->wake_onset_hour >= 24) engine->wake_onset_hour = 0;
    if (engine->wake_onset_minute >= 60) engine->wake_onset_minute = 0;
}
```

---

### 🟠 HIGH-2: Temperature Deviation Overflow in Comfort Metric

**File:** `lib/metrics/metric_comfort.c`  
**Lines:** 38-44  
**Severity:** HIGH

**Issue:**
```c
int16_t temp_dev = abs16(temp_c10 - baseline->avg_temp_c10);
// ...
uint8_t temp_comfort = 100 - (uint8_t)(temp_dev / 3);
```

The subtraction `temp_c10 - baseline->avg_temp_c10` can overflow if the values are at opposite extremes:
- Example: `temp_c10 = 32767` (max int16_t), `avg_temp_c10 = -32768` (min int16_t)
- `32767 - (-32768) = 65535`, which wraps to `-1` in int16_t arithmetic

**Likelihood:** Low (temperatures are typically in range -40°C to +60°C, or -400 to +600 in temp_c10 units)

**Fix:**
Use 32-bit arithmetic for the subtraction:
```c
int32_t temp_diff = (int32_t)temp_c10 - (int32_t)baseline->avg_temp_c10;
int16_t temp_dev = (temp_diff < 0) ? (int16_t)(-temp_diff) : (int16_t)temp_diff;
if (temp_dev > 300) {  // Cap at 30°C deviation
    temp_comfort = 0;
} else {
    temp_comfort = 100 - (uint8_t)(temp_dev / 3);
}
```

---

## Medium Severity Issues (Recommended)

### 🟡 MEDIUM-1: No Validation of Sleep Data Indices

**File:** `lib/metrics/metric_sd.c`  
**Lines:** 28-31  
**Severity:** MEDIUM

**Issue:**
The circular buffer index calculation doesn't validate `sleep_data->write_index`:
```c
int night_idx = (sleep_data->write_index - 1 - i + 7) % 7;
```

If `write_index` is corrupted (> 7), the modulo operation still produces a valid index, but it may not be the intended one.

**Recommendation:**
Add defensive check:
```c
uint8_t metric_sd_compute(const circadian_data_t *sleep_data, uint8_t deficits[3]) {
    if (!sleep_data) {
        deficits[0] = deficits[1] = deficits[2] = 0;
        return 50;
    }
    
    // Validate write_index
    uint8_t write_idx = sleep_data->write_index % 7;
    
    // Calculate deficit for last 3 nights
    int16_t deficit[3];
    for (int i = 0; i < 3; i++) {
        int night_idx = (write_idx - 1 - i + 7) % 7;
        // ... rest of function
    }
}
```

---

### 🟡 MEDIUM-2: Insufficient Error Handling for Null Homebase

**File:** `lib/metrics/metric_comfort.c`  
**Lines:** 25-28  
**Severity:** MEDIUM

**Issue:**
When `baseline` is NULL, the function returns 50 (neutral score). However, this silently hides a potentially important error condition (missing homebase data).

**Recommendation:**
Consider logging or signaling this condition differently, or document that 50 is the expected behavior for missing homebase data.

---

## Positive Security Findings ✅

### BKUP Register Bounds Checking
- ✅ **PASS**: `movement_claim_backup_register()` returns 0 when no registers available
- ✅ **PASS**: `metrics_init()` checks for 0 return values before using registers
- ✅ **PASS**: `watch_store_backup_data()` and `watch_get_backup_data()` validate `reg < 8`

### State Machine Safety
- ✅ **PASS**: `metrics_init()` properly initializes all fields to safe defaults
- ✅ **PASS**: `metrics_update()` checks for NULL and `!initialized` before processing
- ✅ **PASS**: `metrics_set_wake_onset()` validates hour/minute ranges before storing

### Memory Safety
- ✅ **PASS**: No dynamic memory allocation (stack-only)
- ✅ **PASS**: Fixed-size arrays with bounds checking (`sd_deficits[3]`)
- ✅ **PASS**: No string operations or buffer copies

### Input Validation (Partial)
- ✅ **PASS**: `metric_em_compute()` clamps `hour >= 24`
- ✅ **PASS**: `metric_energy_compute()` clamps `hour >= 24`
- ✅ **PASS**: `metrics_set_wake_onset()` validates and clamps hour/minute
- ⚠️ **PARTIAL**: Missing validation for `phase_score` and `day_of_year` ranges

---

## Test Coverage Analysis

The test script (`test_metrics.sh`) covers:
- ✅ Boundary conditions (invalid hours, excessive times)
- ✅ Normal and fallback modes (with/without accelerometer)
- ✅ Circadian and lunar cycle variations
- ❌ **MISSING**: Overflow conditions (large `minutes_awake`, `day_of_year = 365`)
- ❌ **MISSING**: BKUP register persistence/restoration
- ❌ **MISSING**: Corrupted BKUP data scenarios

**Recommendation:** Add overflow test cases for the identified critical issues.

---

## Recommendations Summary

### Must Fix Before Merge (Critical)
1. ✅ Fix `minutes_awake * 100` overflow in `metric_wk.c` (use uint32_t)
2. ✅ Add `phase_score` validation in `metric_energy.c`
3. ✅ Fix `day_of_year * 1000` overflow in `metric_em.c` (use uint32_t)

### Should Fix Before Merge (High)
4. ✅ Add wake onset validation in `metrics_load_bkup()`
5. ✅ Fix temperature deviation overflow in `metric_comfort.c` (use int32_t)

### Recommended for Follow-up PR
6. Add test cases for overflow conditions
7. Add BKUP persistence tests
8. Consider adding runtime assertions for debug builds

---

## Security Approval

**Final Status:** ⚠️ **BLOCKED - REQUIRES FIXES**

This PR will be **approved for merge** once the 3 CRITICAL and 2 HIGH severity issues are addressed. The core architecture is sound, but the arithmetic overflow vulnerabilities pose real stability risks that must be fixed.

**Estimated Fix Time:** 30-60 minutes  
**Re-review Required:** Yes (after fixes applied)

---

## Reviewer Notes

- The integer-only math approach is excellent for embedded systems (no FPU dependency)
- BKUP register management is well-designed with proper bounds checking
- Consider adding a `METRICS_DEBUG` mode with runtime assertions for development
- Good separation of concerns (metrics.c orchestrates, metric_*.c implements)

**Next Steps:**
1. Developer addresses the 5 issues above
2. Update test script to cover overflow conditions
3. Re-run security review on updated code
4. Approve for merge

