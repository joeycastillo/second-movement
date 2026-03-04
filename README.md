# Second Movement - Phase Engine Edition

A fork of [Sensor Watch](https://github.com/joeycastillo/Sensor-Watch) firmware with integrated **Phase Engine** - a circadian rhythm tracking and optimization system.

## What is Phase Engine?

Phase Engine transforms your Sensor Watch into a **circadian awareness tool**, tracking your daily rhythm through four natural phases and providing intelligent feedback to optimize your energy and productivity.

### The Four Phases

**🌅 Emergence** (Dawn)
- Morning transition and mental clarity
- Best for: Planning, creative thinking, learning

**⚡ Active** (Peak Performance)  
- Maximum energy and focus
- Best for: Physical activity, complex tasks, meetings

**🔥 Momentum** (Sustained Output)
- Consistent productivity phase
- Best for: Execution, collaboration, routine work

**🌙 Descent** (Recovery)
- Wind-down and restoration
- Best for: Reflection, light tasks, rest preparation

### Key Features

**Core Phase Engine:**
- Real-time circadian phase tracking
- Adaptive zone boundaries based on your location and season
- Daily circadian score (0-100 alignment metric)
- Sleep quality tracking and calibration
- Smart alarm with phase-aware wake timing

**Watch Faces:**
- **Zone Faces** - Dedicated face for each phase with contextual info
- **Temperature Forecast** - 7-day forecast with daylight hours toggle
- **Phase Display** - Minimal current phase indicator
- **Circadian Score** - Daily alignment tracking

**Builder Enhancements:**
- Phase Engine toggle in firmware builder
- Persona-based templates (Astronaut, Athlete, Developer, etc.)
- Homebase configuration with city presets
- Config validation (Phase Engine requires location data)

**Quick Settings:**
- Runtime Phase Engine ON/OFF toggle (no reflash needed)
- Located in settings face menu

### How It Works

1. **Homebase Configuration**: Your location (lat/lon/elevation) + timezone
2. **Temperature Data**: Historical climate data for accurate circadian modeling
3. **Seasonal Adaptation**: Zones shift with daylight hours (no hardcoded times)
4. **Personal Calibration**: Sleep tracking refines phase boundaries over time

All calculations use **integer-only math** (no floating-point) and fit within the watch's 256KB flash / 32KB RAM constraints.

## Differences from Upstream

This fork adds:
- Complete Phase Engine system (\`lib/phase/\`)
- 4 zone watch faces (emergence, active, momentum, descent)
- Temperature forecast face with daylight hours
- Smart alarm with circadian awareness
- Builder templates and validation
- Homebase generation tools with city presets
- Quick settings Phase Engine toggle

**Base:** Sensor Watch firmware (upstream compatible)  
**Builder Branch:** \`phase4bc-playlist-dispatch\`

## Quick Start

### 1. Generate Homebase Data

\`\`\`bash
# Use city preset
python3 utils/generate_homebase.py --city anchorage

# Or custom coordinates
python3 utils/generate_homebase.py --lat 61.2181 --lon -149.9003
\`\`\`

### 2. Build Firmware

**Web Builder:** https://dlorp.github.io/second-movement/

Or build locally:
\`\`\`bash
make BOARD=sensorwatch_blue PHASE_ENGINE_ENABLED=1
\`\`\`

### 3. Flash to Watch

Follow standard Sensor Watch flashing process.

### 4. Configure Phase Engine

- Navigate to **settings face**
- Toggle **Phase Engine: ON**
- Set your wake time for initial calibration

## Documentation

- **Architecture:** \`docs/PHASE_ENGINE_DATA_ARCHITECTURE.md\`
- **Homebase Generation:** \`utils/README_HOMEBASE.md\`
- **Flash Size Audit:** \`FLASH_SIZE_AUDIT.md\`

## Building

Requires [emscripten](https://emscripten.org/) for simulator builds:

\`\`\`bash
source ~/emsdk/emsdk_env.sh
make clean
emmake make BOARD=sensorwatch_blue PHASE_ENGINE_ENABLED=1
\`\`\`

Phase Engine can be disabled at compile time by omitting \`PHASE_ENGINE_ENABLED=1\`.

## Credits

- **Original Firmware:** [Joey Castillo - Sensor Watch](https://github.com/joeycastillo/Sensor-Watch)
- **Phase Engine Design & Implementation:** dlorp
- **Builder Framework:** Sensor Watch community
- **Climate Data:** [Open-Meteo API](https://open-meteo.com/)

## License

Same as upstream Sensor Watch - MIT License.

---

**Note:** This is a personal fork with experimental circadian tracking features. For the official Sensor Watch firmware, see the [upstream repository](https://github.com/joeycastillo/Sensor-Watch).
