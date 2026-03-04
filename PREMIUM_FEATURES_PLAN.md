# Premium Watch Features - Implementation Plan

Comprehensive plan for implementing premium Casio-inspired features on Sensor Watch.

**Status:** Updated 2026-02-22 based on existing capabilities and roadmap alignment

---

## Part 1: MR-G Lighting Suite (Phase 4.5)

### 1.1 LED Fade Animation ✨ PRIORITY
**Status:** Ready to implement NOW  
**Reference:** Casio MR-G MRG-B5000B-1 smooth fade effect

**Implementation:**
- **Fade-in:** 300ms (20 steps @ 64 Hz), quadratic gamma curve
- **Fade-out:** 500ms (32 steps @ 64 Hz), slower for premium feel
- **Hardware:** Existing TCC0 PWM (watch_set_led_color_rgb 0-255 brightness)
- **Timer:** TC0 reuse (same pattern as buzzer sequences)
- **Math:** Integer-only quadratic: `(step² × target) / N²`

**Files to modify:**
- `movement/movement.c` - Hook fade into illuminate/force_off functions
- `watch-library/shared/watch/watch_led.c` - Fade state machine (optional)

**Resource impact:**
- Flash: ~250 bytes
- RAM: 7 bytes (fade state)
- Guard: `#ifdef SMOOTH_LED_FADE`

**Details:** See `LED_FADE_IMPLEMENTATION_PLAN.md`

---

### 1.2 Auto-Light (Tilt-to-Illuminate)
**Status:** Phase 4.5  
**Reference:** MR-G auto LED light function

**Implementation:**
- **Trigger:** LIS2DW 6D orientation interrupt (wrist-up detection)
- **Gate:** TSL2561 ambient light < 50 lux threshold
- **Action:** Activate existing LED fade-in sequence
- **False-positive prevention:**
  - 3-second cooldown timer between activations
  - Time window filter (only 18:00-06:00 configurable)
  - Activity filter (skip during intense motion)
  - Hysteresis on orientation change

**Files to modify:**
- `lib/phase/sensors.c` - LIS2DW orientation interrupt config
- `movement/movement.c` - Auto-light activation logic
- `watch-faces/sensor/settings_face.c` - Enable/disable toggle

**Resource impact:**
- Flash: ~500 bytes
- RAM: 8 bytes (cooldown state)
- Power: +1 µA standby (orientation detection active)
- Guard: `#ifdef AUTO_LIGHT_ENABLED`

---

### 1.3 Display Power-Save
**Status:** Phase 4.5  
**Reference:** MR-G power-saving display off in darkness

**Implementation:**
- **Trigger:** Ambient light == 0 lux for 30 minutes
- **Action:** Disable SLCD peripheral (SAM L22 LCD controller)
- **Wake:** Any button press, motion interrupt, or light detection
- **Power savings:** ~2-3 µA continuous

**Files to modify:**
- `movement/movement.c` - Darkness timer, SLCD disable/enable
- `watch-library/shared/watch/watch_slcd.c` - Add peripheral enable/disable API

**Resource impact:**
- Flash: ~300 bytes
- RAM: 4 bytes (darkness timer)
- Power: -2.5 µA (net savings)
- Guard: `#ifdef DISPLAY_POWER_SAVE`

---

### 1.4 Ambient-Adaptive LED Brightness
**Status:** Phase 4.5

**Implementation:**
- Map TSL2561 reading to LED brightness: 0-10 lux → 30%, 10-100 → 60%, 100+ → 100%
- Apply during fade-in (target brightness varies by ambient)
- Prevents blinding in dark, ensures visibility in bright

**Resource impact:**
- Flash: ~100 bytes (lookup table)
- RAM: 0 bytes (calculated on-demand)

---

### 1.5 Button Tone Variety
**Status:** Phase 4.5

**Implementation:**
- Premium beep profiles (short crisp tone vs standard)
- Different tones for different button actions
- Uses existing buzzer infrastructure

**Resource impact:**
- Flash: ~150 bytes (tone sequences)
- RAM: 0 bytes

---

## Part 2: Premium Feature Catalog

### Tier 1: Quick Wins (Phase 4.5-5)

