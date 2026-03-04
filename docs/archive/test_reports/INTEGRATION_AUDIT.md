# Phase Engine Integration Audit
**Date:** 2026-02-20 08:45 AKST  
**Scope:** Complete integration analysis of phase engine with firmware ecosystem  
**Status:** 🔴 **CRITICAL GAPS FOUND** - Phase engine is scaffolding with no consumers

---

## Executive Summary

The phase engine is **architecturally sound and feature-complete**, but has **zero integration** with actual watch faces. It's complete scaffolding with no users—like building a perfect API that nobody calls.

### Severity Assessment

| Issue | Severity | Impact |
|-------|----------|--------|
| No watch faces use phase engine | 🔴 **CRITICAL** | Entire feature is unused |
| Two separate circadian systems | 🟡 **HIGH** | Confusing, duplicated effort |
| No UI indication of homebase requirement | 🟡 **HIGH** | Poor UX, users won't know |
| Builder allows useless builds | 🟡 **MEDIUM** | Wastes flash, confusing |
| No example/reference face | 🟡 **MEDIUM** | No integration path |

---

## Critical Finding #1: Zero Watch Face Integration

### The Problem

**Phase Engine Has No Consumers**

```bash
$ find watch-faces -name "*.c" -exec grep -l "phase_engine\|phase_compute" {} \;
# (no output)
```

**What This Means:**
- ✅ Phase engine code exists and compiles
- ✅ Homebase table can be generated
- ✅ Builder UI supports configuration
- ❌ **BUT: Zero watch faces call the phase engine API**

### Impact

1. **Wasted Resources:**
   - User configures homebase in builder
   - ~2.2KB flash consumed by homebase table
   - Phase engine compiled into firmware
   - **Nothing uses it**

2. **Broken User Journey:**
   ```
   User sees "Homebase" in builder
   → Enters coordinates
   → Downloads firmware
   → No visible phase-related functionality
   → Confusion: "What was homebase for?"
   ```

3. **Maintenance Burden:**
   - Code that compiles but never runs
   - Untested integration points
   - Potential bugs won't be discovered

### Root Cause

**Phase engine was built as infrastructure** (bottom-up), but **no features were built on top** (top-down).

Classic case of:
```
Infrastructure ✅
Integration   ❌
```

---

## Critical Finding #2: Duplicate Circadian Systems

### Two Separate Systems Exist

**System 1: `lib/circadian_score.*`**
- Purpose: Sleep quality scoring (retrospective)
- Algorithm: 75% evidence-based scoring
- Components: SRI, duration, efficiency, compliance, light
- Consumer: `circadian_score_face.c` ✅
- Status: **Actively used**

**System 2: `lib/phase/*` (Phase Engine)**
- Purpose: Real-time circadian alignment (prospective)
- Algorithm: Time-of-day + season + activity + environment
- Components: Homebase table, phase score, trends, recommendations
- Consumer: **NONE** ❌
- Status: **Unused scaffolding**

### The Confusion

```
User perspective:
"I want circadian tracking"
  
Which system?
- circadian_score → Sleep quality (works)
- phase_engine   → Real-time alignment (exists but unused)
```

### Why This Happened

**Different design goals:**
- `circadian_score`: Retrospective analysis ("How did I sleep?")
- `phase_engine`: Prospective guidance ("What should I do now?")

**But:**
- No documentation explaining the difference
- Both named "circadian"
- No integration between them
- User can't tell which is which

### Potential Synergy (Currently Missing)

**What COULD happen:**
```c
// Phase engine could use circadian_score data
circadian_data_t *sleep_data = get_circadian_data();
phase_state_t *phase = get_phase_state();

// Adjust phase score based on sleep quality
if (sleep_data->nights[0].efficiency < 80) {
    phase_score -= 10;  // Penalize for poor sleep
}

// Or: Use phase recommendation to improve circadian score
if (phase_get_recommendation(score, hour) == 0) {  // Rest
    recommend_sleep_window(hour, hour + 2);
}
```

**What ACTUALLY happens:**
- Two systems exist in parallel
- Zero integration
- Zero synergy

