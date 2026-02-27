# Phase 3B PR 3: Weighted Playlist Controller

## Summary

Implements the weighted playlist system for zone-based face rotation. Maps phase score (0-100) to circadian zones and computes weighted relevance for each metric to dynamically determine which faces to display.

## Changes

### New Files

- `lib/phase/playlist.h` - Public API for playlist controller
- `lib/phase/playlist.c` - Implementation of zone determination, relevance calculation, and face rotation logic
- `test_playlist.sh` - Test suite for playlist controller

### Modified Files

- `Makefile` - Add `playlist.c` to build sources
- `movement.c` - Add playlist.h stub include (full integration in PR 4)

## Implementation Details

### Zone Determination

Maps phase score to four zones with fixed thresholds:
- **Emergence** (0-25): Waking, orienting
- **Momentum** (26-50): Building energy
- **Active** (51-75): Peak output
- **Descent** (76-100): Winding down

### Weight Tables

Per-zone weights for 5 metrics (SD, EM, WK, Energy, Comfort):

```c
// Weights stored in flash as const
static const uint8_t zone_weights[4][5] = {
    {30, 25,  5, 10, 30},  // EMERGENCE: SD + Comfort priority
    {20, 20, 30, 10, 20},  // MOMENTUM: WK is key
    {15, 20,  5, 40, 20},  // ACTIVE: Energy dominates
    {10, 35,  0, 10, 45},  // DESCENT: EM + Comfort for wind-down
};
```

### Relevance Calculation

Metrics near 50 (neutral) have low relevance. Metrics at extremes (0 or 100) surface strongly:

```c
relevance = weight * abs(metric - 50) / 50
```

### Face Rotation Logic

- Computes relevance for each metric based on current zone weights
- Sorts metrics by relevance (descending) using bubble sort
- Excludes metrics with relevance < 10
- Manages auto-advance after 30 ticks (configurable)
- Supports manual cycling via `playlist_advance()` (ALARM button)

### Hysteresis

Zone changes require 3 consecutive readings in the new zone to prevent flickering at boundaries.

## Testing

**Compilation Test:** ✅ Builds successfully with `PHASE_ENGINE_ENABLED=1`

**Manual Tests Required:**
- Zone determination logic
- Hysteresis behavior at zone boundaries
- Relevance calculation for various metric values
- Face rotation ordering
- Auto-advance timing

Run `./test_playlist.sh` to verify compilation and see test scenarios.

## Flash/RAM Impact

- **Flash:** ~1.5 KB (zone logic + weight tables in const flash)
- **RAM:** ~16 bytes (playlist_state_t)
- **Total build size:** 134,700 bytes text (within budget)

## Dependencies

- Phase 3B PR 2 (Remaining Metrics) - requires `metrics_snapshot_t`
- Phase Engine (Phase 2) - requires phase score computation

## Integration Status

**Current PR:** Playlist controller compiles and links successfully.

**Deferred to PR 4:** Full face dispatch integration in movement.c tick handler. This PR only includes a stub include to verify compilation.

## Reference

- `PHASE3_IMPLEMENTATION_PLAN.md` Section 2 (Weighted Playlist Controller)
- `PHASE3_IMPLEMENTATION_PLAN.md` Section 5 PR 3
- Phase Watch Overhaul Plan PDF (zone logic, weight tables)

## Checklist

- [x] Code compiles without warnings
- [x] Builds with `PHASE_ENGINE_ENABLED=1`
- [x] Builds without `PHASE_ENGINE_ENABLED` (no impact)
- [x] Flash/RAM within budget
- [x] Clean git history with descriptive commits
- [x] Test script created
- [ ] Manual runtime testing (requires PR 4 integration)
