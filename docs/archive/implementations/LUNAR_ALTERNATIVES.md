# Lunar Influence in EM Metric: Alternatives

> **Current:** EM metric uses rough 29-day lunar cycle approximation (20% weight)  
> **Question:** Is this valuable, or should we replace it?

---

## Current Implementation

```c
// Lunar sub-score (20% of EM)
uint16_t lunar_phase = ((day_of_year * 1000) / 29) % 1000;
// Peaks near full moon (500)
uint8_t lunar_score = 100 - (abs(lunar_phase - 500) / 5);
```

**What it does:** Approximates a 29-day cycle peaking mid-cycle (roughly full moon).

**Why approximate:** `moon_phase_face` already shows precise lunar phase. EM just needs a cyclic influence score.

**Esoteric appeal:** Adds a mystical/natural cycles dimension unique to this watch.

---

## Research Context

**Evidence FOR lunar influence on mood:**
- Historical/cultural belief (millennia of lunar calendars)
- Anecdotal reports of sleep disruption near full moon
- Some studies show small effects on sleep quality

**Evidence AGAINST:**
- Meta-analyses find weak/inconsistent effects
- Most studies show no significant mood correlation
- Placebo/expectation effects likely confound results

**Verdict:** Scientifically weak, but culturally/symbolically meaningful.

---

## Alternative 1: Keep Lunar, Improve Explanation

**Rationale:** Embrace the esoteric. This isn't a medical device, it's a personal instrument.

**README update:**
```markdown
**Lunar (20%):** Approximates the 29-day lunar cycle using day-of-year modulo math. 
This is NOT the precise moon phase (see moon_phase_face for that) — it's a symbolic 
representation of monthly cyclical influence. Peaks mid-cycle (roughly full moon). 
Scientifically unproven, culturally resonant.
```

**Pro:** Keeps unique character, acknowledges it's symbolic not scientific  
**Con:** Takes up 20% of EM for something unproven

---

## Alternative 2: Replace with Barometric Pressure Trend

**What:** Track pressure changes (rising = better mood, falling = worse mood)

**How:**
- Read barometric pressure from sensor (if available on future hardware)
- Or: Use homebase data to estimate typical pressure for location/season
- Rising trend (+5 hPa over 3h) = mood boost
- Falling trend (-5 hPa over 3h) = mood penalty

**Research:** Barometric pressure changes correlate with mood in some studies (headaches, joint pain, malaise).

**Pro:** More plausible biological mechanism than lunar  
**Con:** Requires sensor (current Sensor Watch has no barometer)

---

## Alternative 3: Replace with Social Rhythm Metric

**What:** Track consistency of daily activity patterns (regularity improves mood)

**How:**
- Compute variance in wake times over last 7 days
- Low variance (consistent) = mood boost
- High variance (chaotic schedule) = mood penalty

**Research:** Social Rhythm Therapy (Frank et al.) shows schedule regularity improves mood in bipolar disorder.

**Pro:** Evidence-based, uses existing data (wake onset tracking)  
**Con:** Overlaps with SD (sleep debt) and WK (wake momentum)

---

## Alternative 4: Replace with Temperature Comfort History

**What:** Track if you've been consistently comfortable over last few days

**How:**
- Rolling 3-day comfort average
- High average (consistently comfortable) = mood boost
- Low average (persistently uncomfortable) = mood penalty

**Research:** Chronic discomfort impacts mood (well-established).

**Pro:** Uses existing sensors (thermistor, lux)  
**Con:** Overlaps with Comfort metric (already tracked separately)

---

## Alternative 5: Remove Lunar, Redistribute Weights

**New EM formula:**
- **Circadian (60%):** Hour-of-day cosine curve (increased from 40%)
- **Variance (40%):** Activity variance penalty (unchanged)

**Rationale:** Simplify. Focus on proven factors (circadian rhythm, activity mismatch).

**Pro:** Cleaner, more scientifically defensible  
**Con:** Loses esoteric/unique character

---

## Alternative 6: Replace with Seasonal Affect Factor

**What:** Weight based on day-of-year, dipping in winter (seasonal affective disorder)

**How:**
- Use homebase latitude to determine winter severity
- Northern latitudes: larger winter mood dip
- Southern latitudes: inverted (winter is mid-year)
- Equator: minimal seasonal variation

**Research:** SAD is well-documented, latitude-dependent.

**Pro:** Evidence-based, uses homebase data  
**Con:** Slow-changing (doesn't vary much day-to-day)

---

## Recommendation

**If keeping esoteric character:** **Alternative 1** (keep lunar, improve explanation)

**If prioritizing science:** **Alternative 5** (remove lunar, redistribute to circadian)

**If adding new sensor data:** **Alternative 2** (barometric pressure) or **Alternative 6** (seasonal affect)

**Current lean:** Keep lunar for now (Alternative 1), document it clearly as symbolic/cultural rather than scientific. The watch is a *chronomantic instrument*, not a clinical tool. Embrace the mysticism.

---

## Implementation Decision

**Decision (2026-02-21, revised):** **Option B - Reversed Configuration**

**EM (Emotional):** Circadian + **SAD** + Variance
- SAD is about mood/affect (belongs with emotional state)
- Evidence-based, actionable (low winter mood → bright light therapy)
- See EM_SAD_INTEGRATION.md

**Comfort:** Temp + Light + **Lunar**
- Lunar becomes "cosmic environmental alignment" (esoteric interpretation)
- Preserves mystical character of chronomantic instrument
- Not about physical comfort, but alignment with natural/celestial cycles
- See COMFORT_LUNAR_INTEGRATION.md

**Rationale:** SAD is fundamentally about emotional state (it's literally "Seasonal AFFECTIVE Disorder"). Moving it to EM makes scientific sense. Lunar shifts to Comfort to preserve esoteric element — interpreted as cosmic rhythm alignment rather than direct physical comfort.
