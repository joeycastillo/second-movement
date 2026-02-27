# Phase Engine Data Architecture

## Overview
Two-tier temperature data system for circadian rhythm tracking.

## 1. Homebase Table (Build-Time)

### Purpose
Permanent baseline for circadian calculations at user's primary location.

### Data Source
- Open-Meteo Historical Weather API
- Open-Meteo Elevation API
- 365/366 days of daily mean temperatures

### Generation
- Tool: `utils/generate_homebase.py`
- When: Build time (manual, before compile)
- Storage: `lib/phase/homebase_table.h` (baked into firmware)

### Data Stored
- Daily mean temperature (°C × 10, int16_t)
- Location metadata (lat, lon, elevation, timezone)
- Seasonal baselines (0-100 scale)
- Year of data

### Usage
- Phase score calculation
- Seasonal energy baselines
- Long-term circadian rhythm tracking

## 2. Travel Table (Runtime)

### Purpose
Temporary 7-day forecast when traveling to new location.

### Data Source (Future)
- Open-Meteo Forecast API (7-day)
- Open-Meteo Elevation API
- Fetched via UnifiedComms face

### Generation
- When: User initiates travel mode
- Storage: Temporary RAM (not flash)
- Lifecycle: Delete after export/transmission

### Data Stored
- 7 days of hourly temperature forecasts
- New location elevation
- Temporary timezone offset

### Usage
- Override circadian calculations while traveling
- Adjust to new location's weather
- Return to homebase table after travel

## Temperature Correction

Both tables apply elevation-based temperature correction:
- Standard: ~6.5°C drop per 1000m elevation gain
- Formula: `adjusted_temp = api_temp - (elevation_m / 1000 * 6.5)`

## API Endpoints

### Historical Weather (Homebase)
```
https://archive-api.open-meteo.com/v1/archive
?latitude=X&longitude=Y
&start_date=2024-01-01&end_date=2024-12-31
&daily=temperature_2m_mean
&timezone=auto
```

### Forecast (Travel) - Future
```
https://api.open-meteo.com/v1/forecast
?latitude=X&longitude=Y
&hourly=temperature_2m
&forecast_days=7
&timezone=auto
```

### Elevation
```
https://api.open-meteo.com/v1/elevation
?latitude=X&longitude=Y
```

## File Organization

```
second-movement/
├── utils/
│   └── generate_homebase.py          # Homebase table generator
├── lib/
│   └── phase/
│       ├── homebase_table.h          # Generated homebase data
│       └── travel_table.h            # Future: Runtime travel data
├── movement/
│   └── watch_faces/
│       └── io/
│           └── comms_face.c          # Future: UnifiedComms implementation
└── docs/
    └── PHASE_ENGINE_DATA_ARCHITECTURE.md  # This file
```

## Future: UnifiedComms Face

### Features
- Fetch 7-day forecast for new location
- Temporary storage (RAM only)
- Export/transmit data
- Auto-delete after use
- Elevation-aware temperature correction

### Implementation Notes
- Don't override homebase table
- Use separate data structure
- Clear travel data on:
  - Successful export
  - Return to homebase mode
  - Manual clear command

## Developer Notes

### Generating Homebase Table
```bash
cd utils
python3 generate_homebase.py \
  --lat 61.2181 \
  --lon -149.9003 \
  --tz AKST \
  --year 2024
```

### Offline Mode (Testing)
```bash
python3 generate_homebase.py \
  --lat 61.2181 \
  --lon -149.9003 \
  --tz AKST \
  --year 2024 \
  --skip-api  # Forces sinusoidal fallback
```

### Dependencies
- Python 3.7+
- requests library: `pip install requests`
