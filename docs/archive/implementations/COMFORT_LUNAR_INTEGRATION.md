# Comfort Metric: Lunar Cycle Integration (Esoteric Interpretation)

> **Goal:** Integrate lunar cycles into Comfort as a measure of cosmic/natural rhythm alignment — preserving the mystical character of the chronomantic instrument.

---

## Philosophical Foundation

The watch is a **personal chronomantic instrument** — it measures not just physical comfort, but your **alignment with natural and cosmic cycles**.

**Lunar influence on environment (historical/esoteric perspective):**
- **Tidal rhythms:** The moon governs ocean tides (gravitational pull on water)
- **Agricultural timing:** Centuries of lunar planting calendars (full moon = germination, new moon = root growth)
- **Atmospheric pressure:** Subtle lunar effects on barometric pressure (debated, but observed by some)
- **Animal behavior:** Nocturnal activity peaks, mating cycles, migration patterns synchronized to lunar phases
- **Human sleep:** Anecdotal reports of disrupted sleep near full moon (weak scientific evidence, strong cultural belief)

**Key insight:** Lunar cycles don't directly affect temperature or light levels, but they represent **alignment with natural rhythms** — the same cosmic forces that govern tides, seasons, and biological cycles.

---

## Esoteric Interpretation: "Cosmic Environmental Alignment"

**Comfort metric becomes:**
- **Temperature (50%):** Physical thermal comfort relative to seasonal norms
- **Light (30%):** Physical light comfort relative to hour-of-day expectations
- **Lunar (20%):** **Cosmic rhythm alignment** — are you in tune with the celestial cycle?

**The lunar component measures:**
- Not "does the moon make me physically comfortable?"
- But "am I aligned with the same cosmic forces that govern nature's rhythms?"

**Why this matters for a chronomantic instrument:**
- You're not just tracking Human × Environment × Season
- You're tracking Human × Environment × **Cosmos**
- The moon is the visible, measurable manifestation of cosmic cycles

---

## Current EM Metric

**Range:** 0 = low comfort / misalignment, 100 = high comfort / alignment

**Components (before integration):**
- **Temperature (60%):** Current temp vs homebase seasonal norm
- **Light (40%):** Current lux vs hour-of-day expectation

**Data sources:** Thermistor, lux sensor (Pro board), homebase seasonal table

---

## Enhanced Comfort Metric (With Lunar)

**New breakdown:**
- **Temperature (50%):** Physical thermal comfort ← reduced from 60%
- **Light (30%):** Physical light comfort ← reduced from 40%
- **Lunar (20%):** Cosmic rhythm alignment ← **NEW**

---

## Lunar Component Formula

**Algorithm (unchanged from EM implementation):**
```c
int16_t calculate_lunar_alignment(uint16_t day_of_year) {
    // Approximate 29-day lunar cycle using day-of-year modulo
    // Full lunar calculation is overkill — we just need cyclic influence
    uint16_t lunar_phase = ((day_of_year * 1000) / 29) % 1000;
    
    // Score peaks near full moon (phase = 500)
    // Full moon = maximum cosmic alignment (100/100)
    // New moon = minimum alignment (0/100)
    // Formula: 100 - abs(lunar_phase - 500) / 5
    int16_t deviation = abs((int16_t)lunar_phase - 500);
    uint8_t alignment = 100 - (deviation / 5);
    
    return alignment;  // 0-100 scale
}
```

**Interpretation:**
- **Full moon (phase ≈ 500):** Maximum cosmic alignment score (100)
- **New moon (phase ≈ 0 or 1000):** Minimum alignment score (0)
- **First/third quarter:** Moderate alignment (50)

**Why peak at full moon:**
- Brightest night sky (historically significant for nocturnal activity)
- Strongest tidal pull (gravitational peak)
- Cultural symbolism of "fullness" and completion
- Agricultural tradition (harvest under full moon)

---

## Updated Comfort Calculation

**Full formula:**
```c
uint8_t calculate_comfort(
    int16_t current_temp_c10,
    uint16_t current_lux,
    uint16_t day_of_year,
    uint8_t hour,
    const homebase_entry_t* baseline
) {
    // 1. Temperature comfort (50%)
    int16_t temp_dev = abs(current_temp_c10 - baseline->avg_temp_c10);
    uint8_t temp_comfort = (temp_dev > 300) ? 0 : (100 - (temp_dev / 3));
    
    // 2. Light comfort (30%)
    uint16_t expected_lux = (hour >= 6 && hour < 18) ? 500 : 50;
    int16_t light_dev = abs((int16_t)current_lux - (int16_t)expected_lux);
    uint8_t light_comfort = (light_dev > 1000) ? 0 : (100 - (light_dev / 10));
    
    // 3. Lunar alignment (20%) - NEW
    uint8_t lunar_alignment = calculate_lunar_alignment(day_of_year);
    
    // Weighted blend
    uint16_t weighted_sum = (temp_comfort * 50) + (light_comfort * 30) + (lunar_alignment * 20);
    uint8_t comfort = weighted_sum / 100;
    
    return comfort;  // 0-100 scale
}
```

