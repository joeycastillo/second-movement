# Phase 3: Chronomantic System — Full Implementation Plan

> **Status:** Planning  
> **Depends on:** Phase 1 (Scaffolding ✅), Phase 2 (Phase Engine ✅)  
> **Goal:** Implement the complete chronomantic instrument: Metric Engine, Weighted Playlist Controller, zone-specific faces, and Builder UI integration.  
> **Design authority:** Phase Watch Overhaul Plan (dlorp, Feb 2026)

---

## 0. Design Philosophy Recap

The watch is a **chronomantic instrument** — it measures the wearer's biological cycle relative to nature, not clock time. Three axes define every computation:

| Axis | Inputs | Source |
|------|--------|--------|
| **Human** | activity, stillness, wake momentum, behavioral rhythm | LIS2DW12 accelerometer, sleep data |
| **Environment** | ambient light, temperature, motion variance | Light sensor, thermistor, accel variance |
| **Season** | day-of-year, expected daylight, climate norms | Homebase table (flash LUT) |

**Phase** is a continuous weighted field (0–100), not rigid clock zones.  
**Complications** surface dynamically based on weighted relevance per zone.

---

## 1. Metric Engine Architecture

### Overview

Six metrics, each producing a score 0–100, updated at different cadences. All integer math (no FPU on SAM L22).

```
Sensor Readings + Homebase Table + Sleep History + Active Hours
                        ↓
                  Metric Engine
         ┌──────┬──────┬──────┬──────┬──────┬──────┐
         │  SD  │  EM  │  WK  │ NRG  │  JL  │ CMF  │
         │0-100 │0-100 │0-100 │0-100 │0-100 │0-100 │
         └──────┴──────┴──────┴──────┴──────┴──────┘
```

### 1.1 SD — Sleep Debt

| Property | Value |
|----------|-------|
| **Range** | 0 = fully rested, 100 = exhausted |
| **Update** | Once per minute during Emergence zone; once per hour otherwise |
| **Inputs** | `circadian_sleep_night_t.duration_min` (7-night buffer), active hours config |
| **Algorithm** | Target = 480 min (8h). Deficit per night = max(0, 480 − actual). SD = rolling 3-night weighted deficit: `(deficit[0]*50 + deficit[1]*30 + deficit[2]*20) / 100`. Clamp 0–100. |
| **Storage** | 3 bytes in BKUP register (3 × deficit/4, packed as uint8_t each) |
| **Source module** | `lib/circadian_score.h` (existing `circadian_data_t.nights[]`) |

### 1.2 EM — Emotional / Circadian Mood

| Property | Value |
|----------|-------|
| **Range** | 0 = low/disrupted, 100 = aligned/elevated |
| **Update** | Every 15 minutes |
| **Inputs** | Hour-of-day, day-of-year (lunar approximation), activity variance (accel σ over 15 min) |
| **Algorithm** | Three sub-scores blended: |
| | **Circadian** (40%): `cosine_lut_24[hour]` mapped to 0–100 (already exists in phase_engine.c) |
| | **Lunar** (20%): `lunar_phase = ((day_of_year * 1000) / 29) % 1000`. Score peaks near full moon (500): `100 - abs(lunar_phase - 500) / 5` |
| | **Variance** (40%): Activity variance over last 15 min. Low variance during expected-active = penalty; high variance during expected-rest = penalty. Compare against zone expectation. |
| **Storage** | 1 byte in RAM (current score only; no persistence needed) |

### 1.3 WK — Wake Momentum

| Property | Value |
|----------|-------|
| **Range** | 0 = just woke, 100 = fully ramped |
| **Update** | Every minute for first 2 hours after wake; every 15 min after |
| **Inputs** | Minutes since wake onset, cumulative activity since wake |
| **Algorithm** | Base ramp: `min(100, minutes_awake * 100 / 120)` (full ramp in 2h). Activity bonus: if cumulative activity > threshold, accelerate ramp by up to 30%. Activity penalty: if sedentary for >30 min after wake, cap at 60. |
| **Storage** | 2 bytes: wake_onset_hour (uint8_t) + wake_onset_minute (uint8_t) in BKUP. Score is computed, not stored. |
| **Wake detection** | Transition out of sleep window (Active Hours) + first significant motion event |

