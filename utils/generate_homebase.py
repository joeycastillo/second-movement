#!/usr/bin/env python3
"""
Homebase Table Generator for Phase Watch

Generates a location-specific lookup table (LUT) for seasonal data:
- Expected daylight duration (sunrise to sunset)
- Average temperature for each day of year
- Seasonal energy baseline (0-100 scale)

All values use integer math (no floating point) for embedded efficiency.

Usage:
    python3 generate_homebase.py --lat 37.7749 --lon -122.4194 --tz PST --year 2026
    python3 generate_homebase.py --help

Output:
    lib/phase/homebase_table.h (C header with const arrays)
"""

import argparse
import math
import sys
from datetime import datetime, timedelta
from pathlib import Path
import requests


def fetch_elevation(latitude, longitude):
    """
    Fetch elevation data from Open-Meteo Elevation API.
    
    Args:
        latitude: Latitude in degrees (-90 to 90)
        longitude: Longitude in degrees (-180 to 180)
    
    Returns:
        Elevation in meters (float)
        Returns None on error
    """
    try:
        url = "https://api.open-meteo.com/v1/elevation"
        params = {
            'latitude': latitude,
            'longitude': longitude
        }
        
        response = requests.get(url, params=params, timeout=10)
        response.raise_for_status()
        
        data = response.json()
        
        if 'elevation' not in data:
            print("⚠️  API response missing elevation data", file=sys.stderr)
            return None
        
        elevation = data['elevation']
        
        # API returns elevation as a list with a single value
        if isinstance(elevation, list):
            if len(elevation) == 0:
                print("⚠️  API returned empty elevation list", file=sys.stderr)
                return None
            elevation = elevation[0]
        
        if elevation is None or not isinstance(elevation, (int, float)):
            print("⚠️  Invalid elevation value from API", file=sys.stderr)
            return None
        
        return float(elevation)
        
    except requests.exceptions.Timeout:
        print("⚠️  Elevation API request timed out (>10s)", file=sys.stderr)
        return None
    except requests.exceptions.RequestException as e:
        print(f"⚠️  Elevation API request failed: {e}", file=sys.stderr)
        return None
    except (KeyError, ValueError, TypeError) as e:
        print(f"⚠️  Failed to parse elevation API response: {e}", file=sys.stderr)
        return None


def fetch_temperature_data(latitude, longitude, year):
    """
    Fetch historical temperature data from Open-Meteo API.
    
    Args:
        latitude: Latitude in degrees (-90 to 90)
        longitude: Longitude in degrees (-180 to 180)
        year: Year to fetch data for
    
    Returns:
        List of 365/366 daily average temperatures (float, in Celsius)
        Returns None on error
    """
    try:
        # Determine if leap year
        is_leap = (year % 4 == 0 and year % 100 != 0) or (year % 400 == 0)
        days_in_year = 366 if is_leap else 365
        
        # Build API request
        url = "https://archive-api.open-meteo.com/v1/archive"
        params = {
            'latitude': latitude,
            'longitude': longitude,
            'start_date': f'{year}-01-01',
            'end_date': f'{year}-12-31',
            'daily': 'temperature_2m_mean',
            'timezone': 'auto'
        }
        
        # Make request with timeout
        response = requests.get(url, params=params, timeout=10)
        response.raise_for_status()
        
        # Parse response
        data = response.json()
        
        if 'daily' not in data or 'temperature_2m_mean' not in data['daily']:
            print("⚠️  API response missing temperature data", file=sys.stderr)
            return None
        
        temps = data['daily']['temperature_2m_mean']
        
        # Validate we got the right number of days
        if len(temps) != days_in_year:
            print(f"⚠️  Expected {days_in_year} days, got {len(temps)}", file=sys.stderr)
            return None
        
        # Convert to float list (API may return None for missing data)
        result = []
        for temp in temps:
            if temp is None:
                print("⚠️  API returned None for some temperature values", file=sys.stderr)
                return None
            result.append(float(temp))
        
        return result
        
    except requests.exceptions.Timeout:
        print("⚠️  API request timed out (>10s)", file=sys.stderr)
        return None
    except requests.exceptions.RequestException as e:
        print(f"⚠️  API request failed: {e}", file=sys.stderr)
        return None
    except (KeyError, ValueError, TypeError) as e:
        print(f"⚠️  Failed to parse API response: {e}", file=sys.stderr)
        return None