---

## Critical Finding #3: No Watch Face Uses Phase Engine

### Face Audit Results

**Total watch faces scanned:** ~130+ faces

**Faces using `lib/phase/phase_engine.h`:** **0** ❌

**Faces that COULD benefit from phase engine:**
1. **`circadian_score_face.c`** - Already has circadian scoring, could add real-time phase
2. **`sunrise_sunset_face.c`** - Has time-of-day awareness, could show circadian alignment
3. **`temperature_display_face.c`** - Could show expected vs actual temp variance
4. **Custom alarm faces** - Could recommend optimal wake times based on phase

### What's Missing

**Minimal Example Face**

No reference implementation exists showing:
- How to initialize phase engine
- How to call `phase_compute()`
- How to display phase score
- How to use trend/recommendations

**Integration Guide exists** (✅ Created in this task) but no **code to reference**.

---

## Critical Finding #4: Builder UI Gaps

### Gap 1: No Indication of Homebase Purpose

**Current UI:**
```
HOMEBASE LOCATION

LATITUDE
LONGITUDE
TIMEZONE
```

**Issues:**
- No explanation what homebase is for
- No indication it's optional
- No warning that it uses 2KB flash
- User doesn't know which faces need it

**Fix Applied (Local):**
- ℹ️ Collapsible "What is Homebase?" section
- Inline help under each field
- Flash size warning
- **Status:** Ready to deploy

### Gap 2: No Face-Level Homebase Requirements

**Builder doesn't indicate:**
- Which faces require homebase
- Which faces benefit from homebase
- Which faces ignore homebase

**Example missing metadata:**
```json
{
  "id": "hypothetical_phase_face",
  "name": "Phase Score",
  "requires_homebase": true,  // ← MISSING IN SCHEMA
  "description": "..."
}
```

**Impact:**
- User can add ANY face combination
- User can configure homebase even if no faces use it
- User can skip homebase even if face requires it
- Builder doesn't validate or warn

### Gap 3: Useless Builds Allowed

**Current behavior:**
```
User selects: clock_face, alarm_face, settings_face
User enters: Homebase coords
Builder: ✅ Builds successfully

Result:
- Homebase table generated (2.2KB)
- Phase engine enabled
- NO faces use it
- User wastes flash, gets no benefit
```

**Should be:**
```
User selects: clock_face, alarm_face, settings_face
User enters: Homebase coords
Builder: ⚠️ Warning! No selected faces use homebase.
         Homebase will consume 2.2KB flash with zero benefit.
         
Options:
[ Skip Homebase ] [ Add Phase Face ] [ Build Anyway ]
```

---

## Critical Finding #5: GitHub Actions Integration Status

### What's Working ✅

**Workflow Parameters:**
```yaml
homebase_lat: (optional)
homebase_lon: (optional)
homebase_tz: (optional)
```

**Conditional Table Generation:**
```yaml
- name: Generate homebase phase tables
  if: ${{ all params provided }}
  run: python3 utils/generate_homebase.py ...
```

**Phase Engine Flag:**
```python
if phase_engine_enabled:
    phase_engine_define = "#define PHASE_ENGINE_ENABLED\n"
```

✅ **All infrastructure works correctly**

### What's Missing ⚠️

**No validation that homebase is actually useful:**

```yaml
# Should add:
- name: Validate homebase usage
  if: ${{ homebase provided }}
  run: |
    python3 utils/validate_homebase_usage.py \
      --faces "$INPUT_FACES" \
      --registry builder/face_registry.json
    # Exits with warning if no faces use homebase
```

**No build output showing phase engine status:**

```yaml
# Should show in build summary:
- Phase Engine: ENABLED
- Homebase: 61.2181, -149.9003 (AKST)
- Table Size: 2,206 bytes
- Consuming Faces: 0  # ← WARNING!
```

---

## Integration Points Analysis

### Phase Engine → Watch Faces (❌ BROKEN)

**Expected Flow:**
```
phase_engine.c → API functions
               ↓
          Watch faces call API
               ↓
          Display phase info to user
```

