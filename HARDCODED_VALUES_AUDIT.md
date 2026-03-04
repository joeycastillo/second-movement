# Final Hardcoded Values Audit - Phase 4E/4F

**Date:** 2026-02-22  
**Branch:** phase4e-sleep-tracking-fresh  
**Status:** Pre-merge comprehensive review

---

## Category A: Algorithmic Constants ✅ (KEEP - Core Algorithm Design)

### Zone Boundaries (playlist.c:45-47)
- `phase_score <= 25` → EMERGENCE
- `phase_score <= 50` → MOMENTUM  
- `phase_score <= 75` → ACTIVE
- `else` → DESCENT
**Verdict:** KEEP - Core phase model design

### Metric Weights (metric_comfort.c, metric_em.c, metric_sd.c)
- Comfort: 28% temp + 32% light + 20% lunar + 20% sleep
- EM: 40% circadian + 20% lunar + 40% variance
- SD: 50% today + 30% yesterday + 20% day-before
**Verdict:** KEEP - Algorithm design (can tune in Phase 5 if needed)

### Cosine LUT (phase_engine.c, metric_em.c)
- 24-hour circadian curve values (-1000 to +1000)
**Verdict:** KEEP - Mathematical constant

### Lunar Cycle (metric_comfort.c, metric_em.c)
- 29-day approximation
- Peak at day 14.5 (full moon)
**Verdict:** KEEP - Astronomical constant

---

## Category B: Tunable Thresholds ⚠️ (REVIEW - Consider Making Configurable)

### Light Thresholds

**1. Expected Outdoor Light (phase_engine.c:120, Phase 4F context)**
- Current: `500` lux
- **Status:** ⚠️ Phase 4F plan mentioned making this configurable
- **Location:** Used in phase_compute() for light deviation
- **Recommendation:** Make configurable (Alaska winter ~200-300 lux)

**2. Daytime Minimum Light (metric_comfort.c:121)**
- Current: `200` lux
- **Status:** ⚠️ Should match outdoor_lux_min from Phase 4F
- **Recommendation:** Derive from configurable outdoor threshold

**3. Nighttime Maximum Light (metric_comfort.c:129)**  
- Current: `50` lux
- **Status:** ⚠️ Could be configurable as indoor_lux_min
- **Recommendation:** Make configurable (varies by bedroom setup)

### Temperature Threshold

**4. Maximum Temperature Deviation (phase_engine.c:110, metric_comfort.c:104)**
- Current: `300` (30°C deviation)
- **Status:** ⚠️ Reasonable for most climates
- **Recommendation:** Consider making configurable for extreme climates

### Activity Thresholds

**5. Energy Activity Divisor (metric_energy.c:63)**
- Current: `ENERGY_ACTIVITY_DIVISOR 50`
- **Status:** ⚠️ Generic threshold
- **Recommendation:** Phase 4F already implements WK baseline (7-day personalization) - this could follow same pattern

**6. Energy Max Bonus (metric_energy.c:64)**
- Current: `ENERGY_MAX_BONUS 20`
- **Status:** ⚠️ Algorithm parameter
- **Recommendation:** Keep for now, monitor during dogfooding

**7. WK Activity Bonus (metric_wk.c:37)**
- Current: `WK_ACTIVITY_BONUS 30`
- **Status:** ⚠️ Generic threshold  
- **Recommendation:** Phase 4F baseline tracking may make this adaptive

---

## Category C: Hardware/Physical Limits ✅ (KEEP - Based on Hardware)

### Battery Voltage (sleep_data.c:251)
- Maximum: `4200` mV (CR2016 fully charged)
- Minimum: `2000` mV (dead battery)
**Verdict:** KEEP - Hardware constraint

### Default Temperature (sensors.c:221)
- Fallback: `200` (20.0°C room temperature)
**Verdict:** KEEP - Reasonable fallback

### Default Sensor Values (metrics.c:67-71, 93, 95)
- Metrics: `50` (neutral midpoint)
- Temperature: `200` (20°C)
- Variance: `50` (moderate)
**Verdict:** KEEP - Safe fallback values when sensors unavailable

---

## Category D: Already Removed ✅ (DONE)

### Time-Based Hardcoding
- ❌ ~~Hardcoded daytime hours (6-18)~~ → Now uses homebase daylight
- ❌ ~~Hardcoded night hours (22-5)~~ → Now uses active hours
- ❌ ~~Hardcoded sleep start (23:00)~~ → Now uses active hours end
- ✅ All time-based logic now adaptive

---

## Recommendations by Priority

### Phase 4F (Before Merge) - OPTIONAL
These were mentioned in Phase 4F plan but deferred to "future Builder UI":

1. **Light Thresholds** (outdoor_lux_min, indoor_lux_min)
   - Current: Hardcoded 500/200/50 lux
   - Benefit: Alaska winter calibration (200-300 lux vs 500)
   - Effort: Medium (add config params, wire to phase_engine + metric_comfort)
   - **Decision:** DEFER to Phase 5 (use defaults for initial dogfooding)

2. **Temperature Max Deviation**
   - Current: Hardcoded 30°C
   - Benefit: Extreme climate adaptation
   - Effort: Low
   - **Decision:** DEFER to Phase 5 (30°C reasonable for most)

### Phase 5 (Post-Dogfooding)
After collecting real data:

1. **Metric Weights** - May need tuning based on dogfooding feedback
2. **Activity Thresholds** - WK baseline already adaptive, but energy divisor could be tuned
3. **Light Thresholds** - Add Builder UI for outdoor/indoor lux configuration

---

## Summary

**Hardcoded values remaining:** 
- ✅ **Algorithmic constants:** 15+ (KEEP - core design)
- ⚠️ **Tunable thresholds:** 7 (REVIEW - consider Phase 5)
- ✅ **Hardware limits:** 4 (KEEP - physical constraints)

**Critical findings:**
- ✅ No time-based hardcoding (all removed)
- ⚠️ Light thresholds (500/200/50 lux) could be configurable for Alaska
- ✅ Most values are reasonable defaults for initial dogfooding

**Recommendation for merge:**
- ✅ **Safe to merge as-is** - All critical hardcoding removed
- ⚠️ **Note for dogfooding** - Monitor if 500 lux outdoor threshold feels too high for Alaska winter
- 📋 **Phase 5 backlog** - Add light threshold configuration to Builder UI

---

**Verdict:** ✅ READY TO MERGE

All time-based hardcoding eliminated. Remaining hardcoded values are either algorithmic constants (correct by design) or reasonable defaults that can be tuned in Phase 5 based on dogfooding data.
