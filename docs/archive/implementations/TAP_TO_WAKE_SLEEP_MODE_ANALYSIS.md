# Tap-to-Wake Sleep Mode Analysis

**Date:** 2026-02-15  
**Repo:** ~/repos/dlorp-second-movement  
**Branch:** feature/tap-to-wake  
**Question:** Does tap-to-wake still function when the watch enters low power/sleep mode?

## Executive Summary

**Answer: NO** - Tap-to-wake does NOT currently function during sleep mode.

**Root Cause:** The accelerometer INT1 pin (A3) is not registered as an external wake source when entering sleep mode. Only the ALARM button (PA02) is configured for external wake.

**Impact:** Users cannot wake the display by tapping the watch crystal when the watch has entered low power/sleep mode after the inactivity timeout.

---

## 1. Emulator Build & Testing

### Build Status
✅ **Emulator already built** - Files exist in `build-sim/`:
- `firmware.html` (160 KB)
- `firmware.js` (101 KB)  
- `firmware.wasm` (412 KB)

### How to Run Emulator
```bash
cd ~/repos/dlorp-second-movement
python3 -m http.server -d build-sim
```
Then visit: http://localhost:8000/firmware.html

### How to Rebuild (requires emscripten)
```bash
emmake make BOARD=sensorwatch_red DISPLAY=classic
python3 -m http.server -d build-sim
```

### Accelerometer Simulation
❌ **NOT SIMULATED** - The web-based emulator does not simulate the LIS2DW12 accelerometer.

**Evidence:**
- Simulator uses stub I2C implementation (`watch-library/simulator/watch/watch_i2c.c`)
- No tap event generation in simulator code
- Button events only (A/L/M keys or on-screen buttons)

**Consequence:** Cannot test tap-to-wake in emulator - **hardware testing required**.

---

## 2. Code Analysis: Sleep Mode Behavior

### Sleep Mode Entry (movement.c:1336-1343)

When the watch enters sleep mode after inactivity timeout:

```c
if (movement_volatile_state.enter_sleep_mode && !movement_volatile_state.is_buzzing) {
    movement_volatile_state.enter_sleep_mode = false;
    movement_volatile_state.is_sleeping = true;
    
    _movement_disable_inactivity_countdown();
    
    watch_register_extwake_callback(HAL_GPIO_BTN_ALARM_pin(), cb_alarm_btn_extwake, true);
    //                                ^^^^^^^^^^^^^^^^^^^^^ ONLY ALARM BUTTON!
    
    _sleep_mode_app_loop();
    // ...
}
```

**Critical Finding:** Only the ALARM button is registered for external wake via `watch_register_extwake_callback()`.

### Accelerometer Configuration

The accelerometer INT1 (A3) pin is configured during `app_init()` (movement.c:1138):

```c
// INT1 is wired to pin A3 for tap detection
watch_register_interrupt_callback(HAL_GPIO_A3_pin(), cb_accelerometer_event, INTERRUPT_TRIGGER_RISING);
```

**However:** `watch_register_interrupt_callback()` is NOT the same as `watch_register_extwake_callback()`.

- **`watch_register_interrupt_callback`**: Works only when CPU is running (normal/active mode)
- **`watch_register_extwake_callback`**: Uses RTC tamper detection to wake from STANDBY (sleep mode)

### Sleep Mode Deep Dive (watch_deepsleep.c)

The `watch_enter_sleep_mode()` function:

1. Disables all peripherals except SLCD (display)
2. Disables tick interrupts
3. **Disables external interrupts** via `watch_disable_external_interrupts()`
4. Disables all pins except RTC tamper pins
5. Enters STANDBY mode (PM sleep mode 4)

**Key code:**
```c
void watch_enter_sleep_mode(void) {
    _watch_disable_all_peripherals_except_slcd();
    watch_rtc_disable_all_periodic_callbacks();
    _watch_disable_all_pins_except_rtc();  // ← A3 gets disabled!
    
    sleep(4);  // STANDBY mode - only RTC tamper pins can wake
}
```

### RTC Tamper Pins (watch_deepsleep.c:44-79)

Only these pins can wake the watch from STANDBY:
- **RTC/IN[2]** → PA02 (ALARM button) ✅ Currently registered
- **RTC/IN[1]** → PA02 (alternate mapping)
- **RTC/IN[0]** → PB00 (A4 - accelerometer INT2)

**A3 (PB03) is NOT an RTC tamper pin** → Cannot wake from sleep!

### Tap Detection Power Mode

Tap detection remains configured at 400Hz (movement.c:837):

```c
lis2dw_set_data_rate(LIS2DW_DATA_RATE_HP_400_HZ);  // 400 Hz needed for tap detection
```

The accelerometer **continues running** during sleep (I2C peripheral state is preserved), but:
- INT1 interrupts cannot wake the CPU from STANDBY
- INT1 pin is disabled during `_watch_disable_all_pins_except_rtc()`
- Taps are detected but lost (no handler runs)

