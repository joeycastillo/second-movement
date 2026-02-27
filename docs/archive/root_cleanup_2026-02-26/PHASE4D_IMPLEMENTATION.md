# Phase 4D Implementation Summary

**Date:** 2026-02-21  
**Branch:** phase4bc-playlist-dispatch  
**Build Status:** ✅ Successful (137,868 bytes text, 145,988 bytes total)

## Tasks Completed

### 1. ✅ Navigation Fix (Movement.c)
**File:** `movement/movement.c`  
**Lines:** 708-720

**Implementation:**
- Added MODE long-press handler to exit playlist mode
- Handler clears `playlist_mode_active` flag and returns to clock (face index 1)
- Integrated with existing EVENT_MODE_LONG_PRESS switch case

**Code:**
```c
case EVENT_MODE_LONG_PRESS:
#ifdef PHASE_ENGINE_ENABLED
    // Phase 4D: Long-press MODE from any face → exit playlist mode if active
    if (movement_state.playlist_mode_active) {
        movement_state.playlist_mode_active = false;
        movement_move_to_face(1);  // Return to clock (index 1)
        break;
    }
#endif
    // ... existing secondary face logic ...
```

### 2. ✅ Zone Display Face
**Files:** 
- `watch-faces/complication/zone_display_face.h` (1,315 bytes)
- `watch-faces/complication/zone_display_face.c` (4,592 bytes → simplified to ~3,500 bytes)

**Implementation:**
- Created unified zone display face that queries `playlist_get_current_face()` to determine which metric to show
- Displays metric abbreviation + value (SD/EM/WK/Enrgy/Comft + 0-100 value)
- ALARM button cycles through zone playlist via `playlist_advance()`
- MODE long-press returns to clock and exits playlist mode
- Registered 4× in `movement_config.h` at indices 2-5 (one per zone)

**Display Format:**
```
SD    42    (Metric abbrev + value)
          (Reserved for future trend display)
```

**Button Behavior:**
- ALARM: Cycles through zone playlist
- MODE long-press: Exits playlist mode, returns to clock
- LIGHT: Illuminates LED
- TIMEOUT: Returns to clock (stays in playlist mode)

### 3. ✅ Anomaly Detection
**Files:**
- `lib/phase/phase_engine.h` (added 35 lines)
- `lib/phase/phase_engine.c` (added 90 lines)

**Implementation:**
- **Algorithm:** `|metric - 50| ≥ 20` threshold detection (integer math)
- **Bitmask Flags:** 8-bit anomaly_flags field in `phase_state_t`
  - Sleep Debt: high/low (midpoint 30, threshold ±20)
  - Emotional: high/low (midpoint 50, threshold ±20)
  - Energy: high/low (midpoint 50, threshold ±20)
  - Comfort: high/low (midpoint 50, threshold ±20)
- **Active Hours Gating:** Only chimes during configured active hours (default 6-22)
  - Respects `movement_active_hours_t` from BKUP[2]
  - Handles wraparound (e.g., 22:00-06:00 sleep window)
- **Soft Chime:** C7 @ 80ms via `watch_buzzer_play_note()`
- **Deduplication:** Only chimes on *newly detected* anomalies (prevents spam)

**Integration:**
- Called from `movement.c::_movement_handle_top_of_minute()` after metrics update
- Runs every 15 minutes during metrics tick
- State stored in `movement_state.phase.anomaly_flags`

**Code:**
```c
void phase_detect_anomalies(phase_state_t *state,
                            int16_t sd, uint8_t em, uint8_t energy, uint8_t comfort,
                            uint8_t hour, bool active_hours_enabled,
                            uint8_t active_start, uint8_t active_end);
```

### 4. ✅ Lunar Integration
**Files:**
- `lib/metrics/metric_comfort.c` (added 42 lines)
- `lib/metrics/metric_comfort.h` (updated signature)

**Implementation:**
- **Conway Lunar Approximation:** ±1 day accuracy, ~40 lines integer-only code
  - Formula: `age = ((r * 11) % 30 + month * 2 + day) % 30` where `r = (year % 100) % 19`
  - Handles Jan/Feb edge case (treat as months 13/14 of previous year)
- **Lunar Comfort Score:** Maps 0-29 days to 0-100 comfort
  - Full moon (day 15) → 100 (peak comfort)
  - New moon (day 0/29) → 0 (lowest comfort)
  - Linear interpolation between phases
- **Weight:** 20% in Comfort metric (replaces 20% of Light weight)
  - **New blend:** 48% temperature + 32% light + 20% lunar (was 60% temp + 40% light)
- **Date Parameters:** Added `year`, `month`, `day` to `metric_comfort_compute()` signature
  - Updated caller in `metrics_update()` to extract from `watch_rtc_get_date_time()`