def calculate_daylight_minutes(latitude, day_of_year):
    """
    Calculate daylight duration using simplified sunrise equation.
    Based on NOAA solar calculations, integer-approximated.
    
    Args:
        latitude: Latitude in degrees (-90 to 90)
        day_of_year: Day of year (1-365)
    
    Returns:
        Daylight duration in minutes (integer)
    """
    # Solar declination (degrees)
    # Approximation: -23.44 * cos(360/365 * (day + 10))
    declination_deg = -23.44 * math.cos(math.radians(360.0 / 365.0 * (day_of_year + 10)))
    
    # Hour angle at sunrise (degrees)
    # cos(hour_angle) = -tan(latitude) * tan(declination)
    lat_rad = math.radians(latitude)
    decl_rad = math.radians(declination_deg)
    
    try:
        cos_hour_angle = -math.tan(lat_rad) * math.tan(decl_rad)
        # Clamp to [-1, 1] for extreme latitudes
        cos_hour_angle = max(-1.0, min(1.0, cos_hour_angle))
        hour_angle_deg = math.degrees(math.acos(cos_hour_angle))
    except (ValueError, ZeroDivisionError):
        # Extreme latitude or polar day/night - use reasonable defaults
        if abs(latitude) > 66.5:  # Arctic/Antarctic circles
            if -90 <= day_of_year <= 90 or day_of_year >= 270:
                hour_angle_deg = 0 if latitude > 0 else 180  # Winter
            else:
                hour_angle_deg = 180 if latitude > 0 else 0  # Summer
        else:
            hour_angle_deg = 90  # ~12 hours as fallback
    
    # Daylight duration: 2 * hour_angle / 15 (hours) * 60 (minutes)
    daylight_hours = 2.0 * hour_angle_deg / 15.0
    daylight_minutes = int(daylight_hours * 60)
    
    return max(0, min(1440, daylight_minutes))  # Clamp to [0, 1440]


def calculate_avg_temp_c10(latitude, day_of_year, base_temp_c=15):
    """
    Calculate approximate average temperature for a day.
    Uses simple sinusoidal model with latitude adjustment.
    
    Args:
        latitude: Latitude in degrees (-90 to 90)
        day_of_year: Day of year (1-365)
        base_temp_c: Base annual average temperature (celsius)
    
    Returns:
        Temperature * 10 (integer, e.g., 125 = 12.5°C)
    """
    # Seasonal variation (peak in summer, trough in winter)
    # Northern hemisphere: coldest ~day 15 (mid-January), warmest ~day 197 (mid-July)
    # Southern hemisphere: flip the phase
    # phase_offset is where sine crosses zero upward, so minimum is 91 days earlier
    phase_offset = 106 if latitude >= 0 else 289
    seasonal_swing_c = 10 + abs(latitude) / 3.0  # Higher swing at higher latitudes
    
    temp_variation = seasonal_swing_c * math.sin(
        math.radians(360.0 / 365.0 * (day_of_year - phase_offset))
    )
    
    temp_c = base_temp_c + temp_variation
    return int(temp_c * 10)


def calculate_seasonal_baseline(day_of_year, latitude):
    """
    Calculate seasonal energy baseline (0-100).
    Represents expected circadian energy level based on season.
    
    Northern hemisphere: higher in summer, lower in winter.
    Southern hemisphere: inverted.
    
    Args:
        day_of_year: Day of year (1-365)
        latitude: Latitude in degrees
    
    Returns:
        Baseline score (0-100)
    """
    # Peak energy in summer (longer days), low in winter
    phase_offset = 172 if latitude >= 0 else 355  # Summer solstice
    
    # Sinusoidal variation from 30 (winter) to 100 (summer)
    variation = 35 * math.sin(
        math.radians(360.0 / 365.0 * (day_of_year - phase_offset))
    )
    
    baseline = 65 + variation  # Center at 65, swing ±35
    return int(max(0, min(100, baseline)))


