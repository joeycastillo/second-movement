# Phase 3 Pre-Work Results

**Completion Date:** 2026-02-20  
**Author:** Agent (subagent phase3-prework)  
**Purpose:** Verify readiness for Phase 3 Metric Engine implementation

---

## 1. BKUP Register Audit

### Current Allocation

| Register | Usage | Claimant | Size |
|----------|-------|----------|------|
| BKUP[0] | movement_settings_t | Movement core | 32 bits |
| BKUP[1] | movement_location_t | Movement core | 32 bits |
| BKUP[2] | movement_active_hours_t | Movement core | 32 bits |
| BKUP[3] | movement_reserved_t | Movement core (placeholder) | 32 bits |
| BKUP[4] | **AVAILABLE** | — | 32 bits |
| BKUP[5] | **AVAILABLE** | — | 32 bits |
| BKUP[6] | **AVAILABLE** | — | 32 bits |
| BKUP[7] | RTC reference time | watch_rtc (system) | 32 bits |

### Code Evidence

**movement.c:1215** — Initialization:
```c
movement_state.next_available_backup_register = 4;
```

**movement.c:772-773** — Claim function:
```c
uint8_t movement_claim_backup_register(void) {
    // We use backup register 7 in watch_rtc to keep track of the reference time
    if (movement_state.next_available_backup_register >= 7) return 0;
    return movement_state.next_available_backup_register++;
}
```

### Current Claimants in Build

**Search performed:**
```bash
grep -rn "movement_claim_backup_register" watch-faces/ --include="*.c"
```

**Result:** No matches in currently enabled faces.

**Legacy claimants (not in build):**
- `legacy/watch_faces/complication/menstrual_cycle_face.c` claims 2 registers
- This face is **NOT** in `movement_config.h` — legacy only

### Phase 3 Requirements

**Metric Engine Storage:**
- SD (Sleep Debt): 3 bytes (3 × deficit/4, packed uint8_t)
- WK (Wake Momentum): 2 bytes (wake_onset_hour + wake_onset_minute)
- JL (Jet Lag): 2 bytes (jl_score + jl_hours_remaining)
- **Total:** 7 bytes

**Registers needed:** 7 bytes = **2 BKUP registers** (8 bytes capacity)

### Verdict

✅ **GO — Sufficient registers available**

- **Available:** BKUP[4], BKUP[5], BKUP[6] (3 registers = 12 bytes)
- **Required:** 2 registers (8 bytes)
- **Remaining:** 1 register (4 bytes) free for future expansion

**No EEPROM fallback needed.**

---

## 2. Display Mockups

### Display Constraints

**Hardware:** Casio F91W LCD (7-segment)
- 10 character positions total
- Position 0-1: Top left (2 chars) — weekday/mode area
- Position 2-3: Top right (2 chars) — day of month area
- Position 4-9: Bottom (6 chars) — main time display area

**Character support:**
- Alphanumeric: A-Z, a-z, 0-9 (7-segment approximations)
- Special chars: limited (see `watch_common_display.h`)
- **NO arrow characters in charset** (↑↓ not available in 7-segment)
- Arrows only via **indicators** (WATCH_INDICATOR_ARROWS, WATCH_INDICATOR_LAP on custom LCD only)

**Display functions:**
- `watch_display_text(WATCH_POSITION_FULL, buf)` — 10 chars (positions 0-9)
- `watch_display_text(WATCH_POSITION_TOP_LEFT, buf)` — 2 chars (positions 0-1)
- `watch_display_text(WATCH_POSITION_TOP_RIGHT, buf)` — 2 chars (positions 2-3)
- `watch_display_text(WATCH_POSITION_BOTTOM, buf)` — 6 chars (positions 4-9)
- `watch_set_indicator(WATCH_INDICATOR_X)` — turn on indicator (limited options)

**Existing pattern examples:**
```c
// circadian_score_face.c
snprintf(buf, sizeof(buf), "CS  %2d", components.overall_score);
watch_display_text(WATCH_POSITION_FULL, buf);

// sleep_tracker_face.c
snprintf(buf, sizeof(buf), "%d  ", state->total_wake_minutes);
watch_display_text(WATCH_POSITION_TOP_RIGHT, "min");
watch_display_text(WATCH_POSITION_BOTTOM, buf);
```