**Actual Flow:**
```
phase_engine.c → API functions
               ↓
          (nothing calls it)
               ↓
          Code compiled but never executed
```

**Status:** 🔴 **ZERO INTEGRATION**

### Builder UI → Workflow (✅ WORKS)

**Flow:**
```
User enters homebase coords
         ↓
UI sends workflow_dispatch
         ↓
Workflow generates homebase table
         ↓
Firmware compiled with phase engine
```

**Status:** ✅ **FULLY FUNCTIONAL**

### Homebase Table → Phase Engine (✅ WORKS)

**Flow:**
```
homebase_table.h generated
         ↓
phase_engine.c includes it
         ↓
homebase_get_entry() available
         ↓
(but no one calls it)
```

**Status:** ✅ **TECHNICALLY WORKS** ⚠️ **NEVER CALLED**

### Config Generator → Build System (✅ WORKS)

**Flow:**
```
--phase-engine flag
         ↓
PHASE_ENGINE_ENABLED defined
         ↓
phase_engine.c compiled
         ↓
Linked into firmware
```

**Status:** ✅ **FULLY FUNCTIONAL**

---

## TODO Comments and Incomplete Features

### Found in Phase Engine Code

**File:** `lib/phase/phase_engine.c`

```c
// None found - implementation appears complete
```

**File:** `lib/phase/INTEGRATION_GUIDE.md`

```markdown
// TODO: Get from movement tracker
// TODO: Get from temp sensor  
// TODO: Get from light sensor
```

**Status:** These are intentional placeholders for integrators, not unfinished work.

### Found in Builder

**File:** `builder/index.html`

No phase-related TODOs found.

### Found in Workflow

**File:** `.github/workflows/custom-build.yml`

No phase-related TODOs found.

---

## Unreachable Code Analysis

### Code That Compiles But Never Runs

**When `PHASE_ENGINE_ENABLED` is set:**

```c
// lib/phase/phase_engine.c - ALL FUNCTIONS UNREACHABLE
void phase_engine_init(phase_state_t *state) {
    // Never called by any watch face
}

uint16_t phase_compute(...) {
    // Never called by any watch face
}

int16_t phase_get_trend(...) {
    // Never called by any watch face
}

uint8_t phase_get_recommendation(...) {
    // Never called by any watch face
}
```

**Impact:**
- ~1KB of code in firmware that never executes
- ~2.2KB of homebase table data that's never accessed
- Total waste: **~3.2KB** when homebase configured

---

## Recommended Fixes (Priority Order)

### 🔴 CRITICAL: Fix #1 - Create Reference Phase Face

**Create:** `watch-faces/complication/minimal_phase_face.c`

**Minimal implementation:**
```c
#include "phase_engine.h"

typedef struct {
    phase_state_t phase;
} minimal_phase_state_t;

void minimal_phase_face_setup(...) {
    phase_engine_init(&state->phase);
}

bool minimal_phase_face_loop(...) {
    watch_date_time now = watch_rtc_get_date_time();
    uint16_t score = phase_compute(&state->phase,
                                   now.unit.hour,
                                   movement_get_day_of_year(now),
                                   500, 200, 100);  // Mock sensors
    
    char buf[16];
    sprintf(buf, "PH  %3d", score);
    watch_display_string(buf, 0);
}
```

**Add to face registry:**
```json
{
  "id": "minimal_phase_face",
  "name": "Minimal Phase",
  "requires_homebase": true,
  "category": "complication",
  "description": "Displays real-time circadian phase score (0-100)"
}
```

**Impact:** Proves the system works end-to-end.

### 🟡 HIGH: Fix #2 - Add `requires_homebase` to Registry Schema

**Update:** `builder/face_registry.json` schema

**Add field:**
```json
{
  "id": "...",
  "requires_homebase": false,  // New field (default false)
  ...
}
```