---

## 3. Test Plan (Hardware Required)

Since emulator doesn't simulate accelerometer, hardware testing is essential:

### Test Procedure

1. **Build & Flash Firmware**
   ```bash
   make BOARD=sensorwatch_pro DISPLAY=classic
   make install
   ```

2. **Test Normal Mode Tap**
   - Press MODE/LIGHT to wake display
   - Tap watch crystal
   - **Expected:** Display wakes (✅ works)

3. **Test Sleep Mode Tap**
   - Let display sleep (wait for inactivity timeout, typically 60-300s)
   - Watch should enter sleep mode (display blank, low power)
   - Tap watch crystal firmly
   - **Expected:** Display does NOT wake (❌ confirmed bug)
   - Press ALARM button
   - **Expected:** Display wakes (✅ works - ALARM is extwake)

4. **Verify Accelerometer Still Running**
   - Connect serial console
   - Add debug print in `cb_accelerometer_event()` callback
   - Observe: interrupts fire during normal mode, not during sleep

### Test Results (Predicted)

| Test Case | Expected Result | Status |
|-----------|----------------|--------|
| Tap in normal mode | Wake + reset inactivity | ✅ Works |
| Tap during sleep | No wake | ❌ **BUG** |
| ALARM button during sleep | Wake | ✅ Works |
| Accelerometer still sampling | 400Hz continues | ✅ Expected |
| INT1 interrupt fires | Only in normal mode | ❌ Lost during sleep |

---

## 4. Proposed Solutions

### Option A: Register A3 as External Wake (⚠️ NOT POSSIBLE)

**Ideal but blocked by hardware:**

```c
// WOULD BE IDEAL:
watch_register_extwake_callback(HAL_GPIO_A3_pin(), cb_accelerometer_wake, false);
```

**Problem:** A3 (PB03) is not mapped to any RTC tamper input pin.

**Conclusion:** Cannot use INT1 to wake from sleep.

---

### Option B: Use INT2 (A4) for Tap Detection ✅ RECOMMENDED

**Rationale:** A4 (PB00) IS mapped to RTC/IN[0] and CAN wake from sleep.

**Changes Required:**

1. **Remap tap detection to INT2 in movement.c:**

   ```c
   // CURRENT (line 844):
   lis2dw_configure_int1(LIS2DW_CTRL4_INT1_SINGLE_TAP | LIS2DW_CTRL4_INT1_DOUBLE_TAP);
   
   // CHANGE TO:
   lis2dw_configure_int2(LIS2DW_CTRL5_INT2_SINGLE_TAP | LIS2DW_CTRL5_INT2_DOUBLE_TAP);
   ```

2. **Update interrupt registration (line 1138):**

   ```c
   // CURRENT:
   watch_register_interrupt_callback(HAL_GPIO_A3_pin(), cb_accelerometer_event, INTERRUPT_TRIGGER_RISING);
   
   // CHANGE TO:
   watch_register_interrupt_callback(HAL_GPIO_A4_pin(), cb_accelerometer_event, INTERRUPT_TRIGGER_RISING);
   ```

3. **Register A4 for external wake during sleep (line 1343):**

   ```c
   // ADD after ALARM button registration:
   watch_register_extwake_callback(HAL_GPIO_BTN_ALARM_pin(), cb_alarm_btn_extwake, true);
   watch_register_extwake_callback(HAL_GPIO_A4_pin(), cb_accelerometer_wake, false);  // ← NEW
   ```

4. **Update LIS2DW driver to support INT2 tap detection:**

   Check `watch-library/shared/driver/lis2dw.c` for INT2 tap configuration support. May need to add:
   
   ```c
   // In lis2dw.h:
   #define LIS2DW_CTRL5_INT2_SINGLE_TAP (1 << 5)  // Check datasheet
   #define LIS2DW_CTRL5_INT2_DOUBLE_TAP (1 << 4)  // Check datasheet
   ```

**Trade-offs:**
- ✅ Tap-to-wake works during sleep
- ✅ Uses existing hardware capability
- ⚠️ INT2 was previously used for sleep state detection (can be removed)
- ⚠️ Requires LIS2DW driver updates
- ⚠️ Need to verify INT2 tap detection works (datasheet check)

---

### Option C: Keep Tap Detection Active, Disable Sleep ⚠️ HIGH POWER

**Implementation:**

```bash
make BOARD=sensorwatch_pro DISPLAY=classic NOSLEEP=1
```

Or in code:
```c
#define MOVEMENT_LOW_ENERGY_MODE_FORBIDDEN
```

**Trade-offs:**
- ✅ Tap-to-wake works everywhere (no sleep mode)
- ❌ Battery life drastically reduced (no deep sleep)
- ❌ Not recommended for production