def generate_homebase_table(latitude, longitude, timezone_offset, year, output_path, tz_string="N/A", skip_api=False):
    """
    Generate homebase_table.h with 365 days of seasonal data.
    
    Args:
        latitude: Latitude in degrees (-90 to 90)
        longitude: Longitude in degrees (-180 to 180)
        timezone_offset: Timezone offset in minutes (e.g., -480 for PST)
        year: Year for generation (informational only)
        output_path: Path to output .h file
        tz_string: Original timezone string for display purposes
        skip_api: If True, skip API and use sinusoidal fallback
    """
    # Convert coordinates to scaled integers for metadata
    lat_e6 = int(latitude * 1_000_000)
    lon_e6 = int(longitude * 1_000_000)
    
    # Estimate base temperature from latitude (rough approximation)
    # Tropical: ~25°C, Temperate: ~15°C, Polar: ~0°C
    base_temp = 25 - abs(latitude) * 0.4
    
    # Fetch elevation data first
    elevation_m = None
    elevation_text = "unknown (API unavailable)"
    
    if not skip_api:
        print("📡 Fetching elevation from Open-Meteo API...")
        elevation_m = fetch_elevation(latitude, longitude)
        if elevation_m is not None:
            elevation_text = f"{elevation_m:.0f} meters (from Open-Meteo Elevation API)"
            print(f"✓ Elevation: {elevation_m:.0f} meters")
        else:
            print("⚠️  Elevation API fetch failed")
    else:
        print("⏭️  Skipping elevation API (--skip-api flag set)")
    
    # Try to fetch real temperature data from Open-Meteo API
    data_source = "sinusoidal model (fallback)"
    temps_from_api = None
    
    if not skip_api:
        print("📡 Fetching temperature data from Open-Meteo API...")
        temps_from_api = fetch_temperature_data(latitude, longitude, year)
        if temps_from_api:
            data_source = "Open-Meteo Historical Weather API"
            print(f"✓ Successfully fetched {len(temps_from_api)} days of temperature data")
        else:
            print("⚠️  API fetch failed, falling back to sinusoidal model")
    else:
        print("⏭️  Skipping API (--skip-api flag set), using sinusoidal model")
    
    # Generate 365 entries
    entries = []
    for day in range(1, 366):
        daylight_min = calculate_daylight_minutes(latitude, day)
        
        # Use API data if available, otherwise fallback to sinusoidal model
        if temps_from_api:
            temp_c = temps_from_api[day - 1]
            temp_c10 = int(temp_c * 10)
        else:
            temp_c10 = calculate_avg_temp_c10(latitude, day, base_temp)
        
        baseline = calculate_seasonal_baseline(day, latitude)
        entries.append((daylight_min, temp_c10, baseline))
    
    # Generate C header file
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    header = f"""/*
 * GENERATED FILE - DO NOT EDIT MANUALLY
 * 
 * Generated by: utils/generate_homebase.py
 * Generation time: {timestamp}
 * 
 * Homebase configuration:
 * Latitude: {latitude:.4f}°{'N' if latitude >= 0 else 'S'}
 * Longitude: {longitude:.4f}°{'E' if longitude >= 0 else 'W'}
 * Location elevation: {elevation_text}
 * Timezone offset: {timezone_offset} minutes (UTC{timezone_offset//60:+d})
 * Year: {year}
 * Temperature data source: {data_source}
 */

#ifndef HOMEBASE_TABLE_H_
#define HOMEBASE_TABLE_H_

#include "phase_engine.h"

#ifdef PHASE_ENGINE_ENABLED

// Homebase metadata
static const homebase_metadata_t homebase_metadata = {{
    .latitude_e6 = {lat_e6},
    .longitude_e6 = {lon_e6},
    .timezone_offset = {timezone_offset},
    .year = {year},
    .entry_count = 365
}};

// Homebase table (365 entries, one per day of year)
// Entry format: {{daylight_minutes, temp_c10, seasonal_baseline}}
static const homebase_entry_t homebase_table[365] = {{
"""
    
    # Write entries in groups of 4 for readability
    for i, (daylight, temp, baseline) in enumerate(entries, 1):
        if (i - 1) % 4 == 0:
            header += "    "
        header += f"{{{daylight}, {temp}, {baseline}}}"
        if i < 365:
            header += ", "
        if i % 4 == 0 and i < 365:
            header += f" // Days {i-3}-{i}\n"
    
    header += """  // Day 365
};

// Accessor functions (static inline to avoid linker errors)
static inline const homebase_entry_t* homebase_get_entry(uint16_t day_of_year) {
    if (day_of_year < 1 || day_of_year > 365) {
        return &homebase_table[0];  // Safe fallback
    }
    return &homebase_table[day_of_year - 1];
}

static inline const homebase_metadata_t* homebase_get_metadata(void) {
    return &homebase_metadata;
}

#endif // PHASE_ENGINE_ENABLED

#endif // HOMEBASE_TABLE_H_
"""
    
    # Write to file
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(header)
    
    # Calculate detailed statistics
    table_data_bytes = len(entries) * 6  # 3 uint16_t per entry = 6 bytes
    metadata_bytes = 16  # homebase_metadata_t struct size
    total_code_bytes = len(header)
    
    # Daylight statistics
    daylight_values = [e[0] for e in entries]
    min_daylight = min(daylight_values)
    max_daylight = max(daylight_values)
    avg_daylight = sum(daylight_values) // len(daylight_values)
    
    # Temperature statistics
    temp_values = [e[1] for e in entries]
    min_temp = min(temp_values) / 10.0
    max_temp = max(temp_values) / 10.0
    avg_temp = sum(temp_values) / len(temp_values) / 10.0
    
    # Print enhanced statistics
    print("=" * 70)
    print("PHASE ENGINE HOMEBASE TABLE GENERATION COMPLETE")
    print("=" * 70)
    print(f"\n📍 LOCATION:")
    print(f"   Latitude:  {latitude:+8.4f}° {'N' if latitude >= 0 else 'S'}")
    print(f"   Longitude: {longitude:+8.4f}° {'E' if longitude >= 0 else 'W'}")
    print(f"   Timezone:  UTC{timezone_offset//60:+d} ({tz_string})")
    print(f"   Year:      {year}")
    
    print(f"\n📊 DAYLIGHT STATISTICS:")
    print(f"   Shortest day: {min_daylight:3d} minutes ({min_daylight//60:2d}h {min_daylight%60:02d}m)")
    print(f"   Longest day:  {max_daylight:3d} minutes ({max_daylight//60:2d}h {max_daylight%60:02d}m)")
    print(f"   Average:      {avg_daylight:3d} minutes ({avg_daylight//60:2d}h {avg_daylight%60:02d}m)")
    print(f"   Variation:    {max_daylight - min_daylight:3d} minutes")
    
    print(f"\n🌡️  TEMPERATURE RANGE:")
    print(f"   Minimum: {min_temp:5.1f}°C ({min_temp*9/5+32:5.1f}°F)")
    print(f"   Maximum: {max_temp:5.1f}°C ({max_temp*9/5+32:5.1f}°F)")
    print(f"   Average: {avg_temp:5.1f}°C ({avg_temp*9/5+32:5.1f}°F)")
    
    print(f"\n💾 FLASH MEMORY IMPACT:")
    print(f"   Table data:   {table_data_bytes:5d} bytes (365 entries × 6 bytes)")
    print(f"   Metadata:     {metadata_bytes:5d} bytes")
    print(f"   Total header: {total_code_bytes:5d} bytes (includes accessor functions)")
    print(f"   ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print(f"   TOTAL ADDED:  {table_data_bytes + metadata_bytes:5d} bytes to firmware")
    
    print(f"\n📄 OUTPUT:")
    print(f"   File: {output_path}")
    print(f"   Size: {total_code_bytes:,} bytes")
    
    # Sample data points
    print(f"\n📅 SAMPLE DATA POINTS:")
    print(f"   {'Day':>4} │ {'Daylight':>12} │ {'Temperature':>12} │ {'Baseline':>8}")
    print(f"   ─────┼──────────────┼──────────────┼──────────")
    for day in [1, 90, 180, 270, 365]:
        daylight, temp, baseline = entries[day - 1]
        hr, mn = divmod(daylight, 60)
        print(f"   {day:4d} │ {hr:2d}h {mn:02d}m ({daylight:3d}m) │ "
              f"{temp/10:5.1f}°C ({temp/10*9/5+32:5.1f}°F) │ {baseline:3d}/100")
    
    print("\n" + "=" * 70)
    print("✓ Homebase table ready for build")
    print("=" * 70)


def parse_timezone(tz_string):
    """
    Parse timezone string to offset in minutes.
    Supports: PST, EST, AKST, HST, UTC+X, UTC-X, or raw offset.
    """
    tz_map = {
        # Alaska
        'AKST': -540, 'AKDT': -480,  # Alaska Standard/Daylight Time
        # Hawaii-Aleutian
        'HST': -600, 'HAST': -600, 'HADT': -540,  # Hawaii Standard/Daylight Time
        # Pacific
        'PST': -480, 'PDT': -420,
        # Mountain
        'MST': -420, 'MDT': -360,
        # Central
        'CST': -360, 'CDT': -300,
        # Eastern
        'EST': -300, 'EDT': -240,
        # Atlantic
        'AST': -240, 'ADT': -180,  # Atlantic Standard/Daylight Time
        # UTC/GMT
        'UTC': 0, 'GMT': 0,
    }
    
    tz_upper = tz_string.upper().strip()
    
    if tz_upper in tz_map:
        return tz_map[tz_upper]
    
    # Try parsing UTC+X or UTC-X
    if tz_upper.startswith('UTC'):
        offset_str = tz_upper[3:]
        try:
            hours = float(offset_str)
            return int(hours * 60)
        except ValueError:
            pass
    
    # Try parsing as raw integer
    try:
        return int(tz_string)
    except ValueError:
        raise ValueError(f"Unknown timezone format: {tz_string}")


def main():
    parser = argparse.ArgumentParser(
        description="Generate homebase table for Phase Watch",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # San Francisco
  python3 generate_homebase.py --lat 37.7749 --lon -122.4194 --tz PST --year 2026
  
  # New York
  python3 generate_homebase.py --lat 40.7128 --lon -74.0060 --tz EST --year 2026
  
  # Anchorage, Alaska
  python3 generate_homebase.py --lat 61.2181 --lon -149.9003 --tz AKST --year 2026
  
  # London
  python3 generate_homebase.py --lat 51.5074 --lon -0.1278 --tz UTC --year 2026
  
  # Tokyo
  python3 generate_homebase.py --lat 35.6762 --lon 139.6503 --tz UTC+9 --year 2026
        """
    )
    
    parser.add_argument('--lat', type=float, required=True,
                        help='Latitude in degrees (-90 to 90)')
    parser.add_argument('--lon', type=float, required=True,
                        help='Longitude in degrees (-180 to 180)')
    parser.add_argument('--tz', type=str, required=True,
                        help='Timezone (AKST, PST, EST, HST, UTC+X, or minutes offset)')
    parser.add_argument('--year', type=int, default=2026,
                        help='Year for generation (default: 2026, range: 2000-2099)')
    parser.add_argument('--output', type=Path,
                        default=Path(__file__).parent.parent / 'lib' / 'phase' / 'homebase_table.h',
                        help='Output path (default: lib/phase/homebase_table.h)')
    parser.add_argument('--skip-api', action='store_true',
                        help='Skip Open-Meteo API and use sinusoidal temperature model (offline mode)')
    
    args = parser.parse_args()
    
    # Validate inputs
    # Note: Year range limitation is due to watch-library date handling
    # which uses uint16_t for year storage. The valid range 2000-2099
    # matches the century digits stored in RTC BCD registers (SAM L22).
    if not 2000 <= args.year <= 2099:
        print(f"Error: Year must be in range [2000, 2099], got {args.year}", file=sys.stderr)
        print("Note: This limitation is due to Sensor Watch RTC hardware constraints.", file=sys.stderr)
        return 1
    
    if not -90 <= args.lat <= 90:
        print(f"Error: Latitude must be in range [-90, 90], got {args.lat}", file=sys.stderr)
        return 1
    
    if not -180 <= args.lon <= 180:
        print(f"Error: Longitude must be in range [-180, 180], got {args.lon}", file=sys.stderr)
        return 1
    
    try:
        tz_offset = parse_timezone(args.tz)
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1
    
    # Generate table
    generate_homebase_table(args.lat, args.lon, tz_offset, args.year, args.output, args.tz, args.skip_api)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