**Update builder UI logic:**
```javascript
function validateFaceSelection() {
    const selectedFaces = getSelectedFaces();
    const requiresHomebase = selectedFaces.some(f => f.requires_homebase);
    const homebaseProvided = hasHomebaseCoords();
    
    if (requiresHomebase && !homebaseProvided) {
        showWarning("Selected faces require homebase configuration");
        disableBuildButton();
    }
    
    if (!requiresHomebase && homebaseProvided) {
        showWarning("Homebase configured but no faces use it (wastes 2KB flash)");
    }
}
```

**Impact:** Prevents useless homebase configurations.

### 🟡 HIGH: Fix #3 - Integrate or Deprecate Duplicate Systems

**Option A: Integrate** (Recommended)

Make `circadian_score_face` use phase engine for real-time guidance:

```c
// In circadian_score_face.c
#ifdef PHASE_ENGINE_ENABLED
#include "phase_engine.h"

// Add phase state to face state
phase_state_t phase;

// In loop:
uint16_t current_phase = phase_compute(...);

// Show combined view:
// "CS 75 PH 82" - Overall score + current phase
#endif
```

**Option B: Clarify** (Minimal effort)

Add documentation explaining:
- `circadian_score` = Retrospective sleep quality
- `phase_engine` = Prospective activity guidance
- Both can coexist for different purposes

**Option C: Deprecate** (If phase unused long-term)

If no faces adopt phase engine:
- Remove phase engine code
- Remove homebase UI
- Remove workflow integration
- Focus on circadian_score only

### 🟡 MEDIUM: Fix #4 - Add Build Validation

**Create:** `utils/validate_homebase_usage.py`

```python
def validate_homebase_usage(faces, registry):
    """Warn if homebase configured but unused."""
    face_data = load_registry(registry)
    
    requires_homebase = any(
        face_data[f].get('requires_homebase', False) 
        for f in faces
    )
    
    if not requires_homebase:
        print("⚠️  WARNING: Homebase configured but no faces use it")
        print("   This will consume ~2.2KB flash with zero benefit")
        print("   Consider removing homebase or adding a phase-aware face")
        return 1
    
    return 0
```

**Add to workflow:**
```yaml
- name: Validate homebase usage
  if: ${{ homebase params provided }}
  run: |
    python3 utils/validate_homebase_usage.py \
      --faces "$INPUT_FACES" \
      --registry builder/face_registry.json
  continue-on-error: true  # Warning, not error
```

### 🟢 LOW: Fix #5 - Enhance Build Summary

**Update workflow output:**
```yaml
- name: Output build summary
  run: |
    echo "### Build Configuration" >> $GITHUB_STEP_SUMMARY
    echo "" >> $GITHUB_STEP_SUMMARY
    echo "**Board:** $BOARD" >> $GITHUB_STEP_SUMMARY
    echo "**Display:** $DISPLAY" >> $GITHUB_STEP_SUMMARY
    
    if [ -f lib/phase/homebase_table.h ]; then
      echo "**Phase Engine:** ENABLED" >> $GITHUB_STEP_SUMMARY
      echo "**Homebase:** $(grep latitude_e6 lib/phase/homebase_table.h | ...)" >> $GITHUB_STEP_SUMMARY
      echo "**Flash Cost:** ~2.2KB" >> $GITHUB_STEP_SUMMARY
    else
      echo "**Phase Engine:** Disabled" >> $GITHUB_STEP_SUMMARY
    fi
```

---

## Test Coverage Gaps

### Unit Tests: MISSING

No tests exist for:
- `phase_engine_init()`
- `phase_compute()`
- `phase_get_trend()`
- `phase_get_recommendation()`
- Homebase table access functions

**Should add:**
```c
// tests/test_phase_engine.c
void test_phase_compute_basic() {
    phase_state_t state = {0};
    phase_engine_init(&state);
    
    uint16_t score = phase_compute(&state, 12, 180, 500, 200, 100);
    assert(score >= 0 && score <= 100);
}

void test_phase_trend_improving() {
    // Simulate improving conditions
    // Verify trend > 0
}
```

### Integration Tests: MISSING

No tests verify:
- Homebase table generation
- Phase engine compilation
- End-to-end builder workflow