### 1.4 Energy

| Property | Value |
|----------|-------|
| **Range** | 0 = depleted, 100 = peak |
| **Update** | Every 15 minutes |
| **Inputs** | Current activity level, phase score, SD |
| **Algorithm** | `Energy = phase_score - (SD / 3) + activity_bonus`. Activity bonus = `min(20, recent_activity / 50)`. Clamp 0–100. Essentially: how much capacity you have right now given alignment and rest. |
| **Storage** | None (derived from other metrics + phase score) |

### 1.5 JL — Jet Lag

| Property | Value |
|----------|-------|
| **Range** | 0 = no shift, 100 = maximum disruption |
| **Update** | Once per hour |
| **Inputs** | `timezone_offset_delta` (set via comms sync or manual), hours since shift |
| **Algorithm** | On timezone change: `JL = abs(tz_delta_hours) * 12` (capped at 100). Decay: `JL -= max(1, JL / 24)` per hour (roughly 1 day per hour of shift). |
| **Storage** | 2 bytes in BKUP: `jl_score` (uint8_t) + `jl_hours_remaining` (uint8_t) |
| **Trigger** | Timezone change detected via comms sync. No GPS — user sets homebase timezone, comms can push travel timezone. |

### 1.6 Comfort

| Property | Value |
|----------|-------|
| **Range** | 0 = extreme deviation, 100 = seasonal norm |
| **Update** | Every 15 minutes |
| **Inputs** | Current temp (thermistor), current light (light sensor), homebase entry for today |
| **Algorithm** | Two sub-scores: |
| | **Temp comfort** (60%): `100 - min(100, abs(temp_c10 - homebase.avg_temp_c10) / 3)` |
| | **Light comfort** (40%): Compare light_lux against expected for hour. Day (6–18): expect 200+ lux, penalty for <50. Night: expect <50, penalty for >200. |
| **Storage** | None (derived from sensors + homebase) |

### 1.7 Metric Update Summary

| Metric | Cadence | RAM (bytes) | BKUP (bytes) | Inputs |
|--------|---------|-------------|--------------|--------|
| SD | 1/min or 1/hr | 1 | 3 | sleep history |
| EM | 1/15min | 1 | 0 | hour, day, accel variance |
| WK | 1/min → 1/15min | 1 | 2 | wake time, activity |
| Energy | 1/15min | 1 | 0 | phase, SD, activity |
| JL | 1/hr | 1 | 2 | tz delta |
| Comfort | 1/15min | 1 | 0 | temp, light, homebase |
| **Total** | | **6** | **7** | |

BKUP usage: 7 bytes = fits in 2 BKUP registers (8 bytes). Claim via `movement_claim_backup_register()` × 2.

---

## 2. Weighted Playlist Controller

### 2.1 Zone Determination

Phase score (0–100) from `phase_compute()` maps to four zones:

| Zone | Phase Range | Character | Primary Time |
|------|-------------|-----------|--------------|
| **Emergence** | 0–25 | Waking, orienting | Early morning |
| **Momentum** | 26–50 | Building energy | Late morning |
| **Active** | 51–75 | Peak output | Midday–afternoon |
| **Descent** | 76–100 | Winding down | Evening–night |

**Note:** Phase is NOT clock time. A night-shift worker's Emergence might be at 8 PM. The zone boundaries are configurable:

```c
typedef struct {
    uint8_t emergence_max;   // default 25
    uint8_t momentum_max;    // default 50
    uint8_t active_max;      // default 75
    // descent = everything above active_max
} zone_thresholds_t;
```

### 2.2 Per-Zone Metric Weights

Each zone has a weight vector (uint8_t weights summing to 100) determining which metrics surface:

```c
// Weight tables (stored in flash as const)
//                    SD   EM   WK   NRG  JL   CMF
// Emergence:        [30,  25,  5,   10,  10,  20]  → Sleep debt & comfort matter most
// Momentum:         [20,  20,  30,  10,  10,  10]  → Wake momentum is key
// Active:           [15,  20,  5,   40,  10,  10]  → Energy dominates
// Descent:          [10,  25,  0,   10,  25,  30]  → JL & comfort for wind-down

static const uint8_t zone_weights[4][6] = {
    {30, 25,  5, 10, 10, 20},  // EMERGENCE
    {20, 20, 30, 10, 10, 10},  // MOMENTUM
    {15, 20,  5, 40, 10, 10},  // ACTIVE
    {10, 25,  0, 10, 25, 30},  // DESCENT
};
```

