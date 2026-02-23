# Dogfooding Integration Checklist

**Status:** IN PROGRESS  
**Goal:** Complete all integration points before flashing to hardware for dogfooding

---

## Critical Integration Points

### 1. ✅ Fix Build Error (COMPLETED)
- [x] Fix `light_sensor_face` undeclared error in movement_config.h line 76
- [x] Guard with `#ifdef HAS_IR_SENSOR` or remove if not available

### 2. ⏳ Sleep Window Detection (IN PROGRESS)
**File:** `movement/movement.c`  
**Task:** Wire Phase 4E sleep tracking to existing Cole-Kripke sleep detection
**Integration:**
- When Cole-Kripke detects sleep onset, call `sleep_data_start_tracking()`
- Every 30 seconds during sleep, call `sleep_data_record_epoch(movement_count)`
- When Cole-Kripke detects wake, call `sleep_data_stop_tracking()`
**Status:** NOT WIRED YET (manual epoch recording only)

### 3. ⏳ Hourly Telemetry Accumulation (IN PROGRESS)
**File:** `movement/movement.c` or `lib/phase/phase_engine.c`  
**Task:** Wire telemetry to Phase Engine hourly tick
**Integration:**
- On hourly update (RTC alarm or Phase Engine tick):
  - Call `sleep_data_accumulate_telemetry(zone, metrics, battery_mv, light_lux, movement_count)`
  - Populate zone transitions, dominant metric, light exposure, battery voltage
**Status:** NOT WIRED YET (function exists but not called)

### 4. ⏳ Dominant Metric Calculation (IN PROGRESS)
**File:** `lib/metrics/metrics.c` (new function)  
**Task:** Add helper to determine which metric drives current zone
**Signature:**
```c
uint8_t metrics_get_dominant(metrics_snapshot_t* snapshot, phase_zone_t zone);
// Returns: 0=SD, 1=EM, 2=WK, 3=Comfort
```
**Logic:**
- For each metric, calculate deviation from baseline/expected
- Return metric with highest deviation (most influential)
**Status:** NOT IMPLEMENTED YET

### 5. ⏳ Epoch Tracking (IN PROGRESS)
**File:** `movement/movement.c`  
**Task:** Ensure `sensors_tick_epoch()` called every second
**Integration:**
- In movement main loop, call `sensors_tick_epoch()` on RTC tick
- This tracks light exposure duration per hour (critical for EM metric)
**Status:** VERIFY IF WIRED

### 6. ✅ Active Hours Integration (COMPLETED)
**File:** `lib/phase/playlist.c`  
**Task:** Sleep mode enforcement outside active hours
**Status:** DONE (PR 1 commit a29d647)

### 7. ⏳ Config Defaults Verification (IN PROGRESS)
**Task:** Verify Phase 4F defaults work for Alaska climate
**Defaults:**
- Outdoor lux: 500 (may need 200 for Alaska winter)
- Daylight hours: 6-18 (may need 10-16 for Alaska winter)
**Status:** Using hardcoded defaults (Builder UI can be deferred)

---

## Build Verification

### 8. ⏳ Full Firmware Build (IN PROGRESS)
**Command:** `make BOARD=sensorwatch_pro DISPLAY=classic`
**Verify:**
- [ ] Build succeeds (no errors/warnings)
- [ ] Flash size <150 KB
- [ ] RAM usage <6 KB
- [ ] All board variants build (pro, blue, green, red)
- [ ] All display variants build (classic, custom)

---

## Pre-Flash Checklist

Before flashing to hardware:
- [ ] All 8 integration points complete
- [ ] Full firmware build passes
- [ ] Flash/RAM budgets met
- [ ] Unit tests pass
- [ ] Security review issues resolved
- [ ] PR updated with final commits

---

## Integration Agent Instructions

Read this checklist and complete items 2-5 + 7-8 before reporting completion.
dlorp wants to flash and start dogfooding, so ALL functionality must be connected.
