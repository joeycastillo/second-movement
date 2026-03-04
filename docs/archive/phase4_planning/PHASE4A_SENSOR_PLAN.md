# Phase 4A: Sensor Integration — Implementation Plan

> **Date:** 2026-02-20  
> **Status:** Ready for implementation  
> **PRs:** #65 (LIS2DW12 motion), #66 (Lux averaging), #67 (Metric wiring)  
> **Total budget:** ~3 KB flash, ~40 bytes RAM, <4 µA power

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│                    movement.c                        │
│         _movement_handle_top_of_minute()             │
│              (existing 15-min metric tick)            │
│                        │                             │
│              sensors_update() ← PR #65/#66           │
│                   │        │                         │
│            ┌──────┘        └──────┐                  │
│            ▼                      ▼                  │
│     ┌─────────────┐     ┌──────────────┐            │
│     │ LIS2DW12    │     │ Lux ADC      │            │
│     │ (stationary │     │ (1/min poll)  │            │
│     │  detect)    │     │              │            │
│     └──────┬──────┘     └──────┬───────┘            │
│            │                   │                     │
│            ▼                   ▼                     │
│     ┌─────────────────────────────────┐             │
│     │      sensor_state_t (RAM)       │ ← PR #65    │
│     │  motion_active, motion_var,     │              │
│     │  lux_buffer[5], lux_avg        │              │
│     └──────────────┬──────────────────┘             │
│                    │                                 │
│              metrics_update() ← PR #67               │
│            ┌───────┼───────┐                         │
│            ▼       ▼       ▼                         │
│     metric_em  metric_wk  metric_comfort             │
│     (variance) (intensity) (lux_avg)                 │
└─────────────────────────────────────────────────────┘
```

**Key design decisions:**
- **Extend, don't replace** — `sensors.c/h` wraps existing `lis2dw.*` and `watch_adc.*`
- **All state in RAM** — no additional BKUP registers needed (sensor state is transient)
- **Called from existing 15-min tick** — no new timers or interrupts
- **Integer math only** — all computations use uint16/uint32

---

## Shared Data Structures (PR #65)

### `lib/phase/sensors.h` (NEW)

```c
#ifndef SENSORS_H_
#define SENSORS_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef PHASE_ENGINE_ENABLED

// Tuning constants
#define SENSOR_LUX_BUFFER_SIZE    5     // 5 samples = 5-min window at 1/min
#define SENSOR_MOTION_THRESHOLD   62    // mg (wake-on-motion)
#define SENSOR_INACTIVITY_MIN     15    // minutes until motion_active clears

// Motion variance tracking: 5-sample circular buffer of magnitude values
// Updated every 15 min from accelerometer (matches metric tick)
#define SENSOR_MOTION_BUFFER_SIZE 5

typedef struct {
    // --- Motion state (PR #65) ---
    bool     motion_active;                        // True if motion detected within inactivity window
    uint8_t  inactivity_minutes;                   // Minutes since last motion event (0-255)
    uint16_t motion_magnitude;                     // Latest |x|+|y|+|z| reading (raw ADC units)
    uint16_t motion_buffer[SENSOR_MOTION_BUFFER_SIZE]; // Circular buffer of magnitudes
    uint8_t  motion_buf_idx;                       // Next write index
    uint8_t  motion_buf_count;                     // Number of valid entries (0-5)
    uint16_t motion_variance;                      // Computed variance (scaled integer)
    uint16_t motion_intensity;                     // Smoothed intensity (0-1000 scale)
    
    // --- Lux state (PR #66) ---
    uint16_t lux_buffer[SENSOR_LUX_BUFFER_SIZE];   // Circular buffer of raw lux samples
    uint8_t  lux_buf_idx;                          // Next write index
    uint8_t  lux_buf_count;                        // Number of valid entries (0-5)
    uint16_t lux_avg;                              // 5-min rolling average (lux)
    
    // --- Flags ---
    bool     has_accelerometer;                    // Cached from movement_state.has_lis2dw
    bool     initialized;
} sensor_state_t;
// Size: ~34 bytes RAM