### 2.3 Weighted Relevance Score

For each metric in the current zone, compute **relevance**:

```
relevance[i] = weight[zone][i] * abs(metric[i] - 50) / 50
```

Metrics near 50 (neutral) have low relevance regardless of weight. Metrics at extremes (near 0 or 100) surface strongly. This implements the "complications surface dynamically based on weighted relevance" principle.

### 2.4 Face Rotation Logic

The playlist controller manages a **sorted list of faces** per zone, ordered by relevance:

```c
typedef struct {
    uint8_t zone;                    // Current zone (0-3)
    uint8_t face_count;              // Faces in current rotation
    uint8_t face_indices[6];         // Sorted by relevance (max 6 = one per metric)
    uint8_t current_face;            // Index into face_indices
    uint16_t dwell_ticks;            // Ticks on current face
    uint16_t dwell_limit;            // Auto-advance threshold
} playlist_state_t;
```

**Rotation rules:**
- Top 3 most relevant metrics get faces in the rotation
- Metrics with relevance < 10 are excluded (nothing interesting to show)
- Dwell time proportional to relevance (more relevant = longer display)
- ALARM button cycles through rotation manually
- Auto-advance every 30 seconds (configurable) if no interaction

### 2.5 Zone Transitions

**Hysteresis:** Zone change requires phase score to be in new zone for **3 consecutive readings** (3 × 15 min = 45 min). Prevents flickering at boundaries.

```c
typedef struct {
    uint8_t pending_zone;
    uint8_t consecutive_count;  // 0-3, trigger at 3
} zone_transition_t;
```

On zone change:
1. Recompute all metric relevances with new zone weights
2. Rebuild face rotation list
3. Reset to face[0] (highest relevance in new zone)
4. Brief indicator on display: zone glyph for 2 seconds

---

## 3. Phase 3 Work Breakdown

### 3A: Metric Engine (`lib/metrics/`)

**Scope:** Implement all 6 metrics as a cohesive module.

**Files:**
```
lib/metrics/
├── metrics.h              # Public API: metrics_state_t, metrics_update(), metrics_get()
├── metrics.c              # Core update loop, orchestration
├── metric_sd.h / .c       # Sleep Debt computation
├── metric_em.h / .c       # Emotional/circadian mood
├── metric_wk.h / .c       # Wake Momentum
├── metric_energy.h / .c   # Energy (thin — derives from phase + SD + activity)
├── metric_jl.h / .c       # Jet Lag
└── metric_comfort.h / .c  # Comfort (temp/light vs homebase)
```

**Public API:**
```c
typedef struct {
    uint8_t sd;       // Sleep Debt 0-100
    uint8_t em;       // Emotional 0-100
    uint8_t wk;       // Wake Momentum 0-100
    uint8_t energy;   // Energy 0-100
    uint8_t jl;       // Jet Lag 0-100
    uint8_t comfort;  // Comfort 0-100
} metrics_snapshot_t;

typedef struct {
    // Internal state for each metric
    // ... (private, ~32 bytes total)
} metrics_engine_t;

void metrics_init(metrics_engine_t *engine);
void metrics_update(metrics_engine_t *engine,
                    uint8_t hour, uint16_t day_of_year,
                    uint16_t activity_level, int16_t temp_c10,
                    uint16_t light_lux, uint16_t minutes_awake,
                    const circadian_data_t *sleep_data);
void metrics_get(const metrics_engine_t *engine, metrics_snapshot_t *out);
void metrics_save_bkup(const metrics_engine_t *engine);
void metrics_load_bkup(metrics_engine_t *engine);
```

**Guard:** `#ifdef PHASE_ENGINE_ENABLED` (reuse existing flag — metrics require phase engine)

### 3B: Weighted Playlist Controller

**Scope:** Zone determination, weight-based face selection, rotation, transitions.

**Files:**
```
lib/phase/
├── playlist.h             # Public API: playlist_state_t, playlist_update(), playlist_get_face()
└── playlist.c             # Zone logic, weight computation, face rotation
```