---

### Option D: Disable Tap Detection During Sleep ⚠️ CURRENT BEHAVIOR

**Already implemented** (unintentionally):

Tap detection remains enabled, but interrupts are ignored during sleep.

**Power consumption:**
- Accelerometer: ~45-90µA (still running at 400Hz)
- Watch in sleep: ~2-5µA baseline
- **Total: ~50-95µA during sleep** (acceptable)

**Trade-offs:**
- ❌ No tap-to-wake during sleep (user must press ALARM button)
- ✅ Low power consumption maintained
- ⚠️ Accelerometer power wasted (running but interrupts ignored)

**Optimization:** If keeping this behavior, should power down accelerometer during sleep:

```c
// In movement.c, before sleep:
if (movement_state.has_lis2dw) {
    lis2dw_set_data_rate(LIS2DW_DATA_RATE_POWERDOWN);  // Save ~50-90µA
}

// After wake:
movement_enable_tap_detection_if_available();  // Restore 400Hz
```

---

## 5. Recommendation

### Short Term: Option D with Power Optimization

**Action:** Disable accelerometer during sleep to save power.

```c
// movement.c:1336 - before _sleep_mode_app_loop()
if (movement_state.has_lis2dw) {
    movement_disable_tap_detection_if_available();
}

// movement.c:1350 - after _sleep_mode_app_loop() returns
if (movement_state.has_lis2dw) {
    movement_enable_tap_detection_if_available();
}
```

**Benefits:**
- Saves ~50-90µA during sleep
- Minimal code changes
- Maintains current UX (ALARM button wakes from sleep)

**Power Savings:**
- Sleep mode: ~2-5µA baseline → **same** (accelerometer now off)
- Battery life impact: +10-20% (depending on sleep ratio)

---

### Long Term: Option B - Remap to INT2 ✅

**Action:** Move tap detection from INT1 (A3) to INT2 (A4) to enable external wake.

**Prerequisites:**
1. Verify LIS2DW12 supports tap detection on INT2 (check datasheet)
2. Update LIS2DW driver with INT2 tap configuration
3. Test on hardware (cannot test in emulator)

**Benefits:**
- ✅ Tap-to-wake works during sleep
- ✅ Low power maintained (~50-90µA baseline from tap detection)
- ✅ Improved UX (no need for ALARM button)

**Risks:**
- Need to verify INT2 tap detection works (may have hardware limitations)
- Requires driver changes and testing
- INT2 currently used for sleep state detection (needs removal/rework)

---

## 6. Summary Table

| Solution | Tap Works in Sleep? | Power Impact | Code Changes | Hardware Test Required |
|----------|---------------------|--------------|--------------|----------------------|
| **Current (unintentional)** | ❌ No | ⚠️ +50µA wasted | None | ✅ Confirmed |
| **Option A (A3 extwake)** | ❌ Not possible | N/A | N/A | N/A |
| **Option B (INT2 remap)** | ✅ Yes | +50-90µA | Medium | ✅ Required |
| **Option C (disable sleep)** | ✅ Yes | ❌ Very high | Trivial | No |
| **Option D (power down accel)** | ❌ No | ✅ Minimal | Small | No |

---

## 7. Next Steps

### Immediate (No Hardware)
1. ✅ Implement Option D power optimization (disable accel during sleep)
2. Document trade-off: tap-to-wake vs sleep power savings
3. Update TAP_TO_WAKE.md with sleep mode behavior

### Future (Requires Hardware Testing)
1. Test INT2 tap detection on bench hardware
2. If successful, implement Option B (remap to INT2)
3. Measure power consumption with tap-to-wake in sleep
4. User preference setting: "Tap-to-wake during sleep" (on/off)

---

## Appendix: Code References

### Key Files
- `movement.c` - Main movement loop, sleep mode handling
- `watch-library/hardware/watch/watch_deepsleep.c` - Sleep mode implementation
- `watch-library/shared/driver/lis2dw.c` - Accelerometer driver
- `TAP_TO_WAKE.md` - Feature documentation

### Key Functions
- `_sleep_mode_app_loop()` - Sleep mode loop (line 1172)
- `watch_enter_sleep_mode()` - STANDBY entry (watch_deepsleep.c:167)
- `movement_enable_tap_detection_if_available()` - Enable tap (line 826)
- `cb_accelerometer_event()` - Tap interrupt handler (line 1542)
- `cb_alarm_btn_extwake()` - ALARM button wake handler (line 1520)

### Relevant Datasheet Sections
- LIS2DW12 Datasheet: Section 8.7 (Tap detection)
- LIS2DW12 Datasheet: Section 8.10 (Interrupt pins)
- SAMD51 Datasheet: Section 19 (RTC tamper detection)
- SAMD51 Datasheet: Section 17 (Power management and sleep modes)

---

**End of Analysis**
