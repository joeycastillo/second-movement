# EM Metric: Seasonal Affect Disorder (SAD) Integration

> **Goal:** Enhance EM (Emotional) metric with latitude-based seasonal mood expectations to account for Seasonal Affective Disorder patterns.

---

## Current EM Metric

**Range:** 0 = low/disrupted mood, 100 = aligned/elevated mood

**Components (before integration):**
- **Circadian (40%):** Hour-of-day cosine curve (peaks 2 PM, troughs 2 AM)
- **Lunar (20%):** 29-day cycle approximation (peaks near full moon)
- **Variance (40%):** Activity variance penalty (low activity when expected high, or vice versa)

**Data sources:** Hour-of-day, day-of-year, accelerometer variance

---

## Enhanced EM Metric (With SAD)

**New breakdown:**
- **Circadian (40%):** Hour-of-day mood rhythm ← unchanged
- **Seasonal (20%):** Latitude-adjusted SAD baseline ← **NEW** (replaces Lunar)
- **Variance (40%):** Activity-mood mismatch ← unchanged

---

## Seasonal Component Formula

**Algorithm:**
```c
int16_t calculate_seasonal_comfort(uint16_t day_of_year, int16_t latitude) {
    // Normalize latitude to 0-90 range (absolute value)
    uint16_t abs_lat = (latitude < 0) ? -latitude : latitude;
    
    // Determine hemisphere
    bool northern_hemisphere = (latitude >= 0);
    
    // Winter solstice (nadir of seasonal comfort)
    uint16_t winter_solstice = northern_hemisphere ? 355 : 172;  // Dec 21 or Jun 21
    
    // Days from winter solstice (0-182, using shorter half-year)
    int16_t days_from_solstice = abs((int16_t)day_of_year - (int16_t)winter_solstice);
    if (days_from_solstice > 182) {
        days_from_solstice = 365 - days_from_solstice;
    }
    
    // Seasonal base: 0 at winter solstice, 100 at summer solstice
    // Linear interpolation: score = days_from_solstice * 100 / 182
    uint16_t seasonal_base = (days_from_solstice * 100) / 182;
    
    // Apply latitude severity (0° = no SAD effect, 90° = maximum SAD effect)
    // Severity = latitude / 90 (normalized to 0-100%)
    uint16_t severity = (abs_lat * 100) / 90;
    
    // Seasonal score with latitude weighting:
    // Equator (0°): stays near 50 all year (minimal seasonal variation)
    // Poles (90°): swings from 0 (winter) to 100 (summer)
    // Formula: 50 + (seasonal_base - 50) * severity / 100
    int16_t deviation = seasonal_base - 50;
    int16_t seasonal_score = 50 + ((deviation * severity) / 100);
    
    return seasonal_score;  // 0-100 scale
}
```

**Example outputs:**

| Location | Latitude | Dec 21 (winter) | Jun 21 (summer) | Mar 21/Sep 21 (equinox) |
|----------|----------|-----------------|-----------------|-------------------------|
| **Singapore** | 1°N | 49 | 51 | 50 |
| **Anchorage** | 61°N | 32 | 68 | 50 |
| **New York** | 41°N | 38 | 62 | 50 |
| **Melbourne** | -38°S | 62 | 38 | 50 |

---

## Updated EM Calculation

**Full formula:**
```c
uint8_t calculate_em(
    int16_t current_temp_c10,
    uint16_t current_lux,
    uint16_t day_of_year,
    uint8_t hour,
    int16_t latitude,
    const homebase_entry_t* baseline
) {
    // 1. Circadian rhythm (40%)
    int16_t circadian_curve = cosine_lut_24[hour];  // -1000 to +1000
    uint8_t circadian_score = (circadian_curve + 1000) / 20;  // Map to 0-100
    
    // 2. Seasonal affect (20%) - NEW
    uint8_t seasonal_score = calculate_seasonal_affect(day_of_year, latitude);
    
    // 3. Activity variance (40%)
    uint8_t variance_penalty = calculate_variance_penalty(activity_variance, expected_activity);
    uint8_t variance_score = 100 - variance_penalty;
    
    // Weighted blend
    uint16_t weighted_sum = (circadian_score * 40) + (seasonal_score * 20) + (variance_score * 40);
    uint8_t em = weighted_sum / 100;
    
    return em;  // 0-100 scale
}
```

---

## Behavioral Examples

### Winter in Anchorage (Dec 21, 61°N)

**Conditions:**
- Temperature: 15°F (-9°C) — expected for winter
- Light: 200 lux indoors (short daylight hours)
- Day of year: 355

