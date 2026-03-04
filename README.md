Second Movement
===============

**Note:** This is my fork of second-movement. See upstream at [joeycastillo/second-movement](https://github.com/joeycastillo/second-movement).

## Sites

| Site | Description |
|------|-------------|
| [**Firmware Builder**](https://dlorp.github.io/second-movement/builder/) | Web UI - pick faces, configure settings, dispatch a GitHub Actions build, download UF2 |
| [**Companion App**](https://dlorp.github.io/second-movement/companion-app/) | Optical TX web app - send data to the watch via screen flash (Timex DataLink style) |

## Phase Engine: Chronomantic Instrument

#### Phase Engine Documentation
- [Data Architecture](docs/PHASE_ENGINE_DATA_ARCHITECTURE.md) - Temperature data system
- [Implementation](docs/PHASE4E_IMPLEMENTATION.md) - Phase 4E implementation
- [Tuning](docs/PHASE4_TUNING_PLAN.md) - Calibration guide

> ⚠️ **Current Status:** Phase 4E/4F complete - ready for dogfooding. All core features implemented and tested:
> - ✅ Sleep tracking with movement frequency analysis
> - ✅ Telemetry export (512 bytes via comms_face)
> - ✅ Sleep mode enforcement (active hours integration)
> - ✅ Location-adaptive daylight detection (homebase)
> - ✅ Personalized activity baselines (7-day WK tracking)
> - ✅ Zero hardcoded times (all adaptive to location + schedule)

**What it is:** The watch becomes a **personal chronomantic instrument** not just telling you the time, but measuring your alignment with natural cycles. It tracks the relationship between **Human × Environment × Season** to show you where you are relative to what's normal for your body in this place, at this time of year.

**Philosophy:** Instead of passive time telling, the watch actively measures three axes simultaneously:

By monitoring these metrics throughout the day, you can see if your actions are improving your alignment or causing you to stagnate. The homebase data tables (pre computed seasonal norms for your location) provide the reference point; the instrument shows you the delta.

**Three axes of measurement:**

| Axis | What It Measures | Data Source |
|------|------------------|-------------|
| **Human** | Activity rhythm, wake momentum, sleep debt | Accelerometer, sleep tracking |
| **Environment** | Light, temperature, motion variance | Lux sensor, thermistor, motion patterns |
| **Season** | Expected daylight, climate norms for your location | Homebase table (pre-computed seasonal data) |

**Core principle:** Phase is a continuous weighted field (0-100), not rigid clock zones. Your biological state flows through four zones as you move through your day. The watch surfaces metrics that deviate from expected norms  when everything is aligned (phase ≈ 100), you see green lights. When something is off (low phase score), the relevant metric surfaces to show you what needs attention.

---

#### Phase 4E: Advanced Sleep Tracking

Phase 4E adds sophisticated sleep analysis using movement frequency scoring to distinguish sleep depth:

**Movement Frequency States:**
- **DEEP:** Minimal movement (0-1 movements per hour) - restorative sleep
- **LIGHT:** Low movement (2-3 movements per hour) - lighter sleep stages
- **RESTLESS:** Frequent movement (4-6 movements per hour) - disturbed sleep
- **WAKE:** High movement (7+ movements per hour) - awake or semi-conscious

**Wake Event Detection:**
Sustained movement patterns trigger wake events, distinguishing brief awakenings from continuous sleep. Events are logged with timestamp and duration for sleep quality analysis.

**Restlessness Index (RI):**
- 0-100 scale measuring overall sleep quality over 7 nights
- Displayed in circadian face RI mode (long-press ALARM to cycle CS → RI)
- Lower scores indicate better sleep quality
- Formula: `RI = (restless_hours * 60 + wake_hours * 100) / total_sleep_hours`

**Per-Hour Telemetry:**
Every hour during sleep, the system logs:
- Zone transitions (which biological zone was dominant)
- Dominant metrics for that hour
- Light exposure levels
- Battery state
- Confidence scores (data quality indicators)

All telemetry data is buffered for export via the comms_face (face #14).

---

#### Phase 4F: Sleep Mode & Calibration

Phase 4F integrates sleep mode enforcement with location-adaptive calibration:

**Sleep Mode Enforcement:**
Outside active hours (user's configured wake window), all zones lock to DESCENT to prevent inappropriate metric calculations. The watch recognizes you should be sleeping and adjusts all tracking accordingly.

**EM Light Calibration (Alaska-Ready):**
Emotional/Mood (EM) metric now uses homebase-calculated daylight hours instead of hardcoded 6 AM - 6 PM. This enables accurate tracking in extreme latitudes:
- Alaska winter: Adapts to 4-5 hour daylight windows
- Alaska summer: Adapts to 19-20 hour daylight windows
- Calculated from homebase latitude + day of year using sunrise/sunset algorithms

**WK Activity Baseline (7-Day Personalization):**
Wake Momentum (WK) now learns your personal activity patterns:
- Tracks typical movement levels over rolling 7-day window
- Calculates personalized thresholds (not population averages)
- Adapts to your lifestyle: sedentary office worker vs. manual laborer
- Baseline recalibrates automatically as habits change

**Configurable Hysteresis:**
Zone transitions now require sustained state change to prevent jitter:
- Default: 3 consecutive readings (45 minutes) before zone switch
- Prevents rapid zone bouncing during marginal states
- Configurable per-zone for fine-tuning

**Zero Hardcoded Times:**
All time-based thresholds are now derived from:
- Active hours (from user settings)
- Homebase location (sunrise/sunset calculations)
- Personal baselines (learned from your data)

No more "magic numbers" assuming everyone lives at 40°N with a 10 PM bedtime.

---

#### The Four Zones

Phase score maps to zones that represent your biological state:

| Zone | Phase Score | What It Means | Example Time* |
|------|-------------|---------------|---------------|
| **Emergence** | 0-25 | Waking up, orienting, building alertness | Early morning (4-8 AM) |
| **Momentum** | 26-50 | Building energy, ramping activity | Late morning (9-12 PM) |
| **Active** | 51-75 | Peak capacity, maximum output | Afternoon (1-5 PM) |
| **Descent** | 76-100 | Winding down, preparing for rest | Evening (6-11 PM) |

***Note:** Times are examples for a typical schedule. Phase score is based on YOUR biological rhythm, not the clock. A night-shift worker's Emergence might happen at 8 PM.

**Sleep Mode Enforcement (Phase 4F):**
Outside your active hours (configured sleep window), all zones lock to DESCENT. This ensures the watch recognizes you should be sleeping and prevents inappropriate metric calculations during rest periods.

**Zone Faces:**
Each zone has a dedicated watch face (faces #2-5) that displays metrics relevant to that biological state:

- **Emergence Face** (`EM`, face #2): Shows Sleep Debt (SD), Emotional state (EM), Comfort (CMF)
- **Momentum Face** (`MO`, face #3): Shows Wake Momentum (WK), Sleep Debt, Temperature
- **Active Face** (`AC`, face #4): Shows Energy capacity (NRG), Emotional state, Sleep Debt
- **Descent Face** (`DE`, face #5): Shows Comfort, Emotional state (ready for rest)

**Zone Navigation:**
- **Access:** Long-press ALARM button from the clock face to enter zone faces
- **Rotation:** Zone faces are skipped in the normal MODE button rotation (they are tertiary navigation, not primary)
- **Manual cycling:** Long-press ALARM to cycle through zone-specific metrics within each zone
- **Automatic switching:** Watch switches zone faces when phase score crosses zone boundaries (debounced over 3 readings = 45 min for stability)
- **Smart surfacing:** Metrics with extreme values (far from neutral 50) surface more often than mundane values

---

#### The Six Metrics (0-100 Scale)

**1. Sleep Debt (SD)**
- **Meaning:** Cumulative sleep deficit over the last 3 nights
- **Range:** 0 = fully rested, 100 = severely sleep deprived
- **Math:** Target = 480 min (8h/night). Deficit per night = `max(0, 480 - actual_minutes)`. Rolling 3 night weighted average: `(deficit[0]*50 + deficit[1]*30 + deficit[2]*20) / 100`
- **Updates:** Every minute during Emergence zone, hourly otherwise
- **Data source:** Sleep tracker (7-night buffer in flash)

**2. Emotional / Mood (EM)**
- **Meaning:** Circadian alignment + lunar influence + activity variance
- **Range:** 0 = low/disrupted, 100 = aligned/elevated
- **Math:** Three sub scores blended:
  - **Circadian (40%):** Cosine curve peaking at solar noon, trough at solar midnight (adaptive to daylight hours)
  - **Lunar (20%):** Approximates lunar phase from day of year, peaks near full moon
  - **Variance (40%):** Activity variance over last 15 min (high variance during expected-rest = penalty)
- **Updates:** Every 15 minutes
- **Data source:** Hour-of-day, day-of-year, accelerometer variance
- **Phase 4F:** Now uses homebase-calculated daylight hours instead of hardcoded 6 AM - 6 PM. Alaska winter (4-5h daylight) and summer (19-20h daylight) handled correctly.

**3. Wake Momentum (WK)**
- **Meaning:** How ramped up you are since waking
- **Range:** 0 = just woke, 100 = fully alert
- **Math:** Base ramp: `min(100, minutes_awake * 100 / 120)` (full ramp in 2 hours). Activity bonus accelerates ramp up to +30%. Sedentary penalty caps at 60.
- **Updates:** Every minute for first 2 hours after wake, then every 15 min
- **Data source:** Wake onset time (from Active Hours transition) + cumulative activity
- **Phase 4F:** Now uses 7-day rolling baseline to personalize activity thresholds. Your "normal" movement level is learned from your patterns, not population averages. Adapts to lifestyle changes automatically.

**4. Energy Capacity (NRG)**
- **Meaning:** Available physical/cognitive capacity right now
- **Range:** 0 = depleted, 100 = peak capacity
- **Math:** `Energy = phase_score - (SD / 3) + activity_bonus`. Activity bonus = `min(20, recent_activity / 50)`. Essentially: how aligned you are (phase) minus how tired (sleep debt) plus recent momentum.
- **Updates:** Every 15 minutes
- **Data source:** Phase score, Sleep Debt, accelerometer activity

**5. Comfort (CMF)**
- **Meaning:** Alignment with seasonal/environmental norms for your location
- **Range:** 0 = extreme deviation, 100 = perfectly normal
- **Math:** Two sub scores:
  - **Temperature (60%):** `100 - min(100, abs(current_temp - expected_temp) / 3)`. Expected temp from homebase table.
  - **Light (40%):** Compares lux against expected for time of day (500+ lux during day, <50 at night)
- **Updates:** Every 15 minutes
- **Data source:** Thermistor, lux sensor (Pro board only), homebase seasonal table
- **Phase 4F:** Light expectations now use homebase sunrise/sunset calculations instead of fixed day hours. Adapts to extreme latitudes and seasonal variations.

**6. Jet Lag (JL)** *(Deferred to future release)*
- **Meaning:** Timezone disruption and recovery
- **Range:** 0 = no shift, 100 = maximum disruption
- **Math:** On timezone change: `JL = abs(hours_shifted) * 12` (capped at 100). Decay: `JL -= max(1, JL / 24)` per hour (roughly 1 day per hour of shift to recover)

---

#### Telemetry Export (Phase 4E)

The watch collects comprehensive tracking data that can be exported for analysis via the **comms_face** (face #14).

**Export Format:**
- **Total size:** 512 bytes per export
  - Phase engine data: 146 bytes (zone history, transitions, phase scores)
  - Circadian metrics: 112 bytes (CS subscores, 7-day trends)
  - Sleep telemetry: 254 bytes (hourly snapshots, sleep states, wake events)

**Data Collected (Per-Hour Snapshots):**
- Zone state and transitions (which zone was active)
- Dominant metrics for each hour (SD, EM, WK, NRG, CMF values)
- Light exposure levels (lux readings)
- Battery state (voltage and charge percentage)
- Confidence scores (data quality indicators)
- Sleep states (DEEP/LIGHT/RESTLESS/WAKE classification)
- Wake events (timestamp, duration, movement intensity)
- Restlessness Index (RI) for sleep quality tracking

**Transmission:**
- Via piezoelectric transducer (Timex DataLink-style optical transmission)
- Transmission time: ~2 minutes for full 512-byte export
- Encoding: Binary data encoded as light pulses
- Receiver: Companion app captures via screen flash

**Use Cases:**
- Long-term pattern analysis (export weekly)
- Sleep quality review (detailed per-night breakdowns)
- Metric correlation studies (which factors affect your scores)
- Personalized health insights (your data, your patterns)

Access the comms_face via MODE button rotation or direct navigation. Long-press ALARM to initiate export sequence.

---

#### Phase Score Computation (The Core Algorithm)

**Inputs:**
- Hour (0-23)
- Day of year (1-366)
- Activity level (0-1000, from accelerometer)
- Temperature (°C × 10, from thermistor)
- Light level (lux, from light sensor)

**Algorithm:**
```c
1. Get seasonal baseline from homebase table (pre-computed expected values for your location)
2. Calculate circadian curve: cosine_lut_24[hour] (peaks at 2 PM, troughs at 2 AM)
3. Expected activity = seasonal_baseline * ((1000 + circadian_curve) / 2000)
4. Activity deviation = abs(actual_activity - expected_activity)
5. Temperature deviation = abs(current_temp - expected_temp) / 10
6. Light deviation = abs(current_lux - expected_lux) / 50
7. Phase score = 100 - (activity_dev / 2) - temp_dev - light_dev
8. Clamp to 0-100 range
```

**Integer math only** — no floating point (SAM L22 has no FPU). All operations use fixed-point scaling.

**Weighted Relevance (Smart Surfacing):**
Each zone has a weight vector determining which metrics surface:
```c
//                  SD   EM   WK  NRG  CMF
Emergence:        [30,  25,   5,  10,  30]  // Sleep debt & comfort matter most
Momentum:         [20,  20,  30,  10,  20]  // Wake momentum is key
Active:           [15,  20,   5,  40,  20]  // Energy dominates
Descent:          [10,  35,   0,  10,  45]  // Mood & comfort for wind-down
```

Relevance for each metric: `weight * abs(metric - 50) / 50`

Metrics near 50 (neutral) stay quiet. Metrics at extremes (0 or 100) surface strongly, weighted by zone priority.

---

#### Resource Impact

**Flash:** ~22 KB total (with Phase 4E/4F)
- Homebase table: ~6 KB (365 daily entries with seasonal data)
- Phase engine: ~4 KB (expanded for sleep mode enforcement)
- Metric engine: ~5 KB (added personalized baselines)
- Playlist controller: ~2 KB
- Zone faces: ~3 KB
- Sleep tracking (Phase 4E): ~2 KB (movement frequency analysis, wake detection)

**RAM:** 492 bytes total (with Phase 4E sleep tracking)
- Phase state: 40 bytes (24-hour history buffer)
- Metrics engine: 6 bytes (current scores)
- Playlist state: 16 bytes (rotation + hysteresis)
- Sensor state: 54 bytes (motion, lux, temp buffers)
- Sleep tracking state: 376 bytes (hourly telemetry, 7-day RI history, wake events)

**Power:** ~4 µA average (unchanged)
- LIS2DW12 stationary mode: 2.75 µA
- Lux sampling (1/min): <0.1 µA
- Phase computation (1/15min): ~0.2 µA
- Sleep tracking (integrated with existing sensors): <0.1 µA
- Display updates (zone-change only): ~1 µA amortized

**Build-time:** Zero-cost when disabled — all code is conditionally compiled with `#ifdef PHASE_ENGINE_ENABLED`

---

#### Enabling Phase Engine

**Via Makefile:**
```bash
make BOARD=sensorwatch_green DISPLAY=classic PHASE_ENGINE_ENABLED=1
```

**Via movement_config.h:**
```c
#define PHASE_ENGINE_ENABLED
```

**Via Web Builder:**
Configure homebase settings (latitude/longitude/timezone) — Phase Engine auto-enables when homebase is set.

---

This is a work-in-progress refactor of the Movement firmware for [Sensor Watch](https://www.sensorwatch.net).


Getting dependencies
-------------------------
You will need to install [the GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads/) to build projects for the watch. If you're using Debian or Ubuntu, it should be sufficient to `apt install gcc-arm-none-eabi`.

You will need to fetch the git submodules for this repository too, with `git submodule update --init --recursive` 


Building Second Movement
----------------------------
You can build the default watch firmware with:

```
make BOARD=board_type DISPLAY=display_type
```

where `board_type` is any of:
- sensorwatch_pro
- sensorwatch_green  
- sensorwatch_red (also known as Sensor Watch Lite)
- sensorwatch_blue

and `display_type` is any of:
- classic
- custom

Optionally you can set the watch time when building the firmware using `TIMESET=minute`. 

`TIMESET` can be defined as:
- `year` = Sets the year to the PC's
- `day` = Sets the default time down to the day (year, month, day)
- `minute` = Sets the default time down to the minute (year, month, day, hour, minute)


If you'd like to modify which faces are built and included in the firmware, edit `movement_config.h`. You will get a compilation error if you enable more faces than the watch can store.

Installing firmware to the watch
----------------------------
To install the firmware onto your Sensor Watch board, plug the watch into your USB port and double tap the tiny Reset button on the back of the board. You should see the LED light up red and begin pulsing. (If it does not, make sure you didn’t plug the board in upside down). Once you see the `WATCHBOOT` drive appear on your desktop, type `make install`. This will convert your compiled program to a UF2 file, and copy it over to the watch.

If you want to do this step manually, copy `/build/firmware.uf2` to your watch. 


Emulating the firmware
----------------------------
You may want to test out changes in the emulator first. To do this, you'll need to install [emscripten](https://emscripten.org/), then run:

```
emmake make BOARD=sensorwatch_red DISPLAY=classic
python3 -m http.server -d build-sim
```

Finally, visit [firmware.html](http://localhost:8000/firmware.html) to see your work.