/**
 * Initialize sensor state. Call once at startup.
 * @param state  Zeroed sensor state struct
 * @param has_accel  True if LIS2DW12 is present (green board)
 */
void sensors_init(sensor_state_t *state, bool has_accel);

/**
 * Configure LIS2DW12 for stationary detect mode.
 * Sets 1.6 Hz ODR, LPMode1, 62 mg wake threshold, sleep-on.
 * Only acts if has_accelerometer is true.
 * @param state  Sensor state
 */
void sensors_configure_accel(sensor_state_t *state);

/**
 * Update all sensor readings. Called from movement.c metric tick (every 15 min).
 * - Reads accelerometer wake/sleep status + raw reading
 * - Reads lux from ADC
 * - Updates rolling buffers and computed values
 * @param state  Sensor state
 */
void sensors_update(sensor_state_t *state);

/**
 * Sample lux only (called every 1 min from top-of-minute handler).
 * Lighter-weight than full sensors_update().
 * @param state  Sensor state
 */
void sensors_sample_lux(sensor_state_t *state);

/**
 * Get current motion variance (for EM metric).
 * Returns variance of motion_buffer in scaled units.
 */
uint16_t sensors_get_motion_variance(const sensor_state_t *state);

/**
 * Get current motion intensity (for WK metric).
 * Returns smoothed magnitude (0-1000 scale).
 */
uint16_t sensors_get_motion_intensity(const sensor_state_t *state);

/**
 * Get current lux average (for Comfort metric).
 */
uint16_t sensors_get_lux_avg(const sensor_state_t *state);

/**
 * Check if motion is currently active.
 */
bool sensors_is_motion_active(const sensor_state_t *state);

#endif // PHASE_ENGINE_ENABLED
#endif // SENSORS_H_
```

---

## PR #65: LIS2DW12 Stationary Mode Integration

### Scope
- Create `lib/phase/sensors.c/h`
- Configure LIS2DW12 in stationary detect mode
- Track motion state and compute variance/intensity
- ~300 lines, ~1 KB flash

### Files

| File | Action | Description |
|------|--------|-------------|
| `lib/phase/sensors.h` | NEW | Sensor abstraction (shown above) |
| `lib/phase/sensors.c` | NEW | Implementation |
| `movement.c` | UPDATE | Add `#include "sensors.h"`, call `sensors_init()` in `app_init()`, call `sensors_update()` in metric tick |
| `movement.h` | UPDATE | Add `sensor_state_t sensors;` to `movement_state_t` |

### Algorithm: `sensors.c` Motion Functions

```c
// sensors_configure_accel():
//   if (!state->has_accelerometer) return;
//   lis2dw_set_mode(LIS2DW_MODE_LOW_POWER);
//   lis2dw_set_low_power_mode(LIS2DW_LP_MODE_1);     // 12-bit, lowest power
//   lis2dw_set_data_rate(LIS2DW_DATA_RATE_LOWEST);    // 1.6 Hz
//   lis2dw_set_low_noise_mode(false);
//   lis2dw_set_range(LIS2DW_RANGE_2_G);
//   lis2dw_configure_wakeup_threshold(1);              // 62 mg (1 LSB = 62.5 mg @ 2g/6-bit)
//   lis2dw_enable_sleep();                             // Enable sleep/wake detection
//   lis2dw_enable_stationary_motion_detection();       // Stationary detect mode
//   lis2dw_configure_int1(LIS2DW_CTRL4_INT1_WU);      // Wake-up on INT1

// sensors_update() — motion portion:
//   if (!state->has_accelerometer) {
//       state->motion_active = false;
//       state->motion_variance = 0;
//       state->motion_intensity = 0;
//       return;
//   }
//   
//   // Read wake/sleep status
//   uint8_t wake_src = lis2dw_get_wakeup_source();
//   bool is_awake = (wake_src & LIS2DW_WAKEUP_SRC_VAL_WU_IA) != 0;
//   bool is_sleeping = (wake_src & LIS2DW_WAKEUP_SRC_VAL_SLEEP_STATE_IA) != 0;
//   
//   if (is_awake) {
//       state->motion_active = true;
//       state->inactivity_minutes = 0;
//   } else if (is_sleeping) {
//       state->inactivity_minutes += 15;  // Called every 15 min
//       if (state->inactivity_minutes >= SENSOR_INACTIVITY_MIN) {
//           state->motion_active = false;
//       }
//   }
//   
//   // Read raw magnitude for variance tracking
//   lis2dw_reading_t raw = lis2dw_get_raw_reading();
//   uint16_t mag = abs16(raw.x) + abs16(raw.y) + abs16(raw.z);
//   
//   // Push to circular buffer
//   state->motion_buffer[state->motion_buf_idx] = mag;
//   state->motion_buf_idx = (state->motion_buf_idx + 1) % SENSOR_MOTION_BUFFER_SIZE;
//   if (state->motion_buf_count < SENSOR_MOTION_BUFFER_SIZE)
//       state->motion_buf_count++;
//   
//   state->motion_magnitude = mag;
//   state->motion_variance = _compute_variance(state->motion_buffer, state->motion_buf_count);
//   state->motion_intensity = _compute_intensity(mag, state->motion_intensity);
```

