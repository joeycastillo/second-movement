# Phase Engine Telemetry Export - Implementation Summary

**Date:** 2026-02-21  
**File Modified:** `watch-faces/io/comms_face.c`  
**Feature:** Add Phase Engine hourly sample export to comms_face for dogfooding data collection

## Changes Made

### 1. Added Phase Engine Includes
```c
#ifdef PHASE_ENGINE_ENABLED
#include "../../lib/phase/phase_engine.h"
#include "../../lib/metrics/metrics.h"
#endif
```

### 2. Added Data Structures
Added two new structs (guarded by `#ifdef PHASE_ENGINE_ENABLED`):

- `phase_hourly_sample_t` (6 bytes): Stores hour, zone, phase_score, sd, em, wk, comfort
- `phase_export_t` (146 bytes): Version byte + sample_count + 24 hourly samples

**Size Calculation:**
- Header: 2 bytes (version + sample_count)
- Samples: 24 × 6 = 144 bytes
- **Total: 146 bytes**

### 3. Updated Buffer Sizes
```c
#ifdef PHASE_ENGINE_ENABLED
static uint8_t _export_buffer[256];  // 146 (phase) + 112 (circadian) = 258, round to 256
static char    _hex_buffer[513];     // 256 * 2 + 1 = 513
#else
static uint8_t _export_buffer[112];  // Circadian only
static char    _hex_buffer[225];
#endif
```

### 4. Added Export Function
Implemented `_export_phase_data()` that:
- Returns 0 if buffer too small (graceful fallback)
- Gets current metrics via `metrics_get(NULL, &metrics)`
- Fills 24 hourly samples with current snapshot (MVP limitation)
- Returns `sizeof(phase_export_t)` (146 bytes) on success

**MVP Limitation:** Exports current metrics repeated for all 24 hours. Production version will read from circular buffer.

### 5. Modified Transmission Function
Updated `_start_transmission()` to:
1. Call `_export_phase_data()` first (if enabled)
2. Append circadian data at `_export_buffer + offset`
3. Calculate total export size: `offset + circadian_size`

**Backward Compatibility:** If `PHASE_ENGINE_ENABLED` is not defined, code compiles exactly as before.

## Data Format

### Phase Export Structure
```
Offset | Size | Field
-------|------|-------
0      | 1    | version (0x01)
1      | 1    | sample_count (0-24)
2      | 6    | sample[0]
8      | 6    | sample[1]
...
146    | -    | (circadian data follows)
```

### Hourly Sample (6 bytes)
```
Byte 0: [reserved:1][zone:2][hour:5]
Byte 1: phase_score (0-100)
Byte 2: sd (offset by 60: 0-180 → -60 to +120)
Byte 3: em (0-100)
Byte 4: wk (0-100)
Byte 5: comfort (0-100)
```

## TX Time Estimate

**Phase + Circadian:**
- Binary: 146 + 112 = 258 bytes
- Hex-encoded: 516 characters
- FESK 4FSK rate: 26 bps
- **TX time: ~158 seconds (~2.6 minutes)**

**Circadian only (legacy):**
- Binary: 112 bytes
- Hex-encoded: 224 characters
- **TX time: ~69 seconds (~1.1 minutes)**

## Testing Checklist

- [x] Code compiles with `PHASE_ENGINE_ENABLED=0` (backward compatible)
- [ ] Code compiles with `PHASE_ENGINE_ENABLED=1`
- [ ] Export size verification (146 + 112 = 258 bytes)
- [ ] Actual TX transmission test
- [ ] Data import/decode on receiver side

## Build Command
```bash
make BOARD=sensorwatch_pro DISPLAY=classic PHASE_ENGINE_ENABLED=1
```

## Future Improvements (Week 2+)

1. **Add Hourly Circular Buffer:**
   - Store actual hourly samples in `phase_state_t.phase_history[24]`
   - Add `metrics_history[24]` to metrics engine
   - Read real zone transitions from playlist state

2. **Access Global Phase State:**
   - Link to `movement.c` global phase engine instance
   - Read current zone from playlist state
   - Use actual phase scores instead of default 50

3. **Optimize TX Time:**
   - Consider compression for repeated values
   - Delta encoding for slowly-changing metrics

## Notes

- All Phase Engine code is `#ifdef PHASE_ENGINE_ENABLED` guarded
- Uses integer-only math (no floating point)
- Maintains backward compatibility with circadian-only export
- MVP exports current snapshot repeated (sufficient for week 1 dogfooding)
- Graceful fallback: if phase export fails, still transmits circadian data
