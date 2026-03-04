#!/usr/bin/env python3
import requests
import argparse
import json
from datetime import datetime

def fetch_forecast(lat, lon, days=7):
    url = "https://api.open-meteo.com/v1/forecast"
    params = {
        "latitude": lat,
        "longitude": lon,
        "daily": "temperature_2m_mean,daylight_duration",
        "forecast_days": days,
        "timezone": "auto"
    }
    
    response = requests.get(url, params=params)
    response.raise_for_status()
    data = response.json()
    
    results = []
    for i in range(days):
        temp_c = data['daily']['temperature_2m_mean'][i]
        daylight_sec = data['daily']['daylight_duration'][i]
        
        entry = {
            'expected_daylight_min': int(daylight_sec / 60),
            'avg_temp_c10': int(temp_c * 10),
            'seasonal_baseline': calculate_baseline(temp_c)
        }
        results.append(entry)
    
    return results

def calculate_baseline(temp_c):
    # Simple heuristic: 50 + (temp / 2), clamped to 0-100
    baseline = int(50 + (temp_c / 2))
    return max(0, min(100, baseline))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--lat', type=float, required=True)
    parser.add_argument('--lon', type=float, required=True)
    parser.add_argument('--days', type=int, default=7)
    parser.add_argument('--format', choices=['json', 'c'], default='json')
    args = parser.parse_args()
    
    forecast = fetch_forecast(args.lat, args.lon, args.days)
    
    if args.format == 'json':
        print(json.dumps(forecast, indent=2))
    else:
        print("homebase_entry_t forecast_table[7] = {")
        for entry in forecast:
            print(f"    {{{entry['expected_daylight_min']}, {entry['avg_temp_c10']}, {entry['seasonal_baseline']}}},")
        print("};")
