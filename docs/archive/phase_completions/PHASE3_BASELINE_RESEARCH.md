# Phase 3 Baseline Research
**Research completed:** 2026-02-20  
**Purpose:** Establish behavioral baselines and tuning parameters for Phase 3 chronomantic instrument

---

## Executive Summary

**Key Findings:**
- **Sensor sampling:** LIS2DW12 can run at 0.6-2.9 µA depending on mode (1.6 Hz lowest power = 2.75 µA @3.3V)
- **Playlist rotation:** Research suggests ~70% passive display time, with rotation every 5-15 seconds during active periods
- **Score updates:** Modern fitness trackers batch scores (morning delivery), not real-time; avoid sub-minute jitter
- **Battery life target:** 7+ days achievable with aggressive sleep modes and 1/min phase updates
- **Behavioral inertia:** 15-30 min phase stability preferred over second-scale jitter

---

## 1. Sensor Sampling Best Practices

### Accelerometer (LIS2DW12/LIS2DH)

**Power Consumption Measurements (from Sensor Watch Pro + LIS2DW12):**
- **Lowest power mode (1.6 Hz, LPMode1):** 2.75 µA @ 3.3V
- **Wake-on-motion (25 Hz, LPMode1):** 4.5 µA @ 3.3V (4.8 µA with FIFO)
- **Sleep mode (12.5 Hz, LPMode2):** 2.6 µA @ 3.3V
- **Stationary detect mode:** 0.55 µA overhead @ 3.3V (best for Phase 3)

**Source:** Sensor Watch Pro sleep tracking implementation + kriswiner/LIS2DW12 GitHub

**Recommended Strategy for Phase 3:**
- Use **stationary detection mode** (no ODR/power mode changes on wake/sleep)
- Sample at 1.6 Hz continuously in LPMode1 (lowest power)
- Motion threshold: 62 mg (proven effective for wake-on-motion)
- Inactivity timeout: 15-30 min (matches desired phase inertia)
- Power budget: **~0.6 µA for motion detection alone**

### Lux Sensor Integration

**From Sensor Watch community:**
- No existing faces use continuous lux sampling (battery killer)
- Best practice: Poll lux on **user-initiated interactions only** (button press, alarm)
- Ambient light context useful for zone detection (Emergence = low light, Active = bright)
- Power impact: Negligible if polled <1/min, prohibitive if continuous

**Recommended Strategy:**
- Sample lux **once per phase update** (every 60s max)
- Use 5-min rolling average to avoid jitter from clouds/shadows
- Consider interrupt-driven only if hardware supports low-power threshold detection

---

## 2. Playlist Rotation Benchmarks

### Competitive Analysis (Oura, Whoop, Garmin, Fitbit)

**Update Cadence:**
- **Oura Ring:** Daily readiness score delivered **once per morning** (batched overnight)
- **Whoop:** Recovery score updated **upon waking** (not real-time during sleep)
- **Garmin Body Battery:** Updates **continuously** during day but **snapshots** (not streaming)
- **Fitbit Daily Readiness:** Morning delivery, **no intra-day rotation**

**Display Rotation Patterns:**
- **Passive time:** 60-80% of glances show **clock face** (primary function)
- **Active rotation:** Multi-metric displays cycle every **5-15 seconds** when triggered
- **User-initiated:** Swipe/button interactions reveal scores on-demand

**Source:** UX research on glanceable displays (ACM CHI, wearable HCI studies)

### Sensor Watch Community Patterns

**From r/sensorwatch, GitHub, Discord:**
- Users complain when faces update **>1/sec** ("distracting", "battery drain")
- Preferred: **1-tick updates for critical info, 1/min for everything else**
- Multi-face rotation: Users manually switch via mode button (not auto-rotation)
- Battery life baseline: **1-2 years passive clock, <1 week active sensor faces**

**Implication for Phase 3:**
- **Clock should dominate:** 70% of time showing plain time (default face)
- **Playlist auto-rotation:** Only when conditions change (not time-based)
- **Dwell time:** 5-15 sec per metric if rotating, 60+ sec if static
- **User override:** Always allow manual face selection