**Comfort breakdown:**
- **Temperature (50%):** Good match to seasonal norm → 85/100 → contributes 42.5
- **Light (30%):** Low light during day hours → 60/100 → contributes 18
- **Seasonal (20%):** Winter at high latitude → 32/100 → contributes 6.4
- **Total Comfort:** ~67/100

**Interpretation:** Reasonably comfortable for winter conditions, but seasonal baseline is low (SAD factor). User might benefit from bright light therapy.

---

### Summer in Anchorage (Jun 21, 61°N)

**Conditions:**
- Temperature: 65°F (18°C) — expected for summer
- Light: 5000 lux outdoors (long daylight hours)
- Day of year: 172

**Comfort breakdown:**
- **Temperature (50%):** Good match → 90/100 → contributes 45
- **Light (30%):** Excellent light exposure → 95/100 → contributes 28.5
- **Seasonal (20%):** Summer at high latitude → 68/100 → contributes 13.6
- **Total Comfort:** ~87/100

**Interpretation:** High comfort — optimal environmental conditions for high latitude summer.

---

### Winter in Singapore (Dec 21, 1°N)

**Conditions:**
- Temperature: 80°F (27°C) — tropical, no winter
- Light: 5000 lux outdoors (near-equator, consistent daylight)
- Day of year: 355

**Comfort breakdown:**
- **Temperature (50%):** Match to tropical norm → 88/100 → contributes 44
- **Light (30%):** Excellent → 95/100 → contributes 28.5
- **Seasonal (20%):** Near equator, minimal SAD → 49/100 → contributes 9.8
- **Total Comfort:** ~82/100

**Interpretation:** High comfort year-round (no SAD effect at equator).

---

## Integration Benefits

**Why this works:**
1. **Conceptually coherent:** Comfort = environmental alignment, including seasonal expectations
2. **Personalized:** Uses homebase latitude (already configured)
3. **Evidence-based:** SAD research strongly supports latitude-seasonal correlation
4. **Actionable:** Low seasonal score in winter → prioritize bright light exposure
5. **Preserves EM's esoteric character:** Lunar stays in EM (mystical), SAD moves to Comfort (scientific)

**User experience:**
- **Winter at high latitude:** Comfort naturally lower (seasonal factor 20-40) → surfaces more often → reminds user to get light
- **Summer at high latitude:** Comfort naturally higher (seasonal factor 60-80) → less urgent
- **Equator:** Minimal seasonal variation (48-52 all year) → no SAD interference

---

## Implementation Notes

**Homebase latitude source:**
- Already stored in homebase configuration (user sets during web builder setup)
- Example: Anchorage = 61.2181°N → stored as `latitude = 612` (tenths of a degree)

**Update frequency:**
- Seasonal comfort changes slowly (once per day is sufficient)
- Cache calculation result in Comfort metric state
- Recompute only when day_of_year increments

**Power budget:**
- Calculation cost: ~100 integer operations = negligible (<0.01 µA)
- No additional sensor reads required (uses existing data)

---

## Testing Scenarios

**Scenario 1: Anchorage winter (SAD-prone)**
- Day: Dec 21, Latitude: 61°N
- Expected seasonal comfort: 32/100
- Overall comfort likely 50-70 (temp/light may be okay, but seasonal baseline drags it down)
- User interpretation: "Conditions challenging for this time of year, prioritize light exposure"

**Scenario 2: Anchorage summer (SAD-optimal)**
- Day: Jun 21, Latitude: 61°N
- Expected seasonal comfort: 68/100
- Overall comfort likely 75-90 (temp/light/seasonal all aligned)
- User interpretation: "Great conditions, make the most of summer daylight"

**Scenario 3: Equator year-round (no SAD)**
- Any day, Latitude: 0°
- Expected seasonal comfort: 49-51/100 (minimal variation)
- Overall comfort driven entirely by temp/light (consistent year-round)
- User interpretation: Seasonal factor irrelevant (as expected)

---

## Next Steps

**Implementation:**
1. Add `calculate_seasonal_comfort()` to `lib/metrics/metric_comfort.c`
2. Update `metric_comfort_update()` to include seasonal component (20% weight)
3. Adjust temp/light weights (50%/30% from 60%/40%)
4. Add latitude parameter to comfort calculation (from homebase)

**Documentation:**
- Update README Comfort section to mention seasonal component
- Explain SAD integration in user-facing docs

**Testing:**
- Verify seasonal scores at various latitudes (0°, 30°, 60°, 90°)
- Confirm hemisphere detection (northern vs southern winter)
- Validate smooth transitions throughout year
