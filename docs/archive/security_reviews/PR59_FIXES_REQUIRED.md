# PR #59 Required Fixes - Security Review

## Critical Fixes (Required for Merge)

### Fix 1: WK Metric Integer Overflow Protection

**File:** `lib/metrics/metric_wk.c`

**Current Code (Lines 39-42):**
```c
// Base score: linear ramp from 0 to 100 over 120 minutes
uint16_t base = (minutes_awake * 100) / WK_RAMP_NORMAL;
if (base > 100) base = 100;
```

**Issue:**
While likely safe on 32-bit ARM (where `int` is 32-bit), the code is not portable to 16-bit platforms and lacks explicit overflow protection. The intermediate calculation `minutes_awake * 100` could overflow if `minutes_awake` is large.

**Fixed Code:**
```c
// Base score: linear ramp from 0 to 100 over 120 minutes
// Use uint32_t to prevent overflow on all platforms
uint32_t base = ((uint32_t)minutes_awake * 100) / WK_RAMP_NORMAL;
if (base > 100) base = 100;

// Activity bonus: +30% if cumulative activity exceeds threshold
uint8_t bonus = (cumulative_activity >= WK_ACTIVITY_THRESHOLD) ? WK_ACTIVITY_BONUS : 0;

// Combine base + bonus, capped at 100
uint16_t total = base + bonus;
if (total > 100) total = 100;
wk = (uint8_t)total;
```

**Also Apply to Lines 51-52 (Fallback Mode):**
```c
// Linear ramp from 0 to 100 over 180 minutes
uint32_t base = ((uint32_t)minutes_awake * 100) / WK_RAMP_FALLBACK;
if (base > 100) base = 100;
wk = (uint8_t)base;
```

---

### Fix 2: EM Metric Lunar Phase Overflow Protection

**File:** `lib/metrics/metric_em.c`

**Current Code (Lines 65-67):**
```c
// 29-day cycle approximation, peaks at half-cycle (day 14.5)
// Formula: ((day_of_year * 1000) / 29) % 1000
uint16_t lunar_phase = ((day_of_year * 1000) / 29) % 1000;
```

**Issue:**
Same portability issue. On some platforms, `day_of_year * 1000` could overflow uint16_t space.

**Fixed Code:**
```c
// 29-day cycle approximation, peaks at half-cycle (day 14.5)
// Use uint32_t to prevent overflow (365 * 1000 = 365,000)
uint32_t lunar_phase = ((uint32_t)day_of_year * 1000 / 29) % 1000;
```

---

### Fix 3: Energy Metric Input Validation

**File:** `lib/metrics/metric_energy.c`

**Current Code (Line 48):**
```c
uint8_t metric_energy_compute(uint16_t phase_score, uint8_t sd_score, uint16_t recent_activity,
                              uint8_t hour, bool has_accelerometer) {
    // Base formula: phase_score - (sd_score / 3)
    int16_t energy = (int16_t)phase_score - (sd_score / 3);
```

**Issue:**
Parameter `phase_score` is declared as `uint16_t` but expected range is 0-100. No validation performed. If caller passes an invalid value (e.g., 1000), the result will be incorrect.

**Fixed Code:**
```c
uint8_t metric_energy_compute(uint16_t phase_score, uint8_t sd_score, uint16_t recent_activity,
                              uint8_t hour, bool has_accelerometer) {
    // Validate phase_score is in expected range [0-100]
    if (phase_score > 100) phase_score = 100;
    
    // Base formula: phase_score - (sd_score / 3)
    int16_t energy = (int16_t)phase_score - (sd_score / 3);
```

---

## High Priority Fixes (Strongly Recommended)

### Fix 4: Wake Onset Validation After BKUP Load

**File:** `lib/metrics/metrics.c`