**Code:**
```c
// Conway's lunar approximation
uint8_t lunar_age_conway(uint16_t year, uint8_t month, uint8_t day);

// Lunar comfort score (0-100)
uint8_t lunar_comfort_score(uint8_t lunar_age);

// Updated function signature
uint8_t metric_comfort_compute(int16_t temp_c10, uint16_t light_lux, uint8_t hour,
                                const homebase_entry_t *baseline,
                                uint16_t year, uint8_t month, uint8_t day);
```

## Build Configuration

**Tested Build:**
```bash
make BOARD=sensorwatch_red DISPLAY=classic PHASE_ENGINE_ENABLED=1
```

**Build Output:**
```
   text	   data	    bss	    dec	    hex	filename
 137868	   2796	   5324	 145988	  23a44	build/firmware.elf
```

**Flash Budget:** ~138 KB used of 256 KB (53.9% utilization)  
**RAM Budget:** ~8 KB used of 32 KB (25% utilization)  
**New Code:** ~320 lines total (matches strategic plan estimate)

## Testing Checklist

- [x] Code compiles without errors
- [x] All functions #ifdef PHASE_ENGINE_ENABLED
- [x] Integer-only math (no floating point)
- [ ] Emulator test (navigation flow)
- [ ] Hardware test (anomaly chimes)
- [ ] Lunar accuracy verification (±1 day)
- [ ] Active hours gating (no chimes during sleep)

## Known Limitations

1. **Trend Display:** Zone display face shows metric value only (trend calculation deferred)
   - Metrics snapshot doesn't currently track historical values for trend calculation
   - Future enhancement: Add trend fields or compute from phase_history circular buffer

2. **Volatile Warnings:** Compiler warnings about discarding volatile qualifiers
   - Non-critical, consistent with existing codebase patterns
   - Functions expect non-volatile pointers but receive volatile movement_state members

## Next Steps

1. **Testing:** Build emulator version and verify navigation flow
2. **PR Submission:** Create pull request to #pull-requests channel
3. **Documentation:** Update lib/phase/README.md with Phase 4D features
4. **Future Enhancements:**
   - Add trend calculation to metrics system
   - Display trends on zone_display_face (line 2)
   - Tune anomaly thresholds based on real-world usage

## Files Modified

### New Files (3)
- `watch-faces/complication/zone_display_face.h`
- `watch-faces/complication/zone_display_face.c`
- `PHASE4D_IMPLEMENTATION.md` (this file)

### Modified Files (10)
- `movement.c` (navigation fix + anomaly detection integration)
- `movement.h` (added phase_state_t field)
- `movement_config.h` (registered zone_display_face 4×)
- `movement_faces.h` (added zone_display_face include)
- `lib/phase/phase_engine.h` (anomaly detection API)
- `lib/phase/phase_engine.c` (anomaly detection impl)
- `lib/metrics/metric_comfort.h` (lunar params)
- `lib/metrics/metric_comfort.c` (Conway lunar + integration)
- `lib/metrics/metrics.c` (updated metric_comfort call site)
- `watch-faces.mk` (added zone_display_face.c to build)

## Commit Message (Suggested)

```
Phase 4D: Dogfooding & Tuning - Anomalies + Lunar + Zone UI

Implements Phase 4D features for second-movement Phase Engine:

1. **Navigation Fix**: MODE long-press exits playlist mode
2. **Zone Display Face**: Unified metric display (4× registered)
3. **Anomaly Detection**: Soft chimes for ±20 deviations, active-hours gated
4. **Lunar Integration**: Conway approximation (±1 day) @ 20% Comfort weight

Build: 138KB flash, 8KB RAM, ~320 lines
Refs: commit efcf826 (Lunar → Comfort decision)
```

## Decision Log

1. **Zone Face Architecture:** Chose single zone_display_face (4× registered) over 4 separate faces
   - Rationale: DRY principle, easier maintenance, identical behavior across zones
   - Trade-off: Slightly more complex context queries vs duplicated code

2. **Trend Display:** Deferred to future enhancement
   - Rationale: metrics_snapshot_t lacks trend fields, would require significant refactor
   - Workaround: Display metric value only, reserve line 2 for future trend data

3. **Anomaly Chime:** Used watch_buzzer_play_note() directly vs movement_play_note()
   - Rationale: movement_play_note() creates sequence (3-element array), overkill for single note
   - Trade-off: Direct call is simpler but bypasses movement's buzzer volume API

4. **Lunar Weight:** 20% confirmed via commit efcf826
   - Rationale: Per strategic plan, matches dlorp's decision
   - Implementation: Redistributed temp/light weights (48/32) to preserve 100% total