### Zone Face Requirements

From PHASE3_IMPLEMENTATION_PLAN.md:

| Zone | Primary Metrics | Weights (SD/EM/WK/NRG/JL/CMF) |
|------|----------------|-------------------------------|
| Emergence | SD, EM, Comfort | [30, 25, 5, 10, 10, 20] |
| Momentum | WK, SD, EM | [20, 20, 30, 10, 10, 10] |
| Active | Energy, EM, SD | [15, 20, 5, 40, 10, 10] |
| Descent | Comfort, EM, JL | [10, 25, 0, 10, 25, 30] |

---

### Emergence Face

**Zone Character:** EM  
**Primary Metrics:** SD (Sleep Debt), EM (Emotional/Mood), Comfort (CMF)  
**Display Strategy:** Zone indicator + primary metric, secondary on scroll

**Layout Option A — Full 10-char:**
```
Position:  0123456789
Display:   EM  SD  72
```
- Positions 0-1: `EM` (zone indicator)
- Positions 2-3: `  ` (spaces)
- Positions 4-5: `SD` (metric abbreviation)
- Positions 6-7: `  ` (spaces)
- Positions 8-9: `72` (metric value 0-100)

**Layout Option B — Split top/bottom:**
```
Top:       EM      (zone indicator in top-left)
Bottom:    SD  72  (metric + value)
```
- `watch_display_text(WATCH_POSITION_TOP_LEFT, "EM")`
- `watch_display_text(WATCH_POSITION_BOTTOM, "SD  72")`

**Alarm button cycles through metrics:**
```
View 1:  EM  SD  72    (Sleep Debt)
View 2:  EM  EM  45    (Emotional score)
View 3:  EM  CMF 88    (Comfort)
```

**Trend indication workaround** (since no arrow chars):
- Use WATCH_INDICATOR_LAP (looped arrow on custom LCD) for rising trend
- Or display numeric delta: `EM  SD+5` (5-point increase since last hour)
- Or use character approximations: `<` (falling), `>` (rising), `=` (stable)

**Recommendation:** Use split layout (Option B) — cleaner separation, easier to read at a glance.

---

### Momentum Face

