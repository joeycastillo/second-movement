# Stream 1: Active Hours Core Sleep Mode Logic

**Branch:** feature/active-hours-core  
**Date:** 2026-02-15  
**Status:** Implementation Complete

## Overview

Stream 1 implements the foundation for Active Hours sleep mode detection and power optimization. This prevents wrist motion from waking the display during sleep hours while maintaining tap-to-wake and button wake functionality.

## Implementation Summary

### New Functions

#### 1. `is_sleep_window()`
- **Location:** movement.c (static function)
- **Purpose:** Check if current time is outside Active Hours
- **Hardcoded Active Hours:** 04:00-23:00
- **Sleep Window:** 23:00-04:00
- **Returns:** `true` if outside active hours (in sleep window)
- **Note:** Will be made configurable in Stream 2 via BKUP[2] register

#### 2. `is_confirmed_asleep()`
- **Location:** movement.c (static function)
- **Purpose:** Confirm wearer is asleep using both time and motion data
- **Logic:** Returns `true` only if BOTH:
  1. Current time is in sleep window (via `is_sleep_window()`)
  2. Accelerometer reports stationary/sleep state (via LIS2DW_WAKEUP_SRC_SLEEP_STATE)
- **Fallback:** On Green board (no accelerometer), falls back to time-only check
- **Benefit:** Prevents false positives from just lying still during the day

### Modified Functions

#### 3. `cb_accelerometer_wake()`
- **Location:** movement.c
- **Modification:** Added sleep mode check before processing motion wake
- **Behavior:**
  - **During confirmed sleep:** Motion events are suppressed (discarded)
  - **Tap events:** Always processed, even during sleep (tap-to-wake still works)
  - **Outside sleep hours:** Normal behavior (wake on any motion)
- **Key integration:** Calls `is_confirmed_asleep()` to determine if motion wake should be suppressed

## Key Design Decisions

### 1. Dual-Condition Sleep Detection
- **Why both time AND accelerometer?**
  - Prevents false sleep detection during daytime rest
  - Ensures we only suppress wake when truly asleep
  - More robust than time-only approach

### 2. Tap-to-Wake Preserved
- **Implementation:** Tap events are processed BEFORE sleep check
- **Result:** Tap-to-wake works 24/7, even during sleep window
- **Benefit:** User can still wake watch with crystal tap if needed

### 3. Graceful Green Board Fallback
- **Challenge:** Green board has no accelerometer
- **Solution:** `is_confirmed_asleep()` checks `movement_state.has_lis2dw`
- **Fallback:** Falls back to time-only check if no accelerometer
- **Result:** Feature works on all boards with appropriate degradation

### 4. No New Power Cost
- **Accelerometer already at 1.6Hz in stationary mode** (existing behavior)
- **INT2 already configured for sleep state reporting** (existing setup)
- **No peripheral changes needed** - just logic to suppress events
- **Tap detection remains at 400Hz** (45-90µA baseline, unchanged)

## Technical Details

### Accelerometer Integration
- **INT1 (A3):** Tap detection (400Hz, always active)
- **INT2 (A4):** Sleep state reporting (already configured in app_init)
- **Wakeup Source Register:** Read via `lis2dw_get_wakeup_source()`
- **Sleep State Flag:** `LIS2DW_WAKEUP_SRC_SLEEP_STATE` (0b00010000)

### Active Hours Configuration
Current hardcoded values (will be configurable in Stream 2):
```c
// Sleep window: 23:00 through 03:59
// Active hours: 04:00 through 22:59
return (hour >= 23 || hour < 4);
```

### Event Flow During Sleep
1. **Motion detected** → INT2/A4 fires → `cb_accelerometer_wake()` called
2. **Tap detected** → INT1/A3 fires → tap flag set, sleep check passed
3. **Sleep check** → `is_confirmed_asleep()` → checks time + accelerometer
4. **If confirmed asleep** → motion event discarded, tap event processed
5. **If not asleep** → normal wake behavior