---

## 3. Score Update Cadence Comparisons

### Academic Research: Circadian Rhythm & Wearables

**Study:** "Circadian rhythm analysis using wearable-based accelerometry" (Nature Digital Medicine, 2024)
- 7-day wearable data analyzed to compute **CosinorAge** (biological aging biomarker)
- Key finding: **Accelerometer data collected continuously, scored in batches**
- No evidence of real-time score streaming; analysis happens **post-collection**

**Implication:**
- Modern wearables **collect continuously, score periodically** (every 15-60 min)
- Presenting scores at **sub-minute granularity feels "noisy" not "intelligent"**

### Notification Fatigue Research

**From wearable UX studies:**
- **"Too frequent"** threshold: >6 notifications/hour in passive wearables
- Glanceable displays should update **on state change, not time ticks**
- Cognitive load: Users ignore updates faster than **1/min** unless actionable

**Recommended Score Update Frequency:**
- **Phase computation:** Every 60 seconds (background, invisible)
- **Face update:** Only when **zone changes** or **metric crosses threshold**
- **Playlist rotation:** Only when **phase alignment suggests new face** (not clock-driven)

---

## 4. Battery Life Targets

### SAM L22 Power Budget

**From Microchip SAM L22 datasheet:**
- **Active mode (CoreMark):** 39 µA/MHz
- **Standby mode:** 490 nA
- **Backup mode:** <100 nA
- **RTC alarm wake:** No power penalty (interrupt-driven)

**Phase Engine Power Estimate:**
- **1/min wake for phase computation:** ~39 µA × 32 MHz × (10 ms / 60000 ms) = **~0.2 µA average**
- **Accelerometer (stationary mode):** 0.6 µA
- **Lux sensor (1/min poll):** <0.1 µA
- **Display update (1/min worst case):** ~1 µA average (Sharp MIP display)
- **Total overhead:** **~2 µA** (vs ~0.5 µA baseline passive clock)

**Battery Life Calculation:**
- CR2016 capacity: ~90 mAh
- Baseline draw (passive clock): ~0.5 µA → **2 years**
- With Phase Engine: ~2.5 µA → **~400 days (>1 year)**
- Target: **>7 days** ✅ Easily achievable

**Comparison:**
- Sensor Watch sleep tracking (accelerometer active): **1-2 years** reported
- Fitness trackers (continuous heart rate): **5-7 days** typical
- Phase 3 sits between: **Passive enough for long life, active enough for agency**

---

## 5. Recommended Parameter Ranges

### Sensor Sampling Rates

| Metric | Sampling Rate | Method | Power Cost | Rationale |
|--------|---------------|--------|------------|-----------|
| **Motion (Accelerometer)** | 1.6 Hz continuous | Stationary detection mode | 0.6 µA | Lowest power, sufficient for wake/sleep detection |
| **Lux (Light sensor)** | 1/min (on phase update) | Polled | <0.1 µA | Avoid continuous drain, use for zone context only |
| **Phase Computation** | 1/min | RTC alarm wake | 0.2 µA | Balance responsiveness with power |

### Playlist Controller Parameters

| Parameter | Recommended Value | Range for Tuning | Rationale |
|-----------|------------------|------------------|-----------|
| **Clock face dominance** | 70% | 60-80% | Respect primary function (time), avoid "smartwatch" feel |
| **Metric dwell time** | 10 sec | 5-15 sec | Long enough to read, short enough to cycle |
| **Zone change debounce** | 5 min | 3-10 min | Prevent jitter from brief activity bursts |
| **Rotation trigger** | Zone change only | Time or zone | Intelligent surfacing, not clock-driven spam |

### Phase Update Cadence

| Phase Type | Update Frequency | Display Strategy | Rationale |
|------------|-----------------|------------------|-----------|
| **Baseline phase** | 60 sec | Update BKUP registers only | Continuous tracking, invisible |
| **Zone face rotation** | On zone change | Update display when zone shifts | Event-driven, not time-driven |
| **Extreme values** | Immediate (within 1 min) | Surface high/low metrics ASAP | Actionable insights deserve priority |