### Algorithm: Variance Computation (Integer Math)

```c
// _compute_variance(buffer, count):
//   if (count < 2) return 0;
//   
//   // Compute mean (uint32 to prevent overflow)
//   uint32_t sum = 0;
//   for (uint8_t i = 0; i < count; i++) sum += buffer[i];
//   uint16_t mean = (uint16_t)(sum / count);
//   
//   // Compute sum of squared deviations
//   uint32_t sq_sum = 0;
//   for (uint8_t i = 0; i < count; i++) {
//       int32_t diff = (int32_t)buffer[i] - (int32_t)mean;
//       sq_sum += (uint32_t)(diff * diff);
//   }
//   
//   // Return variance (divided by count, not count-1 for simplicity)
//   // Cap at UINT16_MAX
//   uint32_t var = sq_sum / count;
//   return (var > UINT16_MAX) ? UINT16_MAX : (uint16_t)var;

// _compute_intensity(current_mag, prev_smoothed):
//   // Exponential moving average: 75% old + 25% new
//   // Scale to 0-1000: raw magnitude at rest ~1000 (1g), walking ~2000
//   return (uint16_t)((prev_smoothed * 3 + current_mag) / 4);
```

### Integration Points

**In `movement.c` → `app_init()`:**
```c
#ifdef PHASE_ENGINE_ENABLED
    sensors_init(&movement_state.sensors, movement_state.has_lis2dw);
    sensors_configure_accel(&movement_state.sensors);
#endif
```

**In `movement.c` → `_movement_handle_top_of_minute()`, inside the `#ifdef PHASE_ENGINE_ENABLED` block, BEFORE `metrics_update()`:**
```c
    // Sample lux every minute (lightweight)
    sensors_sample_lux(&movement_state.sensors);
    
    // Full sensor update every 15 min (with metric tick)
    if (movement_state.metric_tick_count >= 15) {
        sensors_update(&movement_state.sensors);
        // ... existing metrics_update() call, now with real sensor data ...
    }
```

**In `movement.h` → `movement_state_t`:**
```c
#ifdef PHASE_ENGINE_ENABLED
    sensor_state_t sensors;  // +34 bytes RAM
#endif
```

### Questions Answered

1. **Where does sensor state live?** — RAM only (`sensor_state_t` in `movement_state`). Sensor state is transient; metrics engine already persists what matters to BKUP. On reboot, sensors re-initialize with fresh buffers.

2. **Motion variance tracking?** — 5-sample circular buffer of |x|+|y|+|z| magnitudes, updated every 15 min. Variance computed as sum-of-squared-deviations / count (integer math).

3. **Extend or replace `watch_accelerometer.c`?** — Neither exists (it was listed in error). We use `lis2dw.h/c` directly, which is the actual driver. `sensors.c` wraps it.

4. **Green board detection?** — `sensors_init()` receives `movement_state.has_lis2dw`. All accel functions check `state->has_accelerometer` and no-op gracefully.

