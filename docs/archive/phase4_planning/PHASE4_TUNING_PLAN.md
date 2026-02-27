# Phase 4: Behavioral Tuning & Sensor Integration

> **Status:** Planning (Research Complete)  
> **Depends on:** Phase 3 (Metric Engine ✅, Playlist ✅, Zone Faces ✅)  
> **Goal:** Connect Phase 3 infrastructure to real sensors, implement intelligent surfacing, validate behavioral stability through dogfooding  
> **Design authority:** Phase Watch Overhaul Plan + PHASE3_BASELINE_RESEARCH.md

---

## 0. Philosophy Shift: Features → Behavior

**Phase 3 delivered infrastructure:**
- ✅ Metric engine (SD, Comfort, EM, WK, Energy) - ~9.2 KB flash
- ✅ Phase computation (circadian curve + seasonal baseline)
- ✅ Zone faces (Emergence, Momentum, Active, Descent)
- ✅ Builder UI (homebase config, LED test, face selection)

**Phase 4 delivers experience:**
- Sensor integration (LIS2DW12 motion, lux averaging, temperature context)
- Intelligent surfacing (extreme values surface more, mundane values stay quiet)
- Behavioral stability (15-30 min phase inertia, not jittery seconds)
- Power efficiency (>1 year battery target, not 5-7 days)

**Success metric:** User starts checking watch before deciding what to do.  
**Hidden milestone:** Watch whispers "conditions align, move" without being asked.

---

## 1. Research Findings Summary

### Power Budget (From PHASE3_BASELINE_RESEARCH.md)

| Component | Rate | Power | Source |
|-----------|------|-------|--------|
| LIS2DW12 stationary mode | 1.6 Hz | 2.75 µA | Sensor Watch Pro sleep tracking |
| Lux sampling | 1/min | <0.1 µA | Community best practice |
| Phase update | 1/min | 0.2 µA | RTC alarm wake |
| Display update | zone-change only | ~1 µA amortized | Competitive analysis |
| **Total Phase Engine** | - | **~4 µA** | - |
| **Battery life target** | - | **>1 year** | vs fitness trackers: 5-7 days |

**Constraint:** SAM L22 has 32 KB RAM, 256 KB flash. CR2016 coin cell ~90 mAh. Budget ~10 µA average.

---

### Behavioral Patterns (From COMPETITIVE_ANALYSIS.md)

**Playlist rotation frequency:**
- **Oura/Whoop/Garmin:** Morning batch delivery, NOT real-time streaming
- **Garmin Body Battery:** Continuous background scoring, snapshots on glance
- **User preference:** 70% clock dominance, 10-sec metric dwell, zone-triggered rotation

**Update cadence research:**
- **<1 sec:** "Distracting" (r/sensorwatch complaints)
- **1-5 sec:** "Too frequent" (notification fatigue studies)
- **5-15 sec:** "Just right" for rotating multi-metric displays (Garmin praised)
- **1/min background:** Standard for fitness trackers (Oura, Whoop batch scoring)

**Zone change debouncing:**
- **Garmin:** 5-min smoothing window (prevents jitter, praised by users)
- **Academic research:** 15-30 min phase inertia feels "intelligent", not "reactive"
- **Community feedback:** Sensor Watch users complain when faces update >1/sec

---

### Surfacing Logic (From TUNING_PARAMETERS.md)

Modern fitness trackers use **weighted surfacing** to show relevant metrics:

```c
display_weight = 
    phase_affinity      // Does this metric matter in current zone?
    × anomaly           // How far from baseline/expected?
    × time_since_seen   // Haven't shown this in a while?
    × score_extremity   // Is value unusually high/low?
```

**Example:**
- Morning, Emergence zone, SD (Sleep Debt) = 85 (high) → surfaces immediately
- Afternoon, Active zone, WK (Wake Momentum) = 50 (normal) → stays quiet
- Evening, Descent zone, Comfort = 20 (low, unusual) → surfaces after 5 min

**Goal:** Extreme values surface more often. Mundane values stay quiet. User learns to check when something matters.

---

## 2. Phase 4 Architecture

### 2.1 Sensor Integration Layer

**New files:**
```
lib/phase/sensors.c
lib/phase/sensors.h
```

**Responsibilities:**
- Configure LIS2DW12 in stationary detect mode (1.6 Hz, 2.75 µA)
- Sample lux once per minute, maintain 5-min rolling average
- Read temperature from SAM L22 onboard thermistor
- Provide buffered sensor readings to metric engine

