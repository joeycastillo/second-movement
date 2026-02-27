# First Boot Defaults - Implementation Plan

## 1. BKUP Register Map (from source)

| Register | Type | Contents |
|----------|------|----------|
| BKUP[0] | `movement_settings_t` | version(2), button_should_sound(1), to_interval(2), le_interval(3), led_duration(3), led_red(4), led_green(4), led_blue(4), time_zone(6), clock_mode_24h(1), use_imperial_units(1), button_volume(1) |
| BKUP[1] | `movement_location_t` | latitude(16 signed, hundredths of degree), longitude(16 signed, hundredths of degree) |
| BKUP[2] | `movement_active_hours_t` | start_quarter_hours(7, 0-95), end_quarter_hours(7, 0-95), enabled(1), reserved(17) |
| BKUP[3] | `movement_reserved_t` | reserved(32) - placeholder for future use |
| BKUP[4-6] | -- | Available for third-party watch faces (claimed via `movement_claim_backup_register()`, starting from index 2 per `next_available_backup_register = 2`) |
| BKUP[7] | -- | Used by watch_rtc for reference time |

**Note:** `next_available_backup_register` starts at 2 in `app_init()`, meaning BKUP[0] and BKUP[1] are reserved by movement. BKUP[2] is used for active hours but is NOT reserved through the claim mechanism -- it is accessed directly. Watch faces that call `movement_claim_backup_register()` get BKUP[2] onwards, which creates a collision with active hours. This is a pre-existing bug, not in scope for this plan.

## 2. What is Already in movement_config.h as Compile-Time Defaults

These are handled by `generate_config.py` and written into `movement_config.h`. They configure BKUP[0] fields on first boot (in `app_init()`):

- `MOVEMENT_DEFAULT_24H_MODE` -> `clock_mode_24h`
- `MOVEMENT_DEFAULT_RED_COLOR` / `GREEN` / `BLUE` -> LED color
- `MOVEMENT_DEFAULT_BUTTON_SOUND` -> `button_should_sound`
- `MOVEMENT_DEFAULT_BUTTON_VOLUME` -> `button_volume`
- `MOVEMENT_DEFAULT_SIGNAL_VOLUME` -> `signal_volume` (stored in movement_state, not BKUP)
- `MOVEMENT_DEFAULT_ALARM_VOLUME` -> `alarm_volume` (stored in movement_state, not BKUP)
- `MOVEMENT_DEFAULT_TIMEOUT_INTERVAL` -> `to_interval`
- `MOVEMENT_DEFAULT_LOW_ENERGY_INTERVAL` -> `le_interval`
- `MOVEMENT_DEFAULT_LED_DURATION` -> `led_duration`

**These must NOT be duplicated in `movement_defaults.h`.** The new file only covers runtime-only settings (BKUP[1], BKUP[2]).

## 3. Current First-Boot Initialization Logic

In `app_init()`:
1. Reads `settings.u32` from filesystem
2. If file exists AND `version == 0`, restores settings from file
3. Otherwise, populates `movement_state.settings` from `MOVEMENT_DEFAULT_*` defines and writes to flash

For BKUP[2] (active hours): `movement_get_active_hours()` checks if `reg == 0 || reg == 0xFFFFFFFF` and writes hardcoded defaults (start=16/04:00, end=92/23:00, enabled=true). There is **no sentinel/initialized flag** -- it relies on all-zeros or all-ones detection.

For BKUP[1] (location): No initialization code exists. It defaults to 0,0 (null island).

## 4. New Defines for movement_defaults.h

