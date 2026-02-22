Second Movement
===============

**Note:** This is my fork of second-movement. See upstream at [joeycastillo/second-movement](https://github.com/joeycastillo/second-movement).

## Sites

| Site | Description |
|------|-------------|
| [**Firmware Builder**](https://dlorp.github.io/second-movement/builder/) | Web UI - pick faces, configure settings, dispatch a GitHub Actions build, download UF2 |
| [**Companion App**](https://dlorp.github.io/second-movement/companion-app/) | Optical TX web app - send data to the watch via screen flash (Timex DataLink style) |

## Fork Changes

### Tap-to-Wake Feature
Added crystal tap detection for instant display wake on Sensor Watch Pro boards:
- Hybrid implementation using motion wake (INT2/A4) + software tap polling
- Both crystal tap and wrist raise wake the display from sleep
- Uses LIS2DW12 accelerometer's hardware tap detection (400Hz, Z axis threshold 12)
- Power optimized: Low power Mode 1 (~45-90µA continuous draw)
- Graceful fallback on Green boards (no accelerometer)
- Documentation: TAP_TO_WAKE.md and TAP_TO_WAKE_SLEEP_MODE_ANALYSIS.md

**Hardware constraint:** LIS2DW12 tap interrupts can only route to INT1 (A3), which is not an RTC wake pin. Solution uses motion detection on INT2 (A4) to wake from STANDBY sleep, then polls tap register to distinguish tap events from wrist movement.

### Active Hours Feature
**What it means:** Active Hours define when you're awake and active. Outside these hours (your sleep window), the watch enters a low power mode that prevents accidental display wakes from rolling over in bed.

**What changes during sleep hours:**
- **Motion wake disabled:** Wrist movement no longer triggers display wake (prevents false wakes from tossing/turning)
- **Tap to wake still works:** Crystal tap always wakes display (intentional gesture)
- **Button wake always works:** Physical buttons always wake display
- **Sleep tracking enabled:** Orientation changes logged for sleep quality metrics (Stream 4)
- **Smart alarm active:** Light sleep detection begins 15 min before alarm window (Stream 3)

**Default configuration:**
- Active Hours: 04:00 - 23:00 (awake)
- Sleep Window: 23:00 - 04:00 (sleeping)
- User-configurable via settings face (15 minute increments)

**Components:**
- **Stream 1 (Core Logic):** Sleep detection and motion wake suppression
- **Stream 2 (Settings UI):** User configurable Active Hours in settings face
- **Stream 3 (Smart Alarm):** Up band style alarm with light sleep detection
- **Stream 4 (Sleep Tracking):** Orientation logging for sleep quality (70 bytes, 7 nights)

**Power impact:**
- Deep sleep mode: Minimal power draw (accelerometer at 1.6Hz stationary mode)
- Sleep tracking overhead: Under 5µA (orientation logging only)
- Smart alarm prewake: Ramps to 45-90µA for 15 min before alarm window

**Persistence:**
Settings stored in BKUP[2] register (battery backed RAM). Survives normal power cycles, resets on complete battery removal (standard Sensor Watch behavior).

### Sleep & Circadian Tracking System
**What it is:** Research backed sleep detection + circadian health scoring for long term pattern tracking. Uses time / active hours + accelerometer motion detection + ambient light sensor to distinguish sleep from wake, then calculates health metrics over 7-day rolling windows.

**Three watch faces:**

**1. Sleep Tracker Face (SLP)**
- **During sleep window:** Shows live tracking metrics (duration, efficiency, WASO, awakenings)
- **After wake-up:** Adds Sleep Score (SL 0-100) as first display mode
- **Sleep Score formula:** 50% duration + 30% efficiency + 20% light exposure
- **ALARM button:** Cycles SL → Duration → Efficiency → WASO → Awakenings
- **Algorithm:** Cole-Kripke (1992) + Light Enhancement (11 min sliding window, 85-90% accuracy target)

**2. Circadian Score Face (CS)**
- **Overall score:** CS 0-100 (7 day aggregate, 75% evidence based)
- **ALARM button:** Drill down to 5 subscores (TI/DU/EF/AH/LI)
- **Components:**
  - **TI (Timing):** Sleep Regularity Index - 35% weight (Phillips et al. 2017)
  - **DU (Duration):** Sleep duration penalty - 30% (Cappuccio U curve, 7-8h optimal)
  - **EF (Efficiency):** Sleep efficiency - 20% (% time asleep in bed)
  - **AH (Active Hours):** Compliance with window - 10% (onset/offset within 1h)
  - **LI (Light):** Light exposure quality - 5% (% time in darkness)

**Data flow:**
- Sleep tracking runs automatically 23:00-04:00 (synced with Active Hours from BKUP[2])
- At window end: metrics exported to 7 day rolling buffer (flash row 30, ~200 bytes)
- Circadian Score calculated on demand when CS face is activated
- Power: ~4-5µA during sleep (LIS2DW12 Low Power Mode 1 + light ADC)

**Smart alarm integration:**
- Smart alarm (if enabled) can query sleep_tracker state for light sleep detection
- Accelerometer motion patterns indicate light vs deep sleep phases
- Alarm triggers during light sleep within configured window for gentler wake

**Face navigation:**
0. Wyoscan
1. **Sleep Tracker** (live + review with SL score)
2. **Circadian Score** (7-day CS + drill-down)
3. World Clock, Sunrise/Sunset, etc.

### Phase Engine: Chronomantic Instrument
> ⚠️ **Status:** In active development and testing. This is experimental firmware expect bugs and behavioral changes as the system is tuned.

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

#### The Four Zones

Phase score maps to zones that represent your biological state:

| Zone | Phase Score | What It Means | Example Time* |
|------|-------------|---------------|---------------|
| **Emergence** | 0-25 | Waking up, orienting, building alertness | Early morning (4-8 AM) |
| **Momentum** | 26-50 | Building energy, ramping activity | Late morning (9-12 PM) |
| **Active** | 51-75 | Peak capacity, maximum output | Afternoon (1-5 PM) |
| **Descent** | 76-100 | Winding down, preparing for rest | Evening (6-11 PM) |

***Note:** Times are examples for a typical schedule. Phase score is based on YOUR biological rhythm, not the clock. A night-shift worker's Emergence might happen at 8 PM.

**Zone Faces:**
Each zone has a dedicated watch face that displays metrics relevant to that biological state:

- **Emergence Face** (`EM`): Shows Sleep Debt (SD), Emotional state (EM), Comfort (CMF)
- **Momentum Face** (`MO`): Shows Wake Momentum (WK), Sleep Debt, Temperature
- **Active Face** (`AC`): Shows Energy capacity (NRG), Emotional state, Sleep Debt
- **Descent Face** (`DE`): Shows Comfort, Emotional state (ready for rest)

**Playlist Controller:**
- **Manual:** Long-Press ALARM button to acess and through zone-specific metrics
- **Automatic:** Watch switches zone faces when phase score crosses zone boundaries (debounced over 3 ticks to prevent jitter)
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
  - **Circadian (40%):** Cosine curve peaking at 2 PM, trough at 2 AM (`cosine_lut_24[hour]`)
  - **Lunar (20%):** Approximates lunar phase from day of year, peaks near full moon
  - **Variance (40%):** Activity variance over last 15 min (high variance during expected-rest = penalty)
- **Updates:** Every 15 minutes
- **Data source:** Hour-of-day, day-of-year, accelerometer variance

**3. Wake Momentum (WK)**
- **Meaning:** How ramped up you are since waking
- **Range:** 0 = just woke, 100 = fully alert
- **Math:** Base ramp: `min(100, minutes_awake * 100 / 120)` (full ramp in 2 hours). Activity bonus accelerates ramp up to +30%. Sedentary penalty caps at 60.
- **Updates:** Every minute for first 2 hours after wake, then every 15 min
- **Data source:** Wake onset time (from Active Hours transition) + cumulative activity

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

**6. Jet Lag (JL)** *(Deferred to future release)*
- **Meaning:** Timezone disruption and recovery
- **Range:** 0 = no shift, 100 = maximum disruption
- **Math:** On timezone change: `JL = abs(hours_shifted) * 12` (capped at 100). Decay: `JL -= max(1, JL / 24)` per hour (roughly 1 day per hour of shift to recover)

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

**Flash:** ~18 KB total
- Homebase table: ~6 KB (365 daily entries with seasonal data)
- Phase engine: ~3 KB
- Metric engine: ~4 KB
- Playlist controller: ~2 KB
- Zone faces: ~3 KB

**RAM:** 72 bytes total
- Phase state: 40 bytes (24-hour history buffer)
- Metrics engine: 6 bytes (current scores)
- Playlist state: 16 bytes (rotation + hysteresis)
- Sensor state: 54 bytes (motion, lux, temp buffers)

**Power:** ~4 µA average
- LIS2DW12 stationary mode: 2.75 µA
- Lux sampling (1/min): <0.1 µA
- Phase computation (1/15min): ~0.2 µA
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
