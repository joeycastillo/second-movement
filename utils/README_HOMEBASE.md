# Homebase Generator

Generate location-specific lookup tables for the Phase Engine's circadian tracking features.

## Quick Start with City Presets

For common cities, use the `--city` preset:

```bash
# Anchorage, AK
python3 utils/generate_homebase.py --city anchorage

# Portland, OR
python3 utils/generate_homebase.py --city portland

# Dallas, TX
python3 utils/generate_homebase.py --city dallas

# New York, NY
python3 utils/generate_homebase.py --city ny
```

## Custom Coordinates

For other locations, specify coordinates manually:

```bash
python3 utils/generate_homebase.py --lat 47.6062 --lon -122.3321 --tz PST
```

## Available City Presets

| City | Coordinates | Timezone |
|------|-------------|----------|
| anchorage | 61.2181°N, 149.9003°W | AKST |
| portland | 45.5152°N, 122.6784°W | PST |
| dallas | 32.7767°N, 96.7970°W | CST |
| ny | 40.7128°N, 74.0060°W | EST |

## Options

- `--city` - Use a preset city (anchorage, portland, dallas, ny)
- `--lat` - Latitude in degrees (-90 to 90)
- `--lon` - Longitude in degrees (-180 to 180)
- `--tz` - Timezone (AKST, PST, CST, EST, UTC±X, or minutes offset)
- `--year` - Year for generation (default: 2026, range: 2000-2099)
- `--output` - Output path (default: lib/phase/homebase_table.h)
- `--skip-api` - Skip Open-Meteo API and use sinusoidal temperature model

## Output

Generates `lib/phase/homebase_table.h` with:
- Expected daylight duration for each day of year
- Average temperature for each day of year
- Seasonal energy baseline (0-100 scale)

All values use integer math for embedded efficiency.

## Examples

```bash
# Using preset with custom year
python3 utils/generate_homebase.py --city anchorage --year 2027

# Custom location with offline mode
python3 utils/generate_homebase.py --lat 51.5074 --lon -0.1278 --tz UTC --skip-api

# Specify custom output path
python3 utils/generate_homebase.py --city ny --output /tmp/homebase.h
```

## Adding New City Presets

Edit the `CITY_PRESETS` dictionary in `generate_homebase.py`:

```python
CITY_PRESETS = {
    'london': {
        'name': 'London, UK',
        'lat': 51.5074,
        'lon': -0.1278,
        'tz': 'UTC'
    }
}
```

Then use with `--city london`.
