# Zone Configuration Persistence Test

## Test 1: URL Hash Persistence

### Steps:
1. Open `builder/index.html` in browser
2. Adjust zone boundaries:
   - Emergence: 20
   - Momentum: 45
   - Active: 70
3. Enable zone swap toggle (should show "ON")
4. Check URL hash in address bar - should contain:
   - `zem=20` (emergence max)
   - `zmm=45` (momentum max)
   - `zam=70` (active max)
   - `zswap=1` (swap enabled)
5. Copy the full URL
6. Open URL in new tab/window
7. Verify:
   - Zone boundaries restored correctly (20/45/70)
   - Zone swap toggle shows "ON"
   - Visual zone bar displays correct boundaries
   - Zone descriptions reflect swap state

### Expected URL format:
```
builder/index.html#board=red&display=classic&branch=main&...&zem=20&zmm=45&zam=70&zswap=1
```

## Test 2: Template Saving/Loading

### Steps:
1. Configure zones to custom values (e.g., 22/48/72)
2. Enable zone swap
3. Enter template name: "Morning Workout"
4. Click [save]
5. Reset zones to defaults (25/50/75) and disable swap
6. Open Templates panel
7. Find "Morning Workout" under "SAVED"
8. Click [load]
9. Verify:
   - Zones restored to 22/48/72
   - Zone swap enabled
   - Visual bar updates correctly

## Test 3: Default Values

### Steps:
1. Open `builder/index.html` (no hash params)
2. Verify default values:
   - Emergence: 25
   - Momentum: 50
   - Active: 75
   - Swap: OFF
3. Visual bar should show default layout

## Test 4: Preset Templates

### Steps:
1. Open Templates panel
2. Load "Early Riser" preset (or any preset with custom zones)
3. Verify:
   - Zone boundaries match preset
   - Swap state matches preset
   - URL hash updates with zone params

## Test 5: State Serialization

### Manual JavaScript Console Test:
```javascript
// In browser console:
// 1. Get current state
console.log('Current zone config:', {
  zoneEmergenceMax: state.zoneEmergenceMax,
  zoneMomentumMax: state.zoneMomentumMax,
  zoneActiveMax: state.zoneActiveMax,
  zoneSwapEnabled: state.zoneSwapEnabled
});

// 2. Modify values
state.zoneEmergenceMax = 18;
state.zoneMomentumMax = 42;
state.zoneActiveMax = 68;
state.zoneSwapEnabled = true;

// 3. Update UI
updateZoneBar();
updateHash();

// 4. Check URL has new values
console.log('URL hash:', window.location.hash);

// 5. Reload page and verify values persist
location.reload();
```

## Success Criteria

- ✅ Changing zones updates URL hash immediately
- ✅ URL with zone params loads with correct zones
- ✅ Custom templates include zone config
- ✅ Preset templates continue to work
- ✅ Zone swap toggle persists in URL/templates
- ✅ Default values work when no hash params present
- ✅ Zone visual bar updates in real-time
- ✅ Zone descriptions reflect swap state

## Code Changes Summary

### Fixed:
- Added `zoneSwapEnabled` loading in `loadTemplate()` function
  - Previously: UI was updated but state wasn't loaded from template
  - Now: `state.zoneSwapEnabled = !!tmplState.zoneSwapEnabled;`

### Already Working:
- Zone boundaries in state (zoneEmergenceMax, zoneMomentumMax, zoneActiveMax)
- URL hash serialization (updateHash)
- URL hash deserialization (parseHash)
- Template saving (automatically includes all state)
- Zone UI updates (updateZoneBar, updateZonePreview, updateZoneDescriptions)
- Zone swap toggle event handler