Lives in `lib/phase/` because it's tightly coupled to the phase engine output.

**Integration point:** Called from `movement.c` tick handler when `PHASE_ENGINE_ENABLED`:
```c
#ifdef PHASE_ENGINE_ENABLED
    if (phase_playlist_active) {
        playlist_update(&playlist, phase_score, &metrics);
        uint8_t face_idx = playlist_get_current_face(&playlist);
        // dispatch to face
    }
#endif
```

### 3C: Zone-Specific Faces

Four new faces, one per zone. Each is a **complication face** displaying the zone's priority metrics:

```
watch-faces/complication/
├── emergence_face.h / .c    # SD, EM, Lunar display
├── momentum_face.h / .c     # WK progress bar, SD, temperature
├── active_face.h / .c       # Energy gauge, SD, EM
└── descent_face.h / .c      # JL indicator, EM, Comfort
```

**Display strategy** (7-segment constraints):
- Top line: Zone indicator (2 chars) + primary metric value
- Bottom line: Secondary metric + trend arrow

Example for Emergence face:
```
EM  SD 72
   ↑EM45
```

Each face:
- Receives `metrics_snapshot_t` from the engine
- Displays top 2-3 metrics for its zone
- Handles ALARM button to cycle to next metric detail
- Handles LIGHT button for backlight (standard)
- ~800 bytes flash each (estimated)

### 3D: Builder UI Updates

**Scope:** Add zone configuration to the web-based firmware builder.

**Files:**
```
builder/
├── index.html             # Add zone configuration section
├── js/phase-config.js     # New: zone threshold sliders, weight editors
└── css/phase-config.css   # New: zone config styling
```

**Features:**
- Zone threshold sliders (Emergence/Momentum/Active/Descent boundaries)
- Per-zone weight adjustment (advanced toggle, hidden by default)
- Preview: "At phase score X, you'd be in zone Y with these priorities"
- Generates `#define` values for zone thresholds into build config
- Default values work for 90% of users — advanced config is optional

---

## 4. File/Module Structure

### 4.1 New Files

| File | Size Est. | Purpose |
|------|-----------|---------|
| `lib/metrics/metrics.h` | 400 B | Public metric engine API |
| `lib/metrics/metrics.c` | 1.5 KB | Orchestration, update scheduling |
| `lib/metrics/metric_sd.c` | 600 B | Sleep Debt |
| `lib/metrics/metric_sd.h` | 200 B | SD interface |
| `lib/metrics/metric_em.c` | 800 B | Emotional (most complex sub-scores) |
| `lib/metrics/metric_em.h` | 200 B | EM interface |
| `lib/metrics/metric_wk.c` | 500 B | Wake Momentum |
| `lib/metrics/metric_wk.h` | 200 B | WK interface |
| `lib/metrics/metric_energy.c` | 300 B | Energy (thin derivation) |
| `lib/metrics/metric_energy.h` | 200 B | Energy interface |
| `lib/metrics/metric_jl.c` | 400 B | Jet Lag |
| `lib/metrics/metric_jl.h` | 200 B | JL interface |
| `lib/metrics/metric_comfort.c` | 500 B | Comfort |
| `lib/metrics/metric_comfort.h` | 200 B | Comfort interface |
| `lib/phase/playlist.h` | 400 B | Playlist controller API |
| `lib/phase/playlist.c` | 1.2 KB | Zone logic, weights, rotation |
| `watch-faces/complication/emergence_face.h` | 300 B | |
| `watch-faces/complication/emergence_face.c` | 800 B | |
| `watch-faces/complication/momentum_face.h` | 300 B | |
| `watch-faces/complication/momentum_face.c` | 800 B | |
| `watch-faces/complication/active_face.h` | 300 B | |
| `watch-faces/complication/active_face.c` | 800 B | |
| `watch-faces/complication/descent_face.h` | 300 B | |
| `watch-faces/complication/descent_face.c` | 800 B | |
| `builder/js/phase-config.js` | 2 KB | Builder zone config UI |

### 4.2 Existing Files to Modify

