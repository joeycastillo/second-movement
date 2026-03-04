# Zone Face Word Display System - Implementation Summary

## Overview
Implemented chronomantic word display system for 4 zone faces in PR #70, adding dynamic word selection based on current metrics with streak tracking.

## Files Created

### Core Word System
1. **lib/phase/zone_words.h** (2.8 KB)
   - Header file defining zone identifiers, word arrays, and helper functions
   - 4 zones × 48 words each (192 words total)
   - 96 shared wildcard words
   - Word selection algorithm declarations

2. **lib/phase/zone_words.c** (7.6 KB)
   - Implementation of all word arrays
   - Word selection algorithm with cross-zone and wildcard mixing
   - PRNG for deterministic random selection
   - Total flash impact: ~1.8 KB (well within PR #70 budget)

## Files Modified

### Zone Faces (4 files updated)
Each zone face received identical structural updates:

1. **watch-faces/complication/emergence_face.h/c**
   - Zone: E (Emergence/Balance)
   - Metric: EM (Emotional)
   - Word arrays: ZONE_EM

2. **watch-faces/complication/momentum_face.h/c**
   - Zone: W (Work/Activity) 
   - Metric: WK (Wake Momentum)
   - Word arrays: ZONE_WK

3. **watch-faces/complication/active_face.h/c**
   - Zone: W (Work/Activity)
   - Metric: WK (Activity)
   - Word arrays: ZONE_WK

4. **watch-faces/complication/descent_face.h/c**
   - Zone: R (Recovery)
   - Metric: Comfort
   - Word arrays: ZONE_RS

### State Structure Updates
Added to each face's state struct:
```c
uint8_t display_mode;           // 0=word1, 1=word2, 2=stats
char selected_words[2][9];      // Two selected words (8 chars + null)
uint8_t streak_days;            // Consecutive days viewed
uint8_t last_check_day;         // Day of year for streak tracking
```

### Display Flow Implementation
Each zone face now supports 3 display modes (ALARM button cycling):

**Mode 0 (Word 1):**
```
┌─────────────┐
│ EM          │  ← Zone indicator
│   SPARK     │  ← First selected word
└─────────────┘
```

**Mode 1 (Word 2):**
```
┌─────────────┐
│ EM          │
│   BLOOM     │  ← Second selected word
└─────────────┘
```

**Mode 2 (Stats):**
```
┌─────────────┐
│ EM          │
│ E Lv2  3d   │  ← Zone letter + Level + Streak
└─────────────┘
```

### Build System
- **Makefile**: Added `./lib/phase/zone_words.c` to build

## Word Selection Algorithm

Implemented in `zone_words_select()`:

1. Pick 2 random words from current level's 12-word array
2. ~20% chance: replace one word with word from different zone
3. ~10% chance: replace one word with wildcard

Uses deterministic PRNG seeded from RTC timestamp for variety while maintaining reproducibility.

## Word Arrays

### EM (Emergence/Balance) - 48 words
- **Level 0 (0-25):** SLEEP, REST, HUSH, STILL, VOID, DARK, NULL, FADE, EMPTY, QUIET, NUMB, LOST
- **Level 1 (26-50):** ALIGN, TUNE, PULSE, MESH, WEAVE, LINK, TIDE, BREATH, SWAY, ORBIT, DIAL, TETHER
- **Level 2 (51-75):** SURGE, GLIDE, SPARK, POUR, BLOOM, WIND, WAVE, RUSH, SOAR, DANCE, MELT, BURN
- **Level 3 (76-100):** PEAK, BLAZE, APEX, CROWN, FULL, NOON, ZENITH, CORE, FORGE, TOTAL, FUSE, MERGE

### WK (Activity) - 48 words
- **Level 0 (0-25):** IDLE, PAUSE, WAIT, STILL, SETTLE, STOP, DRIFT, HUSH, STALL, HOLD, SLACK, REPOSE
- **Level 1 (26-50):** WALK, SHIFT, SCAN, TEND, STEP, REACH, LEAN, PROWL, ROAM, CREEP, PACE, CREST
- **Level 2 (51-75):** BUILD, CRAFT, DIG, PRESS, PUSH, CLIMB, GRIND, HAUL, SHAPE, FORGE, HEAVE, DRIVE
- **Level 3 (76-100):** SPRINT, BLAZE, SURGE, HUNT, FIGHT, CHARGE, SMASH, STRIKE, BURST, LAUNCH, CLASH, WRENCH

### RS (Recovery) - 48 words
- **Level 0 (0-25):** ALERT, WIRED, JOLT, BUZZ, SHOCK, SPIKE, FLASH, SNAP, STIR, CRACK, SURGE, SPARK
- **Level 1 (26-50):** EASE, SOFT, SMOOTH, STILL, LEVEL, STEADY, COOL, SETTLE, MILD, STABLE, SAFE, CLEAR
- **Level 2 (51-75):** SINK, DREAM, FLOAT, DRIFT, FALL, DIVE, MELT, FADE, DISSOLVE, PEACE, HUSH, BLANK
- **Level 3 (76-100):** STONE, VOID, NUMB, BLACK, DEAD, GONE, NULL, ZERO, HOLLOW, LOST, DARK, FROZEN

### CM (Circadian) - 48 words
- **Level 0 (0-25):** DRIFT, LOST, OFF, SPLIT, BROKE, FRACTURE, TWISTED, SKEWED, BACKWARD, INVERTED, REVERSED, FLIPPED
- **Level 1 (26-50):** DIAL, TUNE, SHIFT, ADJUST, CALIBRAT, SYNC, MATCH, SETTLE, CONVERGE, APPROACH, NEAR, CLOSE
- **Level 2 (51-75):** FOLLOW, PULSE, RHYTHM, CYCLE, BEAT, PATTERN, WAVE, TIDE, ORBIT, CIRCLE, LOOP, FLOW
- **Level 3 (76-100):** PEAK, ZENITH, NOON, MIDNIGHT, LOCKED, FIXED, TOTAL, PERFECT, ABSOLUTE, COMPLETE, FULL, PURE

### Wildcard Pool - 96 words
Shared across all zones, includes:
- Spectral: GHOST, ECHO, SHADE, WRAITH, SPECTR, PHANTOM, ETHER, VEIL
- Material: BONE, ASH, MOSS, RUST, DUST, STONE, THORN, ROOT, SHELL, SHARD, CRYSTAL, GRAVEL
- Structure: COIL, SPIRAL, HELIX, KNOT, THREAD, WIRE, MESH, GRID, LATTICE, WEB, NET, WEAVE
- Atmospheric: SPARK, EMBER, FLAME, SMOKE, VAPOR, MIST, FOG, DEW, FROST, ICE, SNOW, STORM, WIND, GALE, RAIN
- Temporal: CYCLE, EPOCH, ERA, AGE, MOMENT, INSTANT, PULSE, TICK, TURN, ROUND
- Mystical: RUNE, GLYPH, SIGIL, MARK, SIGN, SYMBOL, TOKEN, CIPHER, CODE, AETHER, WYRD, KAIROS, ANIMA, OMEN, ROTA
- Cosmic: STAR, MOON, SUN, ORBIT, APOGEE, PERIGEE, ECLIPSE, SOLSTICE
- And more...

## Streak Tracking

Each zone face maintains independent streak tracking:
- Increments on first view each day (EVENT_ACTIVATE)
- Resets to 1 if day skipped
- Persists in face state structure
- Displays in stats mode (Mode 2)

## Flash Impact

**Total flash usage:** ~1.8 KB
- 192 zone-specific words (4 zones × 48 words × 6 bytes avg)
- 96 wildcard words (6 bytes avg)
- Code overhead (~200 bytes)

Well within PR #70 budget constraints.

## Build Verification

✅ Build successful (2024-02-22)
```
text     data      bss      dec      hex    filename
136148   2804     5184   144136    23308   build/firmware.elf
```

All 4 zone faces compiled successfully:
- emergence_face.o
- momentum_face.o
- active_face.o
- descent_face.o

## Testing Checklist

- [ ] Flash firmware to watch
- [ ] Verify emergence_face displays EM words (SPARK, BLOOM, etc.)
- [ ] Verify momentum_face displays WK words (BUILD, PUSH, etc.)
- [ ] Verify active_face displays WK words (SPRINT, CHARGE, etc.)
- [ ] Verify descent_face displays RS words (FLOAT, DREAM, etc.)
- [ ] Test ALARM button cycling (word1 → word2 → stats → word1)
- [ ] Verify streak tracking increments daily
- [ ] Verify stats display shows correct format ("E Lv2 3d")
- [ ] Verify wildcard words appear occasionally (~10%)
- [ ] Verify cross-zone words appear occasionally (~20%)

## Implementation Notes

1. **All code wrapped in `#ifdef PHASE_ENGINE_ENABLED`** - ensures clean builds for non-phase builds
2. **All words ≤8 characters** - fits 7-segment LCD constraints
3. **Deterministic PRNG** - words change but remain reproducible from timestamp
4. **Backward compatible** - legacy view_index kept in state structs (unused)
5. **Zone mapping:**
   - Emergence → ZONE_EM (uses EM metric)
   - Momentum → ZONE_WK (uses WK metric)
   - Active → ZONE_WK (uses WK metric)
   - Descent → ZONE_RS (uses Comfort metric)

## Future Enhancements

- [ ] Add Circadian zone face (currently CM word arrays unused)
- [ ] Persistent word history (show last N words)
- [ ] Configurable cross-zone/wildcard probability
- [ ] Word favorite/bookmark system
- [ ] Export word journal via USB

---

**Status:** ✅ Implementation complete, build verified
**Branch:** phase4e-sleep-tracking-fresh
**PR:** #70
**Date:** 2024-02-22