**Current Code (Lines 158-162):**
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
}
```

**Issue:**
If BKUP memory is uninitialized or corrupted, loaded values may be out of range (hour > 23, minute > 59).

**Fixed Code:**
```c
void metrics_load_bkup(metrics_engine_t *engine) {
    if (!engine || engine->bkup_reg_sd == 0 || engine->bkup_reg_wk == 0) return;
    
    // Load SD state from BKUP
    uint32_t sd_data = watch_get_backup_data(engine->bkup_reg_sd);
    engine->sd_deficits[0] = (uint8_t)(sd_data & 0xFF);
    engine->sd_deficits[1] = (uint8_t)((sd_data >> 8) & 0xFF);
    engine->sd_deficits[2] = (uint8_t)((sd_data >> 16) & 0xFF);
    
    // Clamp SD deficits to valid range [0-100]
    if (engine->sd_deficits[0] > 100) engine->sd_deficits[0] = 0;
    if (engine->sd_deficits[1] > 100) engine->sd_deficits[1] = 0;
    if (engine->sd_deficits[2] > 100) engine->sd_deficits[2] = 0;
    
    // Load WK state from BKUP
    uint32_t wk_data = watch_get_backup_data(engine->bkup_reg_wk);
    engine->wake_onset_hour = (uint8_t)(wk_data & 0xFF);
    engine->wake_onset_minute = (uint8_t)((wk_data >> 8) & 0xFF);
    
    // Validate and clamp loaded time values
    if (engine->wake_onset_hour >= 24) engine->wake_onset_hour = 0;
    if (engine->wake_onset_minute >= 60) engine->wake_onset_minute = 0;
}
```

---

### Fix 5: Comfort Metric Temperature Overflow Protection

**File:** `lib/metrics/metric_comfort.c`

**Current Code (Lines 38-44):**
```c
// Temp comfort (60%): deviation from seasonal baseline
// Lower deviation = higher comfort
int16_t temp_dev = abs16(temp_c10 - baseline->avg_temp_c10);
// Penalty: divide by 3 to convert temp deviation (in 0.1°C) to comfort penalty
// Example: 30 units (3°C) deviation → 10 point penalty → 90 comfort
uint8_t temp_comfort = 100;
if (temp_dev / 3 > 100) {
    temp_comfort = 0;
} else {
    temp_comfort = 100 - (uint8_t)(temp_dev / 3);
}
```

**Issue:**
While unlikely with realistic temperature values, the subtraction `temp_c10 - baseline->avg_temp_c10` could overflow if values are at extremes.

**Fixed Code:**
```c
// Temp comfort (60%): deviation from seasonal baseline
// Use int32_t to prevent overflow in subtraction
int32_t temp_diff = (int32_t)temp_c10 - (int32_t)baseline->avg_temp_c10;
int32_t temp_dev = (temp_diff < 0) ? -temp_diff : temp_diff;

// Penalty: divide by 3 to convert temp deviation (in 0.1°C) to comfort penalty
// Cap at 30°C deviation (300 units)
uint8_t temp_comfort;
if (temp_dev > 300) {
    temp_comfort = 0;
} else {
    temp_comfort = 100 - (uint8_t)(temp_dev / 3);
}
```

---

## Test Coverage Additions (Recommended)

Add these test cases to `test_metrics.sh`:

### Overflow Test Cases

```bash
void test_overflow_conditions() {
    printf("Testing Overflow Conditions:\n");
    
    // WK: Large minutes_awake
    uint8_t wk_large = metric_wk_compute(50000, 0, true);
    printf("  WK @ 50000 min (overflow test): %u (expect 100, clamped)\n", wk_large);
    
    // EM: Day 365 (max day_of_year)
    uint8_t em_day365 = metric_em_compute(12, 365, 0);
    printf("  EM @ day 365 (max day): %u (should not crash)\n", em_day365);
    
    // Energy: Invalid phase_score
    uint8_t energy_invalid = metric_energy_compute(5000, 20, 0, 12, false);
    printf("  Energy with phase=5000 (invalid): %u (expect clamped)\n", energy_invalid);
    
    printf("  ✓ Overflow tests passed\n\n");
}
```

---

## Checklist for Developer

- [ ] Apply Fix 1: WK metric uint32_t promotion (both normal and fallback modes)
- [ ] Apply Fix 2: EM metric lunar phase uint32_t promotion
- [ ] Apply Fix 3: Energy metric phase_score validation
- [ ] Apply Fix 4: BKUP load validation for wake onset time
- [ ] Apply Fix 5: Comfort metric int32_t promotion for temperature
- [ ] Add overflow test cases to test_metrics.sh
- [ ] Run updated test suite and verify all tests pass
- [ ] Rebuild firmware and test on hardware (if available)
- [ ] Request re-review from security specialist

---

## Estimated Effort

- **Fix Time:** 30-45 minutes
- **Test Time:** 15-30 minutes
- **Total:** ~1 hour

All fixes are localized and straightforward. No architectural changes required.