| Feature | Status | Flash | RAM | Power | Notes |
|---------|--------|-------|-----|-------|-------|
| LED fade animation | **NOW** | 250 B | 7 B | 0 µA | MR-G signature effect |
| Auto-light | Phase 4.5 | 500 B | 8 B | +1 µA | Tilt-to-illuminate |
| Display power-save | Phase 4.5 | 300 B | 4 B | -2.5 µA | LCD off in darkness |
| Adaptive brightness | Phase 4.5 | 100 B | 0 B | 0 µA | Context-aware LED |
| Button tone variety | Phase 4.5 | 150 B | 0 B | 0 µA | Premium sound |
| Tap-to-wake | Phase 5 | 400 B | 6 B | +0.5 µA | Double-tap backlight |

**Total Tier 1:** ~1.7 KB flash, 25 bytes RAM

---

### Tier 2: Medium Features (Phase 6)

| Feature | Status | Flash | RAM | Power | Notes |
|---------|--------|-------|-----|-------|-------|
| Smooth digit transitions | Phase 6 | 800 B | 16 B | 0 µA | Animated number changes |
| Sunrise/sunset alarm | Phase 6 | 600 B | 12 B | 0 µA | Smart wake timing |
| Sensor fusion dashboard | Phase 6 | 1.2 KB | 24 B | 0 µA | Combined sensor display |

**Total Tier 2:** ~2.6 KB flash, 52 bytes RAM

---

### Tier 3: Aspirational (Phase 7+)

| Feature | Status | Flash | RAM | Power | Notes |
|---------|--------|-------|-----|-------|-------|
| Activity recognition | Phase 7+ | 2 KB | 32 B | +2 µA | Walking/running/cycling detection |

---

## Part 3: Implementation Roadmap

### Phase 4.5: MR-G Lighting Suite (1 week)
**Goal:** Premium LED experience + power optimization

**Week 1:**
- Day 1-2: LED fade animation (PR #71) ← **IMPLEMENT NOW**
- Day 3-4: Auto-light + adaptive brightness
- Day 5-6: Display power-save + button tones
- Day 7: Testing + documentation

**Deliverable:** Unified lighting settings page, ~1 KB flash total

---

### Phase 5: Polish (2 weeks)
- Tap-to-wake
- Smooth digit transitions (start)

---

### Phase 6: Advanced Features (3 weeks)
- Sensor fusion dashboard
- Sunrise/sunset alarms

---

### Phase 7+: Future
- Activity recognition (if needed)

---

## Part 4: Resource Summary

### Combined Impact (All Tier 1+2 Features)
- **Flash:** ~5.85 KB total
- **RAM:** ~75 bytes total
- **Power (idle):** +1.1 µA (with auto-light active)
- **Power (active use):** -2.5 µA (display power-save offsets auto-light)
- **Battery life impact:** -15-20% with auto-light enabled, partially offset by power-save

### Per-Feature Guards
All features behind `#ifdef` macros:
- `SMOOTH_LED_FADE`
- `AUTO_LIGHT_ENABLED`
- `DISPLAY_POWER_SAVE`
- `ADAPTIVE_LED_BRIGHTNESS`
- `PREMIUM_BUTTON_TONES`
- `TAP_TO_WAKE`
- `SMOOTH_DIGIT_TRANSITIONS`

**Zero impact when disabled.**

---

## Part 5: Removed Features

The following features were removed from the original research:

**❌ Gesture navigation** - Removed (not desired)  
**❌ Smart alarm** - Already implemented in existing firmware  
**❌ World clock intelligence** - Handled by unified comms system

---

## Notes

**Battery trade-off:** Auto-light is a power-hungry feature (+1 µA standby for orientation detection, frequent wakes in low light). Display power-save helps offset this. Users should be able to toggle auto-light if battery life is critical.

**Premium feel priority:**
1. LED fade (instant visual polish) ← **IMPLEMENT FIRST**
2. Auto-light (convenience + wow factor)
3. Adaptive brightness (invisible but premium)
4. Display power-save (battery care)

**Hardware capabilities fully utilized:**
- TCC0 PWM: LED brightness control (already implemented)
- TC0 timer: Animation timing (reusable from buzzer infrastructure)
- LIS2DW: Tap detection, orientation interrupts
- TSL2561: Ambient light sensing
- SAM L22 SLCD: Power control

**No hardware modifications required.** All features use existing peripherals.