## Files Modified

- **movement.c** (+61 lines)
  - Added `is_sleep_window()` static function
  - Added `is_confirmed_asleep()` static function
  - Modified `cb_accelerometer_wake()` with sleep mode logic

## Testing Status

### Compilation
- **Pro Board (sensorwatch_pro):** Unable to verify (ARM toolchain not available)
- **Green Board (sensorwatch_green):** Unable to verify (ARM toolchain not available)
- **Syntax:** Verified via code review
- **Logic:** Reviewed against existing codebase patterns

### Hardware Testing Required
Since emulator doesn't simulate LIS2DW12 accelerometer, hardware testing is essential:

1. **Test sleep window detection:**
   - Set time to 02:00 (inside sleep window)
   - Move wrist - display should NOT wake
   - Tap crystal - display SHOULD wake
   - Set time to 10:00 (outside sleep window)
   - Move wrist - display SHOULD wake

2. **Test accelerometer integration:**
   - Lie still in bed during sleep window - should suppress wake
   - Be active during sleep window - should allow wake (not stationary)
   - Verify LIS2DW_WAKEUP_SRC_SLEEP_STATE flag behavior

3. **Test Green board fallback:**
   - Build for Green board
   - Verify time-only sleep detection works
   - Confirm no accelerometer-related crashes

## Integration with Other Streams

### Stream 2: User Configuration UI
- Will replace hardcoded 04:00-23:00 with BKUP[2] stored values
- No changes to Stream 1 logic needed - just update `is_sleep_window()`
- Active hours will be configurable: start hour, end hour

### Stream 3: Enhanced Sleep State Tracking
- May enhance `is_confirmed_asleep()` with additional heuristics
- Could add hysteresis to prevent rapid sleep/wake transitions
- Stream 1 provides clean foundation for enhancements

### Stream 4: Orientation-Based Wake Priority
- May interact with `cb_accelerometer_wake()` for wrist-down suppression
- Stream 1's sleep detection is complementary, not conflicting
- Both features can coexist with minimal changes

## Known Limitations

1. **Hardcoded Active Hours:** 04:00-23:00 (will be fixed in Stream 2)
2. **No transition hysteresis:** Immediate sleep/wake state changes (may add in Stream 3)
3. **Binary sleep detection:** No "drowsy" or "resting" intermediate states
4. **Time-only fallback on Green:** Less accurate without accelerometer data

## Next Steps

### Immediate (This PR)
1. ✅ Implement core functions
2. ⏳ Hardware testing on Pro board
3. ⏳ Hardware testing on Green board
4. ⏳ Merge to main once tested

### Future Streams
1. **Stream 2:** User configuration UI (BKUP[2] settings register)
2. **Stream 3:** Enhanced sleep state tracking and logging
3. **Stream 4:** Orientation-based wake priority
4. **Stream 5:** Sleep quality metrics and reporting

## Code Quality

- ✅ No emojis in code or comments
- ✅ Professional, clear comments
- ✅ Follows existing code style and patterns
- ✅ Graceful degradation on boards without accelerometer
- ✅ No new power cost - pure logic optimization
- ✅ Minimal invasive changes - only 61 lines added

## Performance Impact

- **Power consumption:** No change (reuses existing accelerometer configuration)
- **CPU overhead:** Negligible (two boolean checks + one I2C read on wake events)
- **Memory footprint:** Zero (static functions, no new state)
- **Wake latency:** No change (sleep check takes microseconds)

## Security & Safety

- **No data leakage:** No logging of sleep state or time data
- **Failsafe:** Falls back to time-only check if accelerometer unavailable
- **User control:** Button and tap wake always work (no lockout risk)
- **Reversible:** Can be disabled by setting active hours to 00:00-23:59 (future)

---

**Implementation Status:** ✅ Complete  
**Ready for:** Hardware testing and review  
**Blocks:** None  
**Blocked by:** None