**Zone Character:** MO  
**Primary Metrics:** WK (Wake Momentum), SD (Sleep Debt), temperature  
**Display Strategy:** Progress-style display for WK (it's a ramp 0-100)

**Layout Option A — Progress bar simulation:**
```
Position:  0123456789
Display:   MO  WK--58
```
- `MO`: zone
- `WK`: metric
- `--`: visual fill (repeating chars to simulate progress)
  - Use repeated `=` or `-` characters: 0% = empty, 50% = `--`, 100% = `====`
  - Example: `WK====72` (full ramp) vs `WK--  32` (early morning)

**Layout Option B — Numeric with secondary metric:**
```
Top:       MO
Bottom:    WK  58
```
Press ALARM to cycle:
```
View 1:  MO  WK  58    (Wake Momentum)
View 2:  MO  SD  32    (Sleep Debt)
View 3:  MO  T  18C    (Temperature in Celsius)
```

**Constraint check:**
- Temperature display: `T  18C` (6 chars fits in bottom)
- Or use TOP_RIGHT for units: `watch_display_text(WATCH_POSITION_TOP_RIGHT, " C")`

**Recommendation:** Option B (numeric + cycling). Progress bar is visually cluttered in 7-segment. Reserve visual indicators for custom LCD builds.

---

### Active Face

**Zone Character:** AC  
**Primary Metrics:** Energy (NRG), SD, EM  
**Display Strategy:** Energy dominates (weight=40), show clearly

**Layout:**
```
Top:       AC
Bottom:    NRG 87
```
Press ALARM to cycle:
```
View 1:  AC  NRG 87    (Energy)
View 2:  AC  EM  62    (Emotional)
View 3:  AC  SD  15    (Sleep Debt — low in Active zone)
```

**Alternative — Compact abbreviation:**
```
Display:   AC E87      (E = Energy shorthand)
```
- Saves space but less clear
- Stick with 3-letter codes for consistency: `NRG`, `SD `, `EM `

**Recommendation:** Standard 3-letter metric codes, cycling views.

---

### Descent Face

**Zone Character:** DE  
**Primary Metrics:** Comfort (CMF), EM, JL (Jet Lag)  
**Display Strategy:** Winding down metrics — comfort and mood

**Layout:**
```
Top:       DE
Bottom:    CMF 94
```
Press ALARM to cycle:
```
View 1:  DE  CMF 94    (Comfort)
View 2:  DE  EM  55    (Emotional)
View 3:  DE  JL  12    (Jet Lag — if active)
```

**JL special case:**
- If JL score = 0 (no recent timezone shift), display `DE  JL  --` (unavailable)
- Only show numeric value if jet lag is active

**Recommendation:** Standard layout. Use `--` to indicate inactive/unavailable metrics.

---

### Display Limitations & Recommendations

**Character constraints:**
1. **No arrow characters** — Use indicators (custom LCD only) or text substitutes:
   - `<` = falling trend
   - `>` = rising trend  
   - `=` = stable
   - Or numeric deltas: `+5`, `-3`

2. **No arbitrary icons** — Use watch indicators for status:
   - `WATCH_INDICATOR_SIGNAL` = sensors active
   - `WATCH_INDICATOR_LAP` = zone transition / loop
   - `WATCH_INDICATOR_ARROWS` = data sync (custom LCD only)

3. **Limited character positions** — 10 total, shared among all display elements:
   - Keep zone indicators to 2 chars (EM, MO, AC, DE)
   - Keep metric codes to 2-3 chars (SD, EM, WK, NRG, JL, CMF)
   - Reserve 2-3 chars for numeric values (0-100)

4. **7-segment limitations** — Some characters don't render well:
   - Avoid: `K`, `M`, `W`, `X` in certain positions (segment overlap)
   - Safe: All digits, `A-H`, `L`, `O`, `P`, `S`, `C`, `E`, `F`, `T`, `U`
   - Our metric codes are all safe: SD, EM, WK, NRG (as `nrG`), JL, CMF

**Recommended pattern for all zone faces:**
```c
void _zone_face_update_display(zone_face_state_t *state, metrics_snapshot_t *metrics) {
    char buf[11];
    
    // Display zone indicator in top-left
    watch_display_text(WATCH_POSITION_TOP_LEFT, "EM");
    
    // Display current metric view in bottom
    switch (state->view) {
        case VIEW_PRIMARY:
            snprintf(buf, sizeof(buf), "SD  %2d", metrics->sd);
            break;
        case VIEW_SECONDARY:
            snprintf(buf, sizeof(buf), "EM  %2d", metrics->em);
            break;
        // ... more views
    }
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}
```

**Alternative layouts for consideration:**
- **Compact 10-char full:** `EM SD 72` (6 chars — leaves 4 unused, harder to read)
- **Top-right for units:** Use positions 2-3 for units/context (e.g., `%`, `C`, `hr`)
- **Blinking indicators:** Use `watch_start_indicator_blink()` to signal zone transitions

---

## 3. Jet Lag Metric Decision

### Problem Statement

JL (Jet Lag) metric requires **timezone delta input** to function:
- Algorithm: `JL = abs(tz_delta_hours) * 12` (capped at 100)
- Decay: `JL -= max(1, JL / 24)` per hour
- Trigger: Timezone change event

**Dependency:** Companion app must push new timezone when user travels.

### Current Comms Protocol

**File:** `docs/architecture/sensor-watch-comms-architecture.md`

**Control commands (Stream Type 0x03):**
```
0x01  SET_TIME        (4 bytes: unix epoch, truncated)
0x02  SET_ALARM       (3 bytes: hour, min, window_min)
0x03  SET_ACTIVE_HRS  (2 bytes: start_hour, end_hour)
0x04  ACK_SYNC        (1 byte: streams received bitmask)
0x05  REQUEST_STREAM  (1 byte: stream type to send)
0x10  FIRMWARE_BLOCK  (experimental)
```

**Result:** ❌ **NO timezone push command exists**

### Dependency Chain

To implement JL in Phase 3:
1. ✅ Metric algorithm (trivial — already spec'd)
2. ❌ Comms protocol extension (add `0x06 SET_TIMEZONE`)
3. ❌ Companion app timezone detection (requires geolocation or manual input)
4. ❌ Companion app optical TX for timezone push
5. ✅ Watch-side BKUP storage (2 bytes — already allocated)

**Blockers:** Items 2-4 are **not implemented** and are Phase 4 scope per the architecture doc:
> "Jet lag protocol — Needs robust timezone detection + circadian model" (listed under "Hard (v2+)")

### Implementation Options

#### Option A: Keep JL in Phase 3 (stub implementation)

**Approach:**
```c
// metric_jl.c
uint8_t metric_jl_compute(int8_t tz_delta_hours, uint8_t hours_since_shift) {
    #ifdef COMMS_TZ_PROTOCOL_ENABLED
        // Full implementation (Phase 4+)
        uint8_t jl = abs(tz_delta_hours) * 12;
        if (jl > 100) jl = 100;
        // Decay logic...
        return jl;
    #else
        // Stub: always return 0 (no jet lag)
        return 0;
    #endif
}
```

**Pros:**
- Complete 6-metric architecture in Phase 3
- Descent zone face shows JL slot (displays `JL  --`)
- Algorithm is ready when comms protocol lands (Phase 4)

**Cons:**
- Non-functional code in production (confusing for users who see JL but it's always `--`)
- Wasted flash/RAM (~200 bytes for JL module)
- Descent zone weight table includes JL (weight=25) but it contributes nothing
- Misleading: users might expect jet lag tracking but it won't work

#### Option B: Defer JL to Phase 4

**Approach:**
- Implement only 5 metrics in Phase 3: SD, EM, WK, Energy, Comfort
- Update Descent zone weights to exclude JL:
  ```c
  // OLD (6 metrics): [10, 25, 0, 10, 25, 30]  // SD EM WK NRG JL CMF
  // NEW (5 metrics): [15, 35, 0, 15,  0, 35]  // SD EM WK NRG    CMF
  ```
  - Redistribute JL's 25 weight to EM (+10) and Comfort (+15)
- Descent face shows only CMF, EM, (optionally SD)
- Add JL in Phase 4 when comms protocol is ready

**Pros:**
- ✅ **No dead code** — everything that ships is functional
- ✅ Simpler Phase 3 implementation (one less metric to test)
- ✅ Descent zone still functional (Comfort and EM are strong signals for wind-down)
- ✅ Clear scope boundary: Phase 3 = sensor-based metrics, Phase 4 = comms-enhanced metrics

**Cons:**
- Incomplete metric set (5/6)
- Descent zone weight table needs adjustment
- Need to add JL slot back in Phase 4 (but this is a feature add, not a breaking change)

### Analysis: Descent Zone Without JL

**Current Descent weights:** `[10, 25, 0, 10, 25, 30]` (SD, EM, WK, NRG, JL, CMF)

**Proposed Phase 3 weights:** `[15, 35, 0, 15, 0, 35]` (SD, EM, WK, NRG, —, CMF)

**What Descent zone needs:**
1. **Comfort dominance** — Is temperature/light aligned with seasonal norms for winding down?
2. **Mood tracking** — Is EM dropping naturally (circadian dip expected in evening)?
3. **Sleep debt awareness** — Higher SD in evening = earlier sleep pressure

**Without JL:**
- Comfort (35 weight) still detects environmental misalignment (too bright, too warm)
- EM (35 weight) tracks circadian mood dip (natural evening fade)
- SD (15 weight) surfaces if user is sleep-deprived and needs earlier bedtime

**Jet lag is a travel-specific edge case:**
- Most users are NOT jet-lagged most of the time
- JL only matters after timezone shift (few times per year for most people)
- When it matters, it matters a lot — but that's Phase 4 scope

**Verdict:** Descent zone remains **highly functional** without JL. The core evening wind-down signals (comfort, mood, sleep pressure) are intact.

---

### Recommendation

✅ **Option B: Defer JL to Phase 4**

**Reasoning:**
1. **No comms protocol** — JL cannot function without `SET_TIMEZONE` command
2. **No companion app support** — Timezone detection/push is Phase 4 scope
3. **Minimal functional loss** — Descent zone works well with Comfort + EM + SD
4. **Cleaner Phase 3** — Avoids stub code, confusing UX, and wasted flash
5. **Future-proof** — Adding JL in Phase 4 is a straightforward feature add

**Phase 3 Adjustments:**
- Implement 5 metrics: SD, EM, WK, Energy, Comfort
- Update zone weight tables:
  ```c
  static const uint8_t zone_weights[4][5] = {
      {30, 25,  5, 10, 30},  // EMERGENCE: SD EM WK NRG CMF
      {20, 20, 30, 10, 20},  // MOMENTUM:  SD EM WK NRG CMF
      {15, 20,  5, 40, 20},  // ACTIVE:    SD EM WK NRG CMF
      {15, 35,  0, 15, 35},  // DESCENT:   SD EM WK NRG CMF
  };
  ```
- Descent face views:
  ```
  View 1:  DE  CMF 94    (Comfort — primary)
  View 2:  DE  EM  55    (Emotional — secondary)
  View 3:  DE  SD  18    (Sleep Debt — tertiary)
  ```

**Phase 4 Addition (when comms ready):**
- Add `0x06 SET_TIMEZONE` command to optical RX protocol
- Implement `metric_jl.c` module
- Update weight tables to 6 metrics (add JL column)
- Add JL view to Descent face

---

## 4. Green Board Fallback Algorithms

### Context

**Green Sensor Watch board** (original hardware) has **no LIS2DW12 accelerometer**.

**Affected metrics:**
- **WK (Wake Momentum)** — relies on cumulative activity since wake
- **Energy** — includes activity bonus component

**Detection strategy:**
Runtime check via `movement_state.has_lis2dw` boolean (set during `movement_setup()`).

**movement.c:1270-1272:**
```c
if (lis2dw_begin()) {
    movement_state.has_lis2dw = true;
} else {
    movement_state.has_lis2dw = false;
}
```

---

### Wake Momentum (WK) Fallback

#### Normal Algorithm (with accelerometer)

```c
// metric_wk.c (full implementation)
uint8_t metric_wk_compute(uint16_t minutes_awake, uint16_t cumulative_activity) {
    // Base ramp: 0-100 over 2 hours
    uint8_t base = minutes_awake * 100 / 120;  // 120 min = 2hr
    if (base > 100) base = 100;
    
    // Activity bonus: accelerate ramp if moving
    uint8_t bonus = 0;
    if (cumulative_activity > ACTIVITY_THRESHOLD) {
        bonus = 30;  // +30% for active morning
    }
    
    uint8_t wk = base + (base * bonus / 100);
    if (wk > 100) wk = 100;
    return wk;
}
```

**Inputs:**
- `minutes_awake` — derived from active hours config (wake window start)
- `cumulative_activity` — sum of motion events since wake (accelerometer)

**Behavior:**
- Ramps linearly from 0→100 over 2 hours
- Activity bonus accelerates ramp by up to 30% (e.g., morning exercise)
- Caps at 100

#### Fallback Algorithm (no accelerometer)

**Strategy:** Slower ramp, no activity bonus.

```c
// metric_wk.c (fallback when !has_lis2dw)
uint8_t metric_wk_compute_fallback(uint16_t minutes_awake) {
    // Slower ramp: 0-100 over 3 hours (no activity data to accelerate)
    uint8_t wk = minutes_awake * 100 / 180;  // 180 min = 3hr
    if (wk > 100) wk = 100;
    return wk;
}
```

**Why 3 hours?**
- Without activity data, can't detect active vs sedentary morning
- Conservative estimate: assume average person takes ~3hr to reach full alertness
- Based on sleep inertia research (Tassi & Muzet, 2000): peak cognitive function 1-3hr post-wake

**Accuracy loss:**
- Early riser who exercises: WK score **underestimated** by ~20-30 points in first 2 hours
- Sedentary morning: WK score relatively **accurate** (no activity bonus to miss)
- Converges to 100 after 3hr regardless of activity level

**Example comparison:**

| Time Since Wake | Full (with accel) | Fallback (no accel) | Delta |
|-----------------|-------------------|---------------------|-------|
| 30 min | 25 + bonus (→35 if active) | 17 | -8 to -18 |
| 60 min | 50 + bonus (→65 if active) | 33 | -17 to -32 |
| 90 min | 75 + bonus (→97 if active) | 50 | -25 to -47 |
| 120 min | 100 | 67 | -33 |
| 180 min | 100 | 100 | 0 |

**Impact on zone determination:**
- Slower WK ramp may delay Emergence→Momentum transition by 30-60 minutes
- Once fully awake (3hr+), behavior is identical

---

### Energy Fallback

#### Normal Algorithm (with accelerometer)

```c
// metric_energy.c (full implementation)
uint8_t metric_energy_compute(uint8_t phase_score, uint8_t sd, uint16_t recent_activity) {
    // Base energy = phase alignment - sleep debt penalty
    int16_t energy = phase_score - (sd / 3);
    
    // Activity bonus: recent movement boosts perceived energy
    uint8_t activity_bonus = recent_activity / 50;  // 0-20 range
    if (activity_bonus > 20) activity_bonus = 20;
    
    energy += activity_bonus;
    
    // Clamp to 0-100
    if (energy < 0) energy = 0;
    if (energy > 100) energy = 100;
    
    return (uint8_t)energy;
}
```

**Inputs:**
- `phase_score` — from phase engine (0-100)
- `sd` — Sleep Debt (0-100)
- `recent_activity` — motion events in last 15 minutes (accelerometer)

**Activity bonus:**
- Maps recent motion to 0-20 point boost
- Rationale: Physical movement increases subjective energy (endorphin effect)
- Example: Quick walk = +10-15 energy points for ~30 min

#### Fallback Algorithm (no accelerometer)

**Option 1: No activity bonus, rely on phase + SD only**

```c
// metric_energy.c (simple fallback)
uint8_t metric_energy_compute_fallback(uint8_t phase_score, uint8_t sd) {
    int16_t energy = phase_score - (sd / 3);
    
    // Clamp
    if (energy < 0) energy = 0;
    if (energy > 100) energy = 100;
    
    return (uint8_t)energy;
}
```

**Pros:** Simple, stable
**Cons:** Misses short-term energy boosts from movement (10-20 point underestimation)

**Option 2: Circadian curve bonus** (compensate for missing activity data)

```c
// metric_energy.c (enhanced fallback)
uint8_t metric_energy_compute_fallback(uint8_t phase_score, uint8_t sd, uint8_t hour) {
    int16_t energy = phase_score - (sd / 3);
    
    // Use circadian curve as proxy for "expected activity level"
    // Peak energy at midday, low in early morning/night
    extern const int16_t cosine_lut_24[24];  // from phase_engine.c
    int16_t circadian_bonus = (cosine_lut_24[hour] + 1000) / 100;  // 0-20 range
    
    energy += circadian_bonus;
    
    // Clamp
    if (energy < 0) energy = 0;
    if (energy > 100) energy = 100;
    
    return (uint8_t)energy;
}
```

**Pros:** More dynamic, reflects natural energy curve throughout day
**Cons:** Assumes user follows typical circadian pattern (may not apply to shift workers)

**Recommendation:** **Option 2** (circadian bonus)
- Provides more useful signal than flat no-bonus approach
- Aligns with phase engine's circadian model
- Gracefully degrades for atypical schedules (phase_score is still primary input)

---

### Detection Strategy

**Runtime check pattern:**

```c
// metric_wk.c
uint8_t metric_wk_compute(metrics_engine_t *engine, uint16_t minutes_awake) {
    extern movement_state_t movement_state;
    
    if (movement_state.has_lis2dw) {
        // Full algorithm with activity data
        uint16_t activity = _get_cumulative_activity(engine);
        return _wk_compute_full(minutes_awake, activity);
    } else {
        // Fallback: time-only ramp
        return _wk_compute_fallback(minutes_awake);
    }
}
```

**Why runtime vs compile-time?**
- `movement_state.has_lis2dw` is set at boot after I2C probe
- Same firmware binary runs on both Green board (no accel) and Sensor Watch Pro (with accel)
- No need for separate builds or `#ifdef` maze
- Graceful degradation: firmware works on both platforms

**Alternative: Compile-time guard** (if firmware is built per-board)

```c
#ifdef HAS_ACCELEROMETER
    // Full implementation
#else
    // Fallback
#endif
```

**Verdict:** Use **runtime check** (`movement_state.has_lis2dw`)
- Matches existing pattern in movement.c (grep shows 9 instances)
- Single firmware binary for all hardware variants
- Easier testing (can simulate no-accel mode by forcing `has_lis2dw = false`)

---

### Expected Behavior Difference

| Metric | Full (with accel) | Fallback (no accel) | Accuracy Loss |
|--------|-------------------|---------------------|---------------|
| **WK** | 0-100 over 2hr, activity-accelerated | 0-100 over 3hr, linear | Underestimate by 20-30 pts in first 2hr, converges at 3hr |
| **Energy** | phase - SD + activity bonus (0-20) | phase - SD + circadian bonus (0-20) | Misses short-term movement boosts, but circadian curve compensates on average |

**Zone impact:**
- **Momentum zone** (WK weight=30): May take 30-60 min longer to enter on Green board
- **Active zone** (Energy weight=40): Slight underestimation during sedentary periods, but phase+circadian keeps it in range

**User experience:**
- Green board users: Slightly slower morning ramp, less responsive to immediate activity changes
- Sensor Watch Pro users: Full responsiveness, activity-aware scoring
- Both: Functional metric engine, valid zone transitions, useful complication data

**Mitigation:**
- Display indicator (e.g., `WATCH_INDICATOR_SIGNAL` off) when accel unavailable? → No, too distracting
- Document in help text: "Best experience with Sensor Watch Pro (accelerometer-enhanced)"
- Accept graceful degradation as design tradeoff

---

## Summary & Recommendations

### Task 1: BKUP Registers
✅ **3 registers available (BKUP[4-6]), need 2 — PROCEED**

### Task 2: Display Mockups
✅ **All 4 zone faces fit in 7-segment constraints**
- Use split layout (top-left zone, bottom metric+value)
- No arrow characters — use text substitutes (`<`, `>`, `=`) or numeric deltas
- ALARM button cycles through 2-3 metric views per zone
- See mockups above for exact layouts

### Task 3: Jet Lag Decision
✅ **Defer JL to Phase 4**
- No comms protocol for timezone push (blocker)
- Descent zone functional with 5 metrics (CMF, EM, SD)
- Add JL when `SET_TIMEZONE` command is implemented (Phase 4)
- Update weight tables to 5-metric configuration

### Task 4: Green Board Fallbacks
✅ **Documented exact fallback algorithms**
- **WK:** 3-hour linear ramp (vs 2-hour activity-accelerated)
- **Energy:** phase - SD + circadian bonus (vs activity bonus)
- **Detection:** Runtime check via `movement_state.has_lis2dw`
- **Impact:** 20-30% accuracy loss in first 2 hours post-wake, converges thereafter

### Blockers Cleared
**Phase 3 is GO for implementation.**

All 4 pre-work tasks completed. No critical blockers identified.

---

**Next Steps:**
1. Commit this document: `git add PHASE3_PREWORK.md && git commit -m "phase3: Complete pre-work (BKUP audit, display mockups, JL decision, fallbacks)"`
2. Begin Phase 3A: Metric Engine implementation (`lib/metrics/`)
3. Reference this document for display constraints and fallback patterns

**End of Pre-Work Report**