| File | Change |
|------|--------|
| `Makefile` | Add `lib/metrics/` sources and include path |
| `watch-faces.mk` | Add 4 zone faces |
| `movement.c` | Add playlist tick integration (guarded) |
| `movement.h` | Add `metrics_engine_t` to movement state (guarded) |
| `movement_config.h` | Add zone threshold defines |
| `movement_faces.h` | Register 4 new faces |
| `lib/phase/phase_engine.h` | Add `PHASE_ZONE_*` enum |
| `builder/index.html` | Add zone configuration section |

### 4.3 `#ifdef` Guard Strategy

All new code uses the existing `PHASE_ENGINE_ENABLED` flag:

```c
#ifdef PHASE_ENGINE_ENABLED
// metrics engine, playlist controller, zone faces
#endif
```

Individual metrics do NOT get separate guards — they're lightweight enough that all-or-nothing is simpler and the total cost is bounded. Zone faces ARE individually guarded in `movement_faces.h` via the face list (user selects which faces to include in builder).

---

## 5. Implementation Sequence

### PR 1: Metric Engine Core (`3A` — foundation)

**Branch:** `phase3-metrics`

**Contents:**
- `lib/metrics/metrics.h` and `metrics.c` (orchestration)
- `lib/metrics/metric_sd.c` (Sleep Debt — simplest, uses existing sleep data)
- `lib/metrics/metric_comfort.c` (Comfort — uses existing homebase + sensors)
- Makefile updates
- Unit test: `test_metrics.sh` (compile check + stub sensor values)

**Dependencies:** Phase engine (Phase 2 ✅), circadian_score lib (existing ✅)

**Testing:**
- Compile with/without `PHASE_ENGINE_ENABLED`
- Verify zero cost when disabled (`arm-none-eabi-size` comparison)
- Stub sensor values → verify SD and Comfort produce sane scores
- BKUP save/load round-trip test

**Estimated size:** ~2.5 KB flash

---

### PR 2: Remaining Metrics (`3A` — complete)

**Branch:** `phase3-metrics-full`

**Contents:**
- `metric_em.c` (Emotional — circadian + lunar + variance)
- `metric_wk.c` (Wake Momentum — wake detection + ramp)
- `metric_energy.c` (Energy — derived)
- `metric_jl.c` (Jet Lag — timezone delta + decay)

**Dependencies:** PR 1

**Testing:**
- Each metric tested in isolation with known inputs
- Boundary conditions: all-zeros input, all-max input
- Verify integer overflow safety (no multiplication exceeds int32_t)
- Full `metrics_update()` integration test with all 6 metrics

**Estimated size:** ~2 KB additional flash

---

### PR 3: Playlist Controller (`3B`)

**Branch:** `phase3-playlist`

**Contents:**
- `lib/phase/playlist.h` and `playlist.c`
- Zone determination with hysteresis
- Weight tables (const flash)
- Relevance computation
- Face rotation logic
- Integration into `movement.c` tick handler

**Dependencies:** PR 2 (needs metrics snapshot)

**Testing:**
- Unit test: phase score → zone mapping (including boundary hysteresis)
- Unit test: metric values → relevance ranking → face order
- Integration test: full pipeline from `phase_compute()` → `metrics_update()` → `playlist_update()`
- Manual test on emulator: verify zone transitions with simulated time progression

**Estimated size:** ~1.5 KB flash

---

### PR 4: Zone Faces (`3C`)

**Branch:** `phase3-zone-faces`

**Contents:**
- 4 zone faces (emergence, momentum, active, descent)
- Face registration in `movement_faces.h`
- `watch-faces.mk` updates

**Dependencies:** PR 3 (faces receive metrics from playlist)

**Testing:**
- Emulator build with all 4 faces enabled
- Each face displays correct metrics for its zone
- Button interactions (ALARM cycles detail, LIGHT toggles backlight)
- 7-segment rendering verification (no invalid characters)

**Estimated size:** ~3.2 KB flash (4 × ~800 B)

---

### PR 5: Builder UI (`3D`)

**Branch:** `phase3-builder`

**Contents:**
- Zone configuration UI in builder
- Default threshold presets
- Advanced weight editor (collapsible)
- Generate appropriate `#define` values

**Dependencies:** PR 4 (needs final define names)

**Testing:**
- Builder generates valid config with default settings
- Custom zone thresholds produce correct defines
- Build succeeds with builder-generated config