```c
#ifndef MOVEMENT_DEFAULTS_H_
#define MOVEMENT_DEFAULTS_H_

// --- BKUP[1]: Location defaults ---
// Latitude/longitude in hundredths of a degree (int16_t range: -18000 to 18000)
// Default: 0,0 (unchanged from current behavior)
#ifndef MOVEMENT_DEFAULT_LATITUDE
#define MOVEMENT_DEFAULT_LATITUDE 0
#endif

#ifndef MOVEMENT_DEFAULT_LONGITUDE
#define MOVEMENT_DEFAULT_LONGITUDE 0
#endif

// --- BKUP[2]: Active hours defaults ---
// Quarter-hour values 0-95 (0=00:00, 4=01:00, 32=08:00, 92=23:00)
#ifndef MOVEMENT_DEFAULT_ACTIVE_HOURS_START
#define MOVEMENT_DEFAULT_ACTIVE_HOURS_START 16  // 04:00
#endif

#ifndef MOVEMENT_DEFAULT_ACTIVE_HOURS_END
#define MOVEMENT_DEFAULT_ACTIVE_HOURS_END 92    // 23:00
#endif

#ifndef MOVEMENT_DEFAULT_ACTIVE_HOURS_ENABLED
#define MOVEMENT_DEFAULT_ACTIVE_HOURS_ENABLED true
#endif

#endif // MOVEMENT_DEFAULTS_H_
```

## 5. Minimal Changes to movement.c

**Change `movement_get_active_hours()`** to use the new defines instead of hardcoded values:

```c
// In the initialization branch (reg == 0 || reg == 0xFFFFFFFF):
settings.bit.start_quarter_hours = MOVEMENT_DEFAULT_ACTIVE_HOURS_START;
settings.bit.end_quarter_hours = MOVEMENT_DEFAULT_ACTIVE_HOURS_END;
settings.bit.enabled = MOVEMENT_DEFAULT_ACTIVE_HOURS_ENABLED;
```

**Add location initialization in `app_init()`** (or a new helper):

```c
// After settings initialization, check BKUP[1] for location defaults
movement_location_t location;
location.reg = watch_get_backup_data(1);
if (location.reg == 0 && (MOVEMENT_DEFAULT_LATITUDE != 0 || MOVEMENT_DEFAULT_LONGITUDE != 0)) {
    location.bit.latitude = MOVEMENT_DEFAULT_LATITUDE;
    location.bit.longitude = MOVEMENT_DEFAULT_LONGITUDE;
    watch_store_backup_data(location.reg, 1);
}
```

**Add `#include "movement_defaults.h"` to movement.c.**

Also remove the duplicate `get_active_hours()` static function (the `active_hours_config_t` one near line 130) which duplicates `movement_get_active_hours()`.

## 6. New Settings for the Builder UI

### Phase 1 (minimum viable)

| Setting | Type | UI Control | Range |
|---------|------|-----------|-------|
| Active Hours: Start | time-of-day | Dropdown/time picker | 00:00-23:45 (15-min steps) |
| Active Hours: End | time-of-day | Dropdown/time picker | 00:00-23:45 (15-min steps) |
| Active Hours: Enabled | bool | Checkbox | true/false |

### Phase 2 (stretch)

| Setting | Type | UI Control | Range |
|---------|------|-----------|-------|
| Default Latitude | number | Text input | -90.00 to 90.00 (hundredths of degree) |
| Default Longitude | number | Text input | -180.00 to 180.00 (hundredths of degree) |

Location is lower priority because (a) it requires understanding the hundredths-of-degree encoding, (b) a map picker would be ideal but is complex, and (c) fewer users need it pre-configured.

## 7. Time-of-Day Picker Design

Quarter-hour values 0-95 map to times 00:00 through 23:45.

**Recommended UI approach:** A `<select>` dropdown with 96 options:
- Display: "00:00", "00:15", "00:30", "00:45", "01:00", ..., "23:45"
- Value: 0-95 (the raw quarter-hour integer)
- Conversion: `hour = value / 4`, `minute = (value % 4) * 15`

This is simpler than a dual hour/minute picker and matches the hardware representation exactly. The dropdown is long (96 entries) but browsers handle this fine with type-ahead search.

**Alternative:** Two dropdowns (hour 0-23, quarter 0-3) that combine to the quarter-hour value. Slightly more compact but adds complexity.

## 8. File Changes

### Firmware (C)

