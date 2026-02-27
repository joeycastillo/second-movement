# Zone Faces LCD Fix - COMPLETE ✅

## Task Summary
Fixed all 4 zone faces to work with classic F91W LCD constraints (2-char top line, 6-char bottom line).

## Repository
- **Path:** ~/repos/second-movement
- **Branch:** phase3-pr4-zone-faces
- **Spec:** /tmp/zone_faces_display_design.md

## Files Modified ✅

### 1. watch-faces/complication/emergence_face.c
- Zone: "EM" → "ER" 
- Sleep Debt: `SD  %2d` → `SD %+3d` (signed ±90)
- Emotional: `EM  %2d` → `EM %3d`
- Comfort: `CMF %2d` → `CF %3d`

**Display:**
```
ER           ER           ER
SD -90       EM  35       CF  72
```

### 2. watch-faces/complication/momentum_face.c
- Workload: `WK  %2d` → `WK %3d`
- Sleep Debt: `SD  %2d` → `SD %+3d`
- Third metric: Temperature → Emotional (`EM %3d`)
- Cleaned up unused includes

**Display:**
```
MO           MO           MO
WK  45       SD -30       EM  25
```

### 3. watch-faces/complication/active_face.c
- Energy: `NRG %2d` → `EN %3d`
- Emotional: `EM  %2d` → `EM %3d`
- Sleep Debt: `SD  %2d` → `SD %+3d`

**Display:**
```
AC           AC           AC
EN  87       EM  35       SD +15
```

### 4. watch-faces/complication/descent_face.c
- Comfort: `CMF %2d` → `CF %3d`
- Emotional: `EM  %2d` → `EM %3d`
- Added Sleep Debt as 3rd metric: `SD %+3d`
- View count: 2 → 3

**Display:**
```
DE           DE           DE
CF  72       EM  40       SD -20
```

## All Formats Now Correct

| Metric | Format | Max Chars | Example |
|--------|--------|-----------|---------|
| Sleep Debt | `SD %+3d` | 6 | `SD -90` |
| Energy | `EN %3d` | 6 | `EN  87` |
| Comfort | `CF %3d` | 6 | `CF  72` |
| Workload | `WK %3d` | 6 | `WK  45` |
| Emotional | `EM %3d` | 6 | `EM  35` |

## Build Verification ✅

```bash
cd ~/repos/second-movement
make BOARD=sensorwatch_red DISPLAY=classic
```

**Result:** Build completed successfully!
- All zone faces compiled without errors
- Firmware size: 132,940 bytes (text) + 2,796 bytes (data)
- Output: build/firmware.uf2 (271,872 bytes)

## Key Changes Summary

1. ✅ Zone abbreviations use F91W-compatible characters
2. ✅ All metrics fit 6-char bottom line limit
3. ✅ Sleep Debt uses signed format (`%+3d`) for ±90 range
4. ✅ All metrics use proper spacing (2 spaces after abbreviation)
5. ✅ ALARM button cycles through 3 metrics per zone
6. ✅ Descent zone now has 3 metrics (was 2)
7. ✅ Momentum zone shows Emotional instead of Temperature

## Next Steps

The zone faces are now ready for testing on classic F91W hardware. All display constraints are met and the code compiles successfully.
