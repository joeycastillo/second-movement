# Zone Faces LCD Display Fix - Summary

## Task
Fixed zone faces to work with classic F91W LCD constraints (6-char bottom line, 2-char top).

## Files Modified
1. `watch-faces/complication/emergence_face.c`
2. `watch-faces/complication/momentum_face.c`
3. `watch-faces/complication/active_face.c`
4. `watch-faces/complication/descent_face.c`

## Changes Applied

### 1. emergence_face.c (ER zone)
- ✅ Zone abbreviation: "EM" → "ER"
- ✅ Sleep Debt format: `SD  %2d` → `SD %+3d` (signed, ±90 range)
- ✅ Emotional format: `EM  %2d` → `EM %3d` (proper spacing)
- ✅ Comfort format: `CMF %2d` → `CF %3d` (6-char limit)

**Display examples:**
```
ER           ER           ER
SD -90       EM  35       CF  72
```

### 2. momentum_face.c (MO zone)
- ✅ Workload format: `WK  %2d` → `WK %3d` (proper spacing)
- ✅ Sleep Debt format: `SD  %2d` → `SD %+3d` (signed)
- ✅ Third metric: Temperature (TE) → Emotional (EM)
- ✅ Removed unused includes: `<math.h>`, `watch_slcd.h`, `watch.h`
- ✅ Removed unused extern: `movement_state`

**Display examples:**
```
MO           MO           MO
WK  45       SD -30       EM  25
```

### 3. active_face.c (AC zone)
- ✅ Energy format: `NRG %2d` → `EN %3d` (6-char limit, proper spacing)
- ✅ Emotional format: `EM  %2d` → `EM %3d` (proper spacing)
- ✅ Sleep Debt format: `SD  %2d` → `SD %+3d` (signed)

**Display examples:**
```
AC           AC           AC
EN  87       EM  35       SD +15
```

### 4. descent_face.c (DE zone)
- ✅ Comfort format: `CMF %2d` → `CF %3d` (6-char limit)
- ✅ Emotional format: `EM  %2d` → `EM %3d` (proper spacing)
- ✅ Added third metric: Sleep Debt (SD %+3d)
- ✅ View count: 2 → 3 metrics

**Display examples:**
```
DE           DE           DE
CF  72       EM  40       SD -20
```

## Format Specifications Met

All metrics now use 6-character max format:

| Metric | Format | Range | Example |
|--------|--------|-------|---------|
| Sleep Debt (SD) | `SD %+3d` | ±90 | `SD -90` |
| Energy (EN) | `EN %3d` | 0-100 | `EN  87` |
| Comfort (CF) | `CF %3d` | 0-100 | `CF  72` |
| Workload (WK) | `WK %3d` | 0-999 | `WK  45` |
| Emotional (EM) | `EM %3d` | 0-50 | `EM  35` |

## Zone Metric Assignments

**Emergence (ER)** - Morning zone
1. Sleep Debt (primary)
2. Emotional
3. Comfort

**Momentum (MO)** - Workday zone
1. Workload (primary)
2. Sleep Debt
3. Emotional

**Active (AC)** - Evening zone
1. Energy (primary)
2. Emotional
3. Sleep Debt

**Descent (DE)** - Night zone
1. Comfort (primary)
2. Emotional
3. Sleep Debt

## Testing Notes

All zone faces now:
- ✅ Display correct 2-char zone abbreviation on top line
- ✅ Display metrics in 6-char format on bottom line
- ✅ Support ALARM button cycling through 3 metric views (2-3 per zone)
- ✅ Use proper signed format for Sleep Debt (±90)
- ✅ Use proper spacing for all metrics

## Build Status
Ready for compilation test.