| File | Change |
|------|--------|
| `movement_defaults.h` (NEW) | New file with `#define` defaults for BKUP[1] and BKUP[2] runtime settings |
| `movement.c` | (1) `#include "movement_defaults.h"` (2) Replace hardcoded values in `movement_get_active_hours()` with defines (3) Add location default initialization in `app_init()` (4) Remove duplicate `get_active_hours()` / `active_hours_config_t` static function |
| `Makefile` or build system | Ensure `movement_defaults.h` is on the include path (likely already is if placed alongside `movement_config.h`) |

### Builder / CI

| File | Change |
|------|--------|
| `builder/settings_registry.json` | Add entries for active_hours_start, active_hours_end, active_hours_enabled (and optionally lat/lon) |
| `builder/index.html` | Add time-of-day dropdown controls for active hours in the Settings section |
| `.github/scripts/generate_config.py` | (1) Accept new CLI args: `--active-hours-start`, `--active-hours-end`, `--active-hours-enabled` (2) Generate `movement_defaults.h` as a second output file |
| `.github/workflows/custom-build.yml` | Pass new settings to `generate_config.py` and copy `movement_defaults.h` to the build directory |

## 9. Parallel Implementation Tracks

These three tracks have no dependencies on each other until final integration:

### Track A: Firmware defaults (movement_defaults.h + movement.c changes)
- Create `movement_defaults.h` with `#ifndef` guarded defines
- Modify `movement_get_active_hours()` to use defines
- Add location initialization to `app_init()`
- Remove duplicate `get_active_hours()` static function
- Test: build with default values, verify behavior unchanged

### Track B: Builder UI (index.html + settings_registry.json)
- Add active hours controls to settings_registry.json
- Add time-of-day dropdowns to index.html
- Wire up form data to be passed as build parameters
- Test: verify UI generates correct parameter values

### Track C: Config generation (generate_config.py + workflow)
- Add CLI args to generate_config.py
- Add `movement_defaults.h` template and generation
- Update custom-build.yml to pass new args and copy the file
- Test: verify generated files are correct

### Integration
- Wire Track B output through Track C to produce Track A inputs
- End-to-end test: builder UI -> GitHub Action -> firmware with custom active hours

## 10. Risk Assessment

### High Risk

**BKUP[2] collision with `movement_claim_backup_register()`:**
`next_available_backup_register` starts at 2, which is the same register used for active hours. Any watch face that calls `movement_claim_backup_register()` will get BKUP[2] and overwrite active hours. This is a **pre-existing bug** but the first-boot defaults feature makes it more visible. Mitigation: change `next_available_backup_register` initial value to 4 (skipping BKUP[0-3] which are all reserved by movement types). This is a one-line fix in `app_init()` and should be included in this PR.

### Medium Risk

**First-boot detection is fragile:**
The current check (`reg == 0 || reg == 0xFFFFFFFF`) works because BKUP registers are zeroed on full power loss. But if a user legitimately sets start=0, end=0, enabled=false (quarter-hour values that happen to make reg=0), it would re-trigger initialization. Mitigation: the combination start=0 AND end=0 is already caught by the `start == end` validation (in the static `get_active_hours()`), so this is unlikely in practice. For extra safety, the "enabled" bit being at bit 14 means a reg of 0 always means "disabled with 00:00-00:00", which is nonsensical and safe to treat as uninitialized.

**Location default of 0,0 is a valid location (Gulf of Guinea):**
Cannot use `reg == 0` as "uninitialized" sentinel for BKUP[1] if someone actually lives at 0.00, 0.00. Mitigation: only write location defaults if the define values are non-zero AND the register is zero. Users at 0,0 latitude/longitude (effectively nobody) would need to set it manually.

### Low Risk

**Builder UI complexity:**
96-entry dropdowns are manageable. No risk of data corruption since values are clamped 0-95 in firmware.

**Backward compatibility:**
Existing users who flash new firmware with `movement_defaults.h` will have their active hours remain unchanged (registers are already initialized from previous boot). The defaults only apply when registers read as 0 or 0xFFFFFFFF.

**Generate script ordering:**
`movement_defaults.h` must be written before `make` runs. The workflow already writes `movement_config.h` before building, so adding a second file write is straightforward.