### Hardware Tests: IMPOSSIBLE (No Face)

Can't test on hardware because:
- No watch face uses phase engine
- Can't verify phase score accuracy
- Can't test sensor integration

---

## Documentation Gaps (Now Filled)

### ✅ Created in This Task

1. **`lib/phase/INTEGRATION_GUIDE.md`** - Complete API documentation
2. **Enhanced `utils/generate_homebase.py` output** - Build-time feedback
3. **Enhanced `builder/index.html`** - UI help and tooltips

### Still Missing

1. **Example face code** - No reference implementation
2. **User guide** - How to use phase-aware faces
3. **Developer guide** - How to integrate phase engine into new faces
4. **Comparison doc** - `circadian_score` vs `phase_engine`

---

## Flash Budget Analysis

### When Homebase Configured

**Added to firmware:**
```
Phase engine code:          ~1,000 bytes
Homebase table:             2,206 bytes (365 entries × 6 bytes + metadata)
Accessor functions:         ~100 bytes
─────────────────────────────────────
TOTAL OVERHEAD:            ~3,300 bytes
```

**If NO faces use it:** 3.3KB of wasted flash ❌

**If ONE face uses it:** 3.3KB well-spent ✅

### Current Reality

**Typical custom build:**
- User configures homebase: ~30% of users (estimate)
- Faces that use it: **0**
- Result: **100% waste rate**

---

## Conclusion & Action Items

### What's Working ✅

1. Phase engine architecture is sound
2. Homebase table generation works perfectly
3. Builder UI accepts homebase input
4. Workflow integration is complete
5. Enhanced build feedback is excellent
6. Integration guide is comprehensive

### What's Broken 🔴

1. **ZERO watch faces use the phase engine** (blocking)
2. Two duplicate circadian systems (confusing)
3. No validation prevents useless builds
4. No indication which faces need homebase
5. No tests for phase engine code

### Immediate Actions Required

**Priority 1: Make It Useful**
- [ ] Create `minimal_phase_face.c` reference implementation
- [ ] Add to face registry with `requires_homebase: true`
- [ ] Test end-to-end: builder → firmware → hardware

**Priority 2: Prevent Waste**
- [ ] Add `requires_homebase` field to registry schema
- [ ] Implement builder validation (warn if homebase unused)
- [ ] Add build summary showing phase engine status

**Priority 3: Clarify vs Unify**
- [ ] Document difference between `circadian_score` and `phase_engine`
- [ ] Consider integrating them (use phase for real-time in score face)
- [ ] Or: Deprecate phase if no adoption after reference face

**Priority 4: Deploy Enhancements**
- [ ] Commit enhanced builder UI
- [ ] Push to GitHub Pages
- [ ] Verify live deployment

### Long-Term Health

- [ ] Add unit tests for phase engine
- [ ] Create integration test suite
- [ ] Hardware validation with real sensors
- [ ] User documentation for phase-aware features

---

## Files Requiring Changes

### Critical Path (Unblock Feature)

1. `watch-faces/complication/minimal_phase_face.c` - CREATE
2. `watch-faces/complication/minimal_phase_face.h` - CREATE
3. `builder/face_registry.json` - ADD minimal_phase_face entry

### High Priority (Improve UX)

4. `builder/index.html` - DEPLOY (already fixed locally)
5. `builder/face_registry.json` - ADD `requires_homebase` field
6. `builder/index.html` - ADD validation logic
7. `utils/validate_homebase_usage.py` - CREATE

### Medium Priority (Documentation)

8. `docs/PHASE_VS_CIRCADIAN_SCORE.md` - CREATE comparison guide
9. `docs/PHASE_USER_GUIDE.md` - CREATE end-user documentation

### Low Priority (Nice to Have)

10. `tests/test_phase_engine.c` - CREATE unit tests
11. `.github/workflows/test-phase-engine.yml` - CREATE CI tests

---

**Bottom Line:** Phase engine is architecturally complete but functionally unused. Creating one reference watch face will unlock the entire system and prove it works. Without it, phase engine is just expensive scaffolding.