---

## Behavioral Examples

### Full Moon, Indoor Comfort (Phase ≈ 500)

**Conditions:**
- Temperature: 72°F (22°C) — perfect match to seasonal norm
- Light: 300 lux indoors — moderate
- Day of year: 15 (lunar phase ≈ 517, near full moon)

**Comfort breakdown:**
- **Temperature (50%):** Perfect match → 95/100 → contributes 47.5
- **Light (30%):** Moderate indoor light → 70/100 → contributes 21
- **Lunar (20%):** Full moon (high cosmic alignment) → 97/100 → contributes 19.4
- **Total Comfort:** ~88/100

**User interpretation:** "Physical conditions good, and I'm aligned with the cosmic cycle (full moon). Favorable time for activity."

---

### New Moon, Indoor Comfort (Phase ≈ 0)

**Conditions:**
- Temperature: 72°F (22°C) — perfect match to seasonal norm
- Light: 300 lux indoors — moderate
- Day of year: 1 (lunar phase ≈ 34, near new moon)

**Comfort breakdown:**
- **Temperature (50%):** Perfect match → 95/100 → contributes 47.5
- **Light (30%):** Moderate indoor light → 70/100 → contributes 21
- **Lunar (20%):** New moon (low cosmic alignment) → 7/100 → contributes 1.4
- **Total Comfort:** ~70/100

**User interpretation:** "Physical conditions same as above, but cosmic alignment is low (new moon). Less favorable time — consider rest/introspection rather than peak activity."

**The shift:** Same physical environment, ~18 point comfort swing based purely on lunar phase. This is the esoteric element — it's about **cosmic timing**, not physical comfort.

---

## The Esoteric Narrative

**What you're telling the wearer:**

"Your comfort isn't just about temperature and light. It's about your **alignment with the natural world** — the same forces that govern tides, seasons, and the rhythms of life.

When the moon is full, you're at peak alignment with cosmic cycles. Historically, this was when humans hunted under moonlight, when tides were strongest, when plants reached full bloom. Your ancestors lived by these rhythms.

When the moon is new, you're at minimum alignment. Historically, this was a time for rest, introspection, planning. The night sky is darkest, tides are weakest, nature turns inward.

This isn't about superstition — it's about **remembering that you're part of a larger cosmic system**. The same gravitational forces that move oceans also influence you. The same cycles that govern plant growth also govern your biology.

Your chronomantic instrument tracks this. Not because the moon makes you physically comfortable, but because **comfort includes cosmic alignment** — being in tune with the rhythms that existed long before clocks."

---

## Integration Benefits

**Why this works:**
1. **Preserves mystical character:** Lunar stays in the system (essential for chronomantic instrument)
2. **Conceptually defensible:** Comfort = environmental + cosmic alignment (not just physical)
3. **Daily variation:** Comfort varies ~20 points over lunar cycle (keeps metric dynamic)
4. **Cultural resonance:** Ties to millennia of lunar calendars, agricultural timing, tidal rhythms
5. **Actionable (esoterically):** Low lunar comfort → Consider timing activities around cosmic cycles

**User experience:**
- **Full moon:** Comfort boosted 15-20 points (cosmic alignment peak)
- **New moon:** Comfort reduced 15-20 points (cosmic alignment low)
- **Physical environment unchanged:** The shift is purely cosmic/temporal

---

## Implementation Notes

**Lunar phase approximation:**
- Precise moon phase calculation requires complex astronomy (sun/moon/earth geometry)
- Approximation using `(day_of_year % 29)` is sufficient for symbolic representation
- If user wants precise lunar phase → use `moon_phase_face` (that's what it's for)

**Update frequency:**
- Lunar alignment changes slowly (once per day update is sufficient)
- Cache calculation result in Comfort metric state

**Power budget:**
- Calculation cost: ~20 integer operations = negligible (<0.01 µA)
- No additional sensor reads required

---

## Next Steps

**Implementation:**
1. Add `calculate_lunar_alignment()` to `lib/metrics/metric_comfort.c`
2. Update `metric_comfort_update()` to include lunar component (20% weight)
3. Adjust temp/light weights (50%/30% from 60%/40%)

**Documentation:**
- Update README Comfort section to explain lunar integration
- Emphasize esoteric/cosmic interpretation
- Distinguish from precise moon phase calculation (moon_phase_face)

**User-facing explanation:**
"Comfort tracks your alignment with environment AND cosmos. The lunar component measures your position in natural cycles — the same forces that govern tides and seasons. Peak at full moon (cosmic alignment high), lowest at new moon (time for introspection)."