5. **Test strategy:**
   - **62 mg threshold:** Walk for 2 min → `state->motion_active == true`. Sit 15 min → `motion_active == false`.
   - **Variance:** Steady walking = low variance. Walk-stop-walk = high variance.
   - **Power:** Measure with multimeter; expect ≤3 µA (2.75 µA accel + overhead).

### Power Budget
| Component | Current | Notes |
|-----------|---------|-------|
| LIS2DW12 1.6 Hz LP1 | 2.75 µA | Continuous, hardware-managed |
| I²C read (15 min) | ~0.01 µA avg | 6-byte read, <1 ms |
| **Total PR #65** | **~2.76 µA** | |

### Flash/RAM Budget
| Item | Size | Notes |
|------|------|-------|
| `sensors.c` code | ~800 bytes flash | Functions + variance math |
| `sensor_state_t` | ~34 bytes RAM | In `movement_state` |

---

## PR #66: Lux Sensor Integration

### Scope
- Add lux sampling to `sensors.c/h`
- 5-min rolling average via circular buffer
- Sampled every 1 min from existing top-of-minute handler
- ~150 lines added, ~0.5 KB flash

### Files

| File | Action | Description |
|------|--------|-------------|
| `lib/phase/sensors.c` | UPDATE | Add `sensors_sample_lux()` implementation |
| `lib/phase/sensors.h` | (Already defined in PR #65) | Lux state already in `sensor_state_t` |
| `movement.c` | UPDATE | Call `sensors_sample_lux()` every minute |

### Algorithm: Lux Sampling

```c
// sensors_sample_lux(state):
//   // Enable ADC briefly
//   watch_enable_adc();
//   
//   // Read ambient light from A2 pin (Sensor Watch light sensor)
//   // The exact pin depends on board variant; use HAL_GPIO_A2_pin()
//   uint16_t raw = watch_get_analog_pin_level(HAL_GPIO_A2_pin());
//   
//   // Disable ADC immediately to save power
//   watch_disable_adc();
//   
//   // Convert raw ADC to approximate lux
//   // Raw 0-65535 → roughly 0-10000 lux (calibration TBD during dogfooding)
//   // Simple linear mapping: lux = raw / 6 (gives ~0-10922 range)
//   uint16_t lux = raw / 6;
//   
//   // Push to circular buffer
//   state->lux_buffer[state->lux_buf_idx] = lux;
//   state->lux_buf_idx = (state->lux_buf_idx + 1) % SENSOR_LUX_BUFFER_SIZE;
//   if (state->lux_buf_count < SENSOR_LUX_BUFFER_SIZE)
//       state->lux_buf_count++;
//   
//   // Compute rolling average
//   uint32_t sum = 0;
//   for (uint8_t i = 0; i < state->lux_buf_count; i++)
//       sum += state->lux_buffer[i];
//   state->lux_avg = (uint16_t)(sum / state->lux_buf_count);
```

### Integration Point

**In `movement.c` → `_movement_handle_top_of_minute()`, early in the `#ifdef PHASE_ENGINE_ENABLED` block (runs every minute, not just every 15):**

```c
#ifdef PHASE_ENGINE_ENABLED
    // Sample lux every minute for 5-min rolling average
    sensors_sample_lux(&movement_state.sensors);
    
    // Existing 15-min metric tick follows...
    movement_state.metric_tick_count++;
    if (movement_state.metric_tick_count >= 15) {
        // ...
    }
#endif
```

### Questions Answered

1. **Rolling average structure?** — 5-entry `uint16_t` circular buffer (10 bytes). At 1 sample/min, fills in 5 min. Average = sum/count. Integer only.

2. **RTC alarm setup?** — No new alarm needed. We hook into the existing `_movement_handle_top_of_minute()` which fires every minute via the already-configured RTC compare callback.

3. **ADC integration?** — Use existing `watch_enable_adc()` / `watch_get_analog_pin_level()` / `watch_disable_adc()`. Enable briefly, read, disable. ~1 ms per sample.

4. **Sensor unavailable?** — If ADC read returns 0 or board lacks light sensor, `lux_avg` stays at 0. Comfort metric treats 0 lux as dark (applies nighttime logic). Acceptable fallback.

5. **Test strategy:**
   - Indoor (hand over sensor) → lux_avg ~50-200
   - Outdoor → lux_avg ~2000-10000
   - Cover sensor, wait 5 min → avg smoothly decays
   - Flash → avg resists single-sample spike (5-sample buffer)

### Power Budget
| Component | Current | Notes |
|-----------|---------|-------|
| ADC enable + read + disable | ~0.05 µA avg | ~50 µA for ~1 ms, once per 60s |
| **Total PR #66** | **~0.05 µA** | |

### Flash/RAM Budget
| Item | Size | Notes |
|------|------|-------|
| `sensors_sample_lux()` code | ~200 bytes flash | Short function |
| Lux buffer already in `sensor_state_t` | 10 bytes RAM | Part of PR #65 struct |

---

## PR #67: Metric Engine Sensor Wiring

### Scope
- Replace placeholder sensor values in `metrics_update()` with real sensor data
- Update individual metric compute functions to use variance/intensity
- Add simple anomaly detection (rolling mean + threshold)
- ~200 lines changed, ~1 KB flash

### Files

| File | Action | Description |
|------|--------|-------------|
| `movement.c` | UPDATE | Pass sensor values to `metrics_update()` |
| `lib/metrics/metrics.c` | UPDATE | Receive sensor state, pass to individual metrics |
| `lib/metrics/metrics.h` | UPDATE | Add `sensor_state_t*` parameter or individual fields |
| `lib/metrics/metric_em.c` | UPDATE | Use real `motion_variance` instead of placeholder |
| `lib/metrics/metric_wk.c` | UPDATE | Use `motion_intensity` for activity bonus |
| `lib/metrics/metric_comfort.c` | UPDATE | Use `lux_avg` instead of placeholder |

### Sensor → Metric Mapping

| Metric | Sensor Input | Field | How Used |
|--------|-------------|-------|----------|
| **Comfort** | Lux average | `sensors_get_lux_avg()` | Replace `light_lux` param (currently 0) |
| **EM** | Motion variance | `sensors_get_motion_variance()` | Replace `activity_variance` placeholder (currently 50) |
| **WK** | Motion intensity | `sensors_get_motion_intensity()` | Replace `cumulative_activity` with real value |
| **SD** | (unchanged) | sleep data | Already wired to circadian_data_t |
| **Energy** | (derived) | phase + SD + activity | `activity_level` now from motion_intensity |

### Changes to `movement.c`

In the metric tick block, replace placeholder values:

```c
// BEFORE (current):
uint16_t activity_level = 0;
uint16_t light_lux = 0;

// AFTER (PR #67):
uint16_t activity_level = sensors_get_motion_intensity(&movement_state.sensors);
uint16_t light_lux = sensors_get_lux_avg(&movement_state.sensors);

// Also update cumulative_activity tracking:
if (sensors_is_motion_active(&movement_state.sensors)) {
    movement_state.cumulative_activity += sensors_get_motion_intensity(&movement_state.sensors) / 100;
    if (movement_state.cumulative_activity > 65535) movement_state.cumulative_activity = 65535;
}
```

### Changes to `metric_em.c`

Replace the placeholder variance_score:

```c
// BEFORE:
(void)activity_variance;
uint8_t variance_score = 50;

// AFTER:
// Map variance (0-65535) to score (0-100)
// Low variance = low EM (stable), high variance = high EM (emotionally dynamic)
uint8_t variance_score;
if (activity_variance == 0) {
    variance_score = 30;  // Resting baseline (not zero — stillness has dignity)
} else if (activity_variance > 10000) {
    variance_score = 100;
} else {
    variance_score = 30 + (uint8_t)((uint32_t)activity_variance * 70 / 10000);
}
```

### Changes to `metric_wk.c`

The existing `cumulative_activity` parameter now receives real data (no code change needed in metric_wk.c itself — the improvement is in the caller providing real values).

### Changes to `metric_comfort.c`

No code changes needed. The function already accepts `light_lux` — the improvement is the caller now passes `sensors_get_lux_avg()` instead of 0.

### Anomaly Detection

Simple per-metric anomaly tracking using running mean + deviation threshold:

```c
// In metrics.h, add to metrics_engine_t:
typedef struct {
    uint16_t running_mean;   // Exponential moving average (×100 for precision)
    uint16_t running_var;    // Running variance estimate (×100)
    uint8_t  sample_count;   // Samples seen (caps at 255)
} metric_baseline_t;

// Add to metrics_engine_t:
metric_baseline_t baselines[5];  // One per metric (SD, EM, WK, Energy, Comfort)
// +30 bytes RAM
```

```c
// anomaly_update(baseline, new_value):
//   if (baseline->sample_count < 255) baseline->sample_count++;
//   
//   // Exponential moving average: α = 1/16 (shift-friendly)
//   // mean = mean + (new - mean) / 16
//   int32_t new_scaled = (int32_t)new_value * 100;
//   int32_t delta = new_scaled - (int32_t)baseline->running_mean;
//   baseline->running_mean += (int16_t)(delta / 16);
//   
//   // Running variance (Welford's, simplified):
//   // var = var + (|delta| - var) / 16
//   uint16_t abs_delta = (delta < 0) ? (uint16_t)(-delta) : (uint16_t)delta;
//   int32_t var_delta = (int32_t)abs_delta - (int32_t)baseline->running_var;
//   baseline->running_var += (int16_t)(var_delta / 16);

// anomaly_score(baseline, current_value):
//   // Returns 0-100: how anomalous is current value?
//   // 0 = exactly at baseline, 100 = very far from baseline
//   if (baseline->sample_count < 4) return 50;  // Not enough data
//   int32_t dev = abs((int32_t)current_value * 100 - (int32_t)baseline->running_mean);
//   if (baseline->running_var == 0) return (dev > 500) ? 100 : 50;
//   uint32_t z_score_x10 = (dev * 10) / baseline->running_var;
//   if (z_score_x10 > 30) return 100;  // >3 stddev
//   return (uint8_t)(z_score_x10 * 100 / 30);
```

**Usage:** Anomaly scores feed into the playlist surfacing weights (Phase 4B), not the metrics themselves. Metrics compute raw values; anomaly detection is metadata for surfacing priority.

### Questions Answered

1. **Which metrics need which inputs?** — See mapping table above. SD and Energy unchanged. Comfort gets lux_avg. EM gets motion_variance. WK gets real cumulative_activity.

2. **Anomaly detection?** — Per-metric running mean + variance using exponential moving average (α=1/16, shift-friendly). Window is effectively ~16 samples deep. Stored as `metric_baseline_t` (6 bytes × 5 metrics = 30 bytes).

3. **Baseline per metric or shared?** — Per metric. 30 bytes total is trivial for 32 KB RAM.

4. **Missing sensors?** — All `sensors_get_*()` functions return 0 if sensor unavailable. Metrics handle 0 gracefully: Comfort treats 0 lux as dark, EM uses 30 as resting variance baseline, WK falls back to time-only ramp.

5. **Test strategy:**
   - Walk outside → Comfort should drop (bright), EM should rise (variance), WK activity bonus triggers
   - Sit still indoors → Comfort stable (~90 at 500 lux), EM settles (~50), WK ramps on time only
   - Compare metric values before/after PR: should see movement instead of flat 50s

### Power Budget
| Component | Current | Notes |
|-----------|---------|-------|
| Computation | ~0 µA | Integer math in existing wake cycle |
| **Total PR #67** | **~0 µA** | |

### Flash/RAM Budget
| Item | Size | Notes |
|------|------|-------|
| EM variance mapping | ~100 bytes flash | Simple scaling |
| Anomaly tracking | ~400 bytes flash | Update + score functions |
| `metric_baseline_t[5]` | 30 bytes RAM | Running stats |
| movement.c wiring changes | ~200 bytes flash | Glue code |

---

## Integration Sequence

### PR Ordering (must be sequential)

1. **PR #65** first — creates `sensors.h/c`, defines `sensor_state_t`, configures accelerometer
2. **PR #66** second — adds lux sampling to existing `sensors.c`
3. **PR #67** third — wires sensors → metrics (depends on both #65 and #66)

### Total Resource Budget

| Resource | PR #65 | PR #66 | PR #67 | Total |
|----------|--------|--------|--------|-------|
| Flash | ~800 B | ~200 B | ~700 B | ~1.7 KB |
| RAM | ~34 B | (included) | ~30 B | ~64 B |
| Power | 2.76 µA | 0.05 µA | 0 µA | ~2.8 µA |
| BKUP regs | 0 | 0 | 0 | 0 |

**Well within budgets:** 3 KB flash target, 10 µA power target.

---

## Test Plan

### Unit Tests (Manual on Hardware)

**T1: Accelerometer Configuration (PR #65)**
- Flash firmware to green board
- Check `lis2dw_get_data_rate() == LIS2DW_DATA_RATE_LOWEST`
- Check `lis2dw_get_wakeup_threshold() == 1` (62.5 mg)
- Verify: `sensors_is_motion_active()` returns true when waving hand

**T2: Motion Inactivity Timeout (PR #65)**
- Walk for 2 min → `motion_active == true`
- Set watch on table for 15 min → `motion_active == false`
- Pick up and walk → `motion_active == true` within one tick

**T3: Lux Rolling Average (PR #66)**
- Cover light sensor → lux_avg drops over 5 samples (5 min)
- Expose to lamp → lux_avg rises over 5 min
- Single flash (1 sample) → avg barely moves (1/5 weight)

**T4: Metric Values Under Real Conditions (PR #67)**
- Print metric snapshot via USB serial
- Walk outside: Comfort < 60 (bright), EM > 60 (variance up), WK > 50 (activity)
- Sit indoors: Comfort > 70, EM ~50, WK ramps with time
- Compare to Phase 3 (all metrics were ~50): values should now vary meaningfully

### Power Validation

**P1: Current Measurement**
- Use µCurrent Gold or equivalent between battery and board
- Measure baseline (Phase 3, no Phase 4A): record µA
- Flash Phase 4A firmware: record µA
- Delta should be <3 µA (primarily LIS2DW12 2.75 µA)

**P2: No Continuous Polling**
- Verify no I²C bus activity between 15-min ticks (oscilloscope on SDA/SCL)
- Verify ADC only active during lux sample (~1 ms/min)

---

## Open Questions for dlorp — ANSWERED

1. **Light sensor ADC pin:** ✅ **ANSWERED:** Only Pro board has integrated light sensor. Green/red/blue boards have NO light sensor. Use build-time detection (`#ifdef BOARD_SENSORWATCH_PRO`).

2. **Raw-to-lux calibration:** ✅ **ANSWERED:** No reference data needed - Pro board only. Use `raw / 6` as initial mapping, tune during dogfooding if needed.

3. **Accelerometer wake threshold LSB:** ✅ **ANSWERED:** Unsure - will test empirically during PR #65 implementation. Start with `threshold = 1`, validate with real motion.

4. **Sleep mode interaction:** ✅ **ANSWERED:** DOES conflict. Solution: Phase engine accel config guards with `!is_sleep_window()` check. During sleep window, sleep tracker owns accelerometer (400 Hz tap detection). Outside sleep window, phase engine uses 1.6 Hz stationary mode.

5. **Anomaly detection scope:** ✅ **ANSWERED:** Defer to Phase 4B. PR #67 just wires sensors → metrics, no baseline tracking yet.

---

## Risks & Mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Accel config conflicts with sleep tracker | Medium | High | Guard with `!is_sleep_window()` check; restore sleep config when entering sleep window |
| Lux sensor not present on all boards | Low | Low | `lux_avg` stays 0; Comfort uses temperature-only fallback |
| Motion variance too noisy | Medium | Low | Increase buffer size from 5→10 if needed (10 more bytes RAM) |
| ADC wake costs more power than estimated | Low | Low | Already measured at <0.1 µA avg; worst case disable lux sampling |

---

**Ready for implementation.** Start with PR #65 (accelerometer), then #66 (lux), then #67 (wiring).