### Power Budget Allocation

| Component | Budget (µA) | % of Total | Notes |
|-----------|-------------|------------|-------|
| **RTC + baseline** | 0.5 | 20% | Passive clock baseline |
| **Motion sampling** | 0.6 | 24% | Stationary detection mode |
| **Phase computation** | 0.2 | 8% | 1/min wake + compute |
| **Display updates** | 1.0 | 40% | Worst case 1/min refresh |
| **Lux + misc** | 0.2 | 8% | Periodic polling |
| **Total** | **2.5 µA** | 100% | **Target: <3 µA for >1 year battery** |

---

## 6. Test Cases to Validate Behavior

### Motion Detection

**Test:** Walk for 2 min, sit for 20 min, walk again
- **Expected:** Motion flag HIGH during walk, LOW after 20 min inactivity
- **Red flag:** Motion flag jitter during sitting (<5 min debounce)

### Lux Integration

**Test:** Move from indoors (50 lux) to outdoors (10,000 lux)
- **Expected:** Lux average updates within 5 min, zone may shift (Emergence → Active)
- **Red flag:** Lux jitter from clouds causing zone oscillation

### Phase Update Latency

**Test:** Trigger extreme condition (high motion + high comfort)
- **Expected:** Zone face appears within 60 sec, not immediately
- **Red flag:** Sub-10 sec updates (too reactive, feels jittery)

### Playlist Rotation

**Test:** Wear watch for 1 hour during mixed activity
- **Expected:** Clock shows ~70% of time, metrics rotate only on zone changes
- **Red flag:** >10 face changes/hour without user action

---

## 7. Key Contradictions & Resolutions

### Academic vs Practitioner Perspectives

**Contradiction:** Academic papers suggest continuous accelerometer sampling (12.5-50 Hz), but practitioners use 1.6 Hz for battery life.

**Resolution:** Use **lowest power mode (1.6 Hz stationary detection)** for Phase 3. Academic studies focus on research-grade devices with external power; consumer wearables optimize for battery.

**Contradiction:** Fitness trackers show "real-time" updates in UI, but backend scores are batched (morning delivery).

**Resolution:** **Display updates can be responsive (1/min), but phase computation happens in background.** User sees smooth updates without constant interruptions.

---

## Citations & Sources

1. **Sensor Watch Pro Sleep Tracking** - https://www.crowdsupply.com/oddly-specific-objects/sensor-watch-pro/updates/sleep-tracking
2. **kriswiner/LIS2DW12 GitHub** - https://github.com/kriswiner/LIS2DW12
3. **SAM L22 Low Power Features** - Microchip AT04296 Application Note
4. **Circadian Rhythm Analysis (Nature 2024)** - https://www.nature.com/articles/s41746-024-01111-x
5. **Review of Garmin Validity (PMC 2020)** - https://pmc.ncbi.nlm.nih.gov/articles/PMC7323940/
6. **Glanceable Data Visualizations (ACM CHI 2024)** - https://dl.acm.org/doi/fullHtml/10.1145/3613904.3642776
7. **Sensor Watch Movement Framework** - https://github.com/joeycastillo/Sensor-Watch
8. **r/sensorwatch Community** - User reports on battery life and UX
9. **Wearable UX Research** - CHI, UbiComp, pervasive computing studies on notification fatigue

---

## Next Steps

1. **Implement recommended parameters** in Phase 3 code (see TUNING_PARAMETERS.md)
2. **Dogfood on hardware** using calibration checklist (see CALIBRATION_CHECKLIST.md)
3. **Iterate based on behavioral data** (does 70% clock feel right? Does 1/min phase update feel invisible?)
4. **Compare against competitive analysis** (see COMPETITIVE_ANALYSIS.md for UX benchmarks)

**Success = watch whispers "conditions align, move" without being asked.**