**Estimated size:** 0 KB firmware (builder is web-only)

---

### PR 6: Integration & Polish

**Branch:** `phase3-integration`

**Contents:**
- End-to-end integration testing
- `minimal_phase_face` updated to show zone indicator
- Documentation updates (README, INTEGRATION_GUIDE)
- Flash/RAM budget verification

**Dependencies:** All previous PRs

**Testing:**
- Full firmware build with phase system enabled
- 24-hour simulated run on emulator
- Flash/RAM measurements vs budget
- Regression: all existing faces still work

---

## 6. Flash/RAM Budget

### 6.1 Component Estimates

| Component | Flash (bytes) | RAM (bytes) |
|-----------|---------------|-------------|
| **Existing Phase 1-2** | | |
| Homebase table | 2,200 | 0 (const) |
| Phase engine | 2,800 | 64 |
| Minimal phase face | 1,200 | 32 |
| **New Phase 3** | | |
| Metric engine core | 1,500 | 8 |
| metric_sd | 600 | 4 |
| metric_em | 800 | 4 |
| metric_wk | 500 | 4 |
| metric_energy | 300 | 2 |
| metric_jl | 400 | 4 |
| metric_comfort | 500 | 2 |
| Playlist controller | 1,500 | 16 |
| Zone weight tables | 200 | 0 (const) |
| Emergence face | 800 | 16 |
| Momentum face | 800 | 16 |
| Active face | 800 | 16 |
| Descent face | 800 | 16 |
| **Phase 3 Total** | **~9,200** | **~106** |
| **Grand Total (P1-3)** | **~15,400** | **~202** |

### 6.2 Budget Assessment

| Resource | Available | Used by Phase | % Used | Verdict |
|----------|-----------|---------------|--------|---------|
| Flash | 256 KB (262,144 B) | ~15.4 KB | 5.9% | ✅ Comfortable |
| RAM | 32 KB (32,768 B) | ~202 B | 0.6% | ✅ Negligible |
| BKUP registers | 8 regs (32 B) | 2 regs (7 B used) | 25% | ⚠️ Monitor — check availability with `next_available_backup_register` |

### 6.3 Verification Strategy

1. **Per-PR:** `arm-none-eabi-size` before/after comparison
2. **Threshold alert:** Flag if any PR adds >3 KB flash
3. **Final:** Full build with phase enabled vs disabled, document delta
4. **Regression:** Verify total firmware still fits with maximum face configuration

---

## 7. Risk Register

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| BKUP register exhaustion | Medium | High | Survey current usage first; fall back to EEPROM/flash row if needed |
| Integer overflow in metric math | Low | High | All multiplications use int32_t intermediates; test boundary values |
| Playlist flicker at zone boundaries | Medium | Low | 45-min hysteresis; tested in PR 3 |
| Activity sensor unavailable (Green board) | Certain | Medium | Graceful degradation: WK and Energy use time-only fallback |
| Flash budget exceeded | Low | Medium | Individual metrics are tiny; can remove JL (niche) if needed |

---

## 8. Open Questions

1. **BKUP register availability:** Need to audit current `next_available_backup_register` value. If ≤5, we have room. If 6+, need EEPROM fallback for metric persistence.

2. **Wake detection:** Currently using Active Hours boundary. Should we also detect wake from accelerometer "first significant motion after sleep window"? More accurate but more complex.

3. **Comms sync for JL:** What's the protocol for pushing timezone delta via optical comms? Needs companion app update.

4. **Zone face auto-advance:** Should the display auto-cycle through relevant metrics, or only advance on button press? Auto-advance saves button wear but may be distracting.

---

## 9. Success Criteria

- [ ] All 6 metrics produce scores 0–100 from real sensor inputs
- [ ] Playlist controller correctly maps phase → zone → face rotation
- [ ] Zone transitions are smooth (no flicker, hysteresis works)
- [ ] All 4 zone faces render correctly on 7-segment display
- [ ] Builder UI generates valid zone configuration
- [ ] Flash overhead < 16 KB total (phases 1-3 combined)
- [ ] RAM overhead < 256 bytes total
- [ ] Existing faces unaffected when `PHASE_ENGINE_ENABLED` is off
- [ ] Full system compiles and runs on emulator for 24-hour simulation