**Power budget:** ~3 µA (motion 2.75 µA + lux <0.1 µA + temp negligible)

---

### 2.2 Playlist Controller Enhancement

**Updated files:**
```
lib/phase/playlist.c (create - doesn't exist yet!)
lib/phase/playlist.h (create - doesn't exist yet!)
movement.c (wire playlist to face dispatch)
```

**Current state:** Playlist functions referenced in `movement.c` but not implemented.

**Responsibilities:**
- **Zone detection with 5-min debounce** (prevent jitter)
- **Surfacing weight calculation** (phase_affinity × anomaly × time × extremity)
- **70% clock dominance** (passive mode default)
- **10-sec metric dwell** when rotating
- **Zone-change triggered rotation** (not time-based spam)

**Power budget:** ~1 µA (display updates amortized)

---

### 2.3 Metric Engine Sensor Wiring

**Updated files:**
```
lib/metrics/metric_sd.c (wire to sleep tracker - already exists)
lib/metrics/metric_comfort.c (wire to lux sensor)
lib/metrics/metric_em.c (wire to motion variance)
lib/metrics/metric_wk.c (wire to motion intensity)
lib/metrics/metric_energy.c (derived from phase + WK)
```

**What to add:**
- Replace placeholder sensor reads with actual LIS2DW12/lux data
- Implement 5-min rolling averages for motion variance (EM input)
- Implement anomaly detection (moving baseline + stddev tracking)
- Add bounds checking and validation (PR #56 pattern)

**Power budget:** Negligible (computation only, sensors already sampled)

---

## 3. Implementation Phases

### Phase 4A: Sensor Integration (Week 1)

**Goal:** Connect LIS2DW12 and lux sensor to metric engine

**PRs:**
1. **PR #65: LIS2DW12 Stationary Mode Integration**
   - Configure accelerometer: 1.6 Hz ODR, LPMode1, stationary detect
   - Motion threshold: 62 mg, inactivity timeout: 15 min
   - Expose buffered motion data to metric engine
   - Test: Motion flag HIGH on 2-min walk, LOW after 15-min stillness
   - **Size:** ~300 lines (sensors.c/h)

2. **PR #66: Lux Sensor Integration**
   - Sample lux 1/min via RTC alarm
   - Maintain 5-min rolling average (300-sec window)
   - Expose averaged lux to Comfort metric
   - Test: Indoor (100 lux) → Outdoor (5000 lux) transitions over 5 min
   - **Size:** ~150 lines (sensors.c update)

3. **PR #67: Metric Engine Sensor Wiring**
   - Wire sensor readings to existing metrics (Comfort, EM, WK)
   - Replace placeholder values with real data
   - Add anomaly detection (baseline + stddev)
   - Test: All metrics compute correctly from sensor inputs
   - **Size:** ~200 lines (metric updates)

**Deliverable:** Metrics compute from real sensors (not placeholders)  
**Battery impact:** ~3 µA (LIS2DW12 2.75 µA + lux <0.1 µA)

---

### Phase 4B: Playlist Controller Implementation (Week 2)

**Goal:** Intelligent face rotation based on weighted surfacing

**PRs:**
1. **PR #68: Playlist Controller Core**
   - Create `lib/phase/playlist.c/h` (referenced but missing)
   - Implement zone detection with 5-min debounce
   - Implement hysteresis (±10% buffer to prevent edge oscillation)
   - Test: Rapid zone transitions debounce over 5 min
   - **Size:** ~250 lines (playlist.c/h)

2. **PR #69: Surfacing Weight Algorithm**
   - Calculate `display_weight = affinity × anomaly × time × extremity`
   - Track `last_displayed` timestamp per face
   - Sort faces by weight, select highest
   - Test: Extreme values (SD=85) surface immediately, normal values (WK=50) stay quiet
   - **Size:** ~150 lines (playlist.c update)

3. **PR #70: Clock Dominance & Dwell Timer**
   - Implement 70% clock dominance (passive mode default)
   - 10-sec dwell timer when rotating metrics
   - Zone-change triggered rotation (not time-based)
   - Test: 1-hour passive wear = ~42 min clock, ~18 min metrics
   - **Size:** ~100 lines (playlist.c update)

**Deliverable:** Intelligent rotation (not round-robin spam)  
**Battery impact:** ~1 µA (display updates amortized)

---

### Phase 4C: Display Integration & Tuning (Week 3)

**Goal:** Wire playlist to face dispatch, validate behavior on hardware

**PRs:**
1. **PR #71: Movement.c Playlist Dispatch**
   - Wire `playlist_update()` → face selection in `movement.c`
   - Display updates ONLY on zone changes (not every tick)
   - Manual override via MODE button
   - Test: Zone change triggers face switch within 5 min
   - **Size:** ~100 lines (movement.c update)

2. **PR #72: Power Optimization Audit**
   - Profile actual power consumption (oscilloscope if available)
   - Verify sleep modes between updates
   - Ensure no continuous polling (all interrupt/alarm-driven)
   - Document power budget breakdown
   - **Size:** ~0 lines code, documentation only

**Deliverable:** Complete Phase 4 system ready for dogfooding  
**Total battery budget:** ~4 µA (within 10 µA target)

---

### Phase 4D: Dogfooding & Iteration (Week 4+)

**Goal:** Validate behavioral stability through real-world wear

**Process (from CALIBRATION_CHECKLIST.md):**

**Days 1-3: Initial Validation**
- Flash Phase 4 firmware to Sensor Watch Pro (green board)
- Wear continuously for 72 hours
- Log observations:
  - Playlist rotation frequency (tally clock vs metric faces)
  - Zone changes (note time, conditions, did it feel right?)
  - Phase inertia (does phase feel jittery or smooth?)
  - Battery drain (measure voltage drop)

**Days 4-7: Tuning Iteration**
- Adjust parameters based on logs:
  - Debounce window (5 min → 10 min if too jittery)
  - Surfacing weights (if boring values surface too often)
  - Dwell time (10 sec → 15 sec if feels rushed)
- Re-flash, wear for 4 more days

**Days 8-14: Stability Validation**
- Long-term wear (full week)
- Measure success criteria:
  - ✅ 70% clock dominance achieved?
  - ✅ Phase changes over 15-30 min (not seconds)?
  - ✅ Extreme values surface appropriately?
  - ✅ Battery drain <10% over 7 days?

**Deliverable:** PHASE4_DOGFOOD_REPORT.md with tuning recommendations

---

## 4. Success Criteria

### Quantitative Metrics

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| Clock dominance | 70% ± 10% | Tally clock glances over 1 hour |
| Phase inertia | 15-30 min per zone | Log zone change timestamps |
| Rotation dwell | 10 sec ± 2 sec | Time metric face display duration |
| Battery life | >1 year (est) | Measure voltage drop over 7 days |
| Power consumption | <10 µA average | Oscilloscope profiling |

### Qualitative Indicators

**✅ Success signals:**
- "I checked the watch before deciding to go outside" (check-before-decide behavior)
- "Phase felt smooth, not jittery" (inertial, not reactive)
- "Metric surfaced right when I needed it" (intelligent surfacing)
- "Didn't notice rotation unless relevant" (invisible when mundane)

**❌ Failure signals:**
- "Watch kept switching faces randomly" (too frequent rotation)
- "Phase jumped around every minute" (jittery, not inertial)
- "Always showing boring metrics" (surfacing weights broken)
- "Battery died in 3 days" (power budget exceeded)

---

## 5. Risks & Mitigations

### Risk 1: Power Budget Exceeded

**Issue:** Phase Engine drains battery faster than 1 year target  
**Probability:** Medium (new sensor integration)  
**Impact:** High (user frustration, hardware limitation)  
**Mitigation:**
- Profile power consumption early (PR #72)
- Fallback: Reduce sensor sampling rates (1.6 Hz → 0.6 Hz if needed)
- Worst case: Disable continuous motion sampling, poll on button press only

---

### Risk 2: Jittery Phase / Zone Switching

**Issue:** Phase or zones change too frequently (< 5 min)  
**Probability:** Medium (tuning required)  
**Impact:** Medium (annoying UX, not broken)  
**Mitigation:**
- Research recommends 5-10 min debounce (already planned)
- Hysteresis prevents edge oscillation
- Dogfooding will reveal if 5 min → 10 min adjustment needed

---

### Risk 3: Playlist Implementation Complexity

**Issue:** Weighted surfacing algorithm too complex for 32 KB RAM / 230 KB flash  
**Probability:** Low (simple integer math)  
**Impact:** Medium (fallback to simpler rotation)  
**Mitigation:**
- Start with simplest version (affinity only, no anomaly tracking)
- Add complexity incrementally (PR #69 can be split)
- Worst case: Round-robin rotation within zone (still better than random)

---

### Risk 4: Sensor Drift / Calibration

**Issue:** LIS2DW12 or lux sensor drifts over time, metrics become inaccurate  
**Probability:** Low (hardware is stable)  
**Impact:** Medium (metrics feel "off")  
**Mitigation:**
- Use relative values (anomaly = deviation from baseline, not absolute)
- 5-min rolling averages smooth transient noise
- Dogfooding will reveal if recalibration needed

---

## 6. Open Questions (To Resolve During Implementation)

### Q1: Playlist State Storage

**Question:** Where does playlist state live? BKUP registers (battery-backed) or RAM?  
**Options:**
- **BKUP:** Survives sleep, limited to 16 bytes (4 registers)
- **RAM:** 32 KB available, lost on deep sleep

**Recommendation:** RAM for `playlist_state_t`, BKUP for zone persistence only (1 byte)

---

### Q2: Anomaly Detection Window

**Question:** How long should anomaly baseline window be?  
**Options:**
- 1 hour (60 samples @ 1/min) - fast adaptation
- 24 hours (1440 samples) - stable baseline
- 7 days (10080 samples) - seasonal stability

**Recommendation:** 24-hour window (matches circadian cycle), stored as running mean + stddev (8 bytes per metric)

---

### Q3: Manual Override Behavior

**Question:** When user presses MODE to switch faces, should playlist pause?  
**Options:**
- **Pause:** Manual selection disables auto-rotation until next zone change
- **Continue:** Auto-rotation resumes after dwell timer expires

**Recommendation:** Pause (user intent = "I want to see this face"), resume on next zone change

---

## 7. Documentation Deliverables

### Code Documentation

**Per PR:**
- Inline comments explaining algorithm choices
- Header file documentation (function contracts)
- Test cases with expected behavior

**Post-Phase 4:**
- `lib/phase/README.md` - Complete Phase Engine API guide
- `POWER_BUDGET.md` - Measured consumption breakdown
- `TUNING_GUIDE.md` - How to adjust parameters for different users

---

### User Documentation

**Builder UI:**
- Tooltip explanations for homebase, active hours, rotation settings
- Example configurations (early bird, night owl, default)

**Companion App:**
- Phase score interpretation guide
- Metric meaning explanations
- Troubleshooting (jittery phase, battery drain, etc.)

---

## 8. Timeline Estimate

| Phase | Duration | Deliverable |
|-------|----------|-------------|
| **4A: Sensors** | 1 week | LIS2DW12 + lux integrated (PRs #65-67) |
| **4B: Playlist** | 1 week | Intelligent rotation (PRs #68-70) |
| **4C: Integration** | 1 week | Display wiring + power audit (PRs #71-72) |
| **4D: Dogfooding** | 2-4 weeks | Behavioral validation + tuning |
| **Total** | **5-7 weeks** | Phase 4 complete |

**Parallel work opportunities:**
- Sensor integration (4A) can happen while playlist (4B) is planned
- Documentation can be written during dogfooding (4D)

---

## 9. Dependencies on Future Phases

**Phase 5 (Future):**
- JL (Joy/Lightness) metric - deferred from Phase 3
- Weekly comms (piezo TX, phototransistor RX) - sync data + homebase updates
- Advanced weight editor in Builder UI - per-zone metric customization
- Trend graphs / reflection - Phase 1→2→3→4 is all present-tense awareness

**Phase 4 scope constraint:** Present-tense agency only. No prediction, no trends, no reflection.

---

## 10. Approval Checklist

Before starting Phase 4A:
- [ ] Research findings reviewed (PHASE3_BASELINE_RESEARCH.md) ✅
- [ ] Tuning parameters documented (TUNING_PARAMETERS.md) ✅
- [ ] Power budget validated (<10 µA target achievable) ✅
- [ ] Behavioral patterns confirmed (70% clock, 5-min debounce) ✅
- [ ] Implementation plan reviewed and approved by dlorp
- [ ] First PR (#65: LIS2DW12 integration) ready to start

---

**Ready to proceed?** Phase 4A can begin once dlorp approves this plan.
