# Phase 1 Testing Checklist

**Branch:** `feature/hybrid-3d-builder`  
**Goal:** Verify modularized architecture preserves ALL existing functionality

## Pre-Testing Setup

- [ ] Ensure local development server running (e.g., `python3 -m http.server 8000`)
- [ ] Open builder in browser: `http://localhost:8000/builder/index.html`
- [ ] Open browser console (F12) to watch for errors

## Critical Functionality Tests

### 1. Page Load & Initialization
- [ ] Page loads without JavaScript errors
- [ ] Console shows: `✅ Builder initialized successfully`
- [ ] Console shows: `✅ Three.js scene initialized (placeholder mode)`
- [ ] Black canvas visible in background (retro-canvas)
- [ ] UI elements render correctly (no missing content)

### 2. Hash URL Parsing
- [ ] Load page with hash URL (test template)
- [ ] Verify faces loaded from hash
- [ ] Verify settings loaded from hash (LED colors, toggles, etc.)
- [ ] Try example hash: `#board=pro&display=classic&faces=clock_face,wyoscan_face&secondary=2&24h=true`

### 3. Face Selection
- [ ] Available faces list renders
- [ ] Drag face from available → active list (Sortable.js)
- [ ] Drag to reorder faces in active list
- [ ] Remove face from active list (click remove button)
- [ ] Verify `__secondary__` divider stays in place
- [ ] Flash usage counter updates when faces change

### 4. Templates
- [ ] Preset templates visible in Templates panel
- [ ] Click [load] on preset template → loads configuration
- [ ] Save custom template (enter name, click save)
- [ ] Custom template appears in "Your Saved Templates"
- [ ] Load custom template → restores configuration
- [ ] Delete custom template → removes from list

### 5. Configuration Controls
- [ ] LED color sliders update preview in real-time
- [ ] Toggle 24h clock on/off
- [ ] Toggle button sound on/off
- [ ] Toggle smooth LED fade → shows/hides animation preview
- [ ] Active hours enabled toggle → shows/hides time pickers
- [ ] Zone configuration sliders update preview indicator

### 6. Alarm Tune Previews
- [ ] Select different alarm tunes from dropdowns
- [ ] Click [preview] button next to each tune
- [ ] Audio plays (Web Audio API)
- [ ] Mobile: Click "Unlock Audio" first (if on mobile device)

### 7. GitHub Token & Build
**⚠️ Requires valid GitHub Personal Access Token with `workflow` scope**
- [ ] Enter GitHub PAT in token field
- [ ] Token validation shows green "✓ TOKEN OK" or red "✗ INVALID TOKEN"
- [ ] Click [Build Firmware] button
- [ ] Build queued message appears
- [ ] Cooldown bar animates (90 seconds)
- [ ] (Optional) Wait for build completion (2-5 minutes)
- [ ] Download button activates when build succeeds

### 8. Hash URL Updates
- [ ] Change any setting (face, LED color, toggle)
- [ ] Verify browser URL hash updates automatically
- [ ] Copy URL → paste in new tab → settings restored

### 9. Copy Link
- [ ] Click [copy link] button
- [ ] Verify button text changes to "[copied!]"
- [ ] Paste clipboard → full URL with hash present

### 10. Three.js Scene
- [ ] Retro canvas visible (black background)
- [ ] No console errors from Three.js
- [ ] Window resize → canvas resizes correctly
- [ ] Check console: device tier detected (LOW/MEDIUM/HIGH)

## Regression Testing

### Things That Should NOT Change
- [ ] Visual appearance identical to pre-refactor (except black canvas)
- [ ] All buttons clickable
- [ ] All panels expand/collapse correctly
- [ ] Mobile layout unchanged (responsive)
- [ ] CRT quality toggle works (low/medium/high/auto)

### Known Limitations (Phase 1)
- [ ] Three.js scene renders NOTHING (intentional - just black placeholder)
- [ ] No 3D face gallery yet
- [ ] No split-pane layout yet (single column)

## Browser Compatibility

Test on multiple browsers (if possible):
- [ ] Chrome/Edge (Chromium)
- [ ] Firefox
- [ ] Safari (macOS/iOS)

## Performance Check

- [ ] Page load time < 2 seconds (local)
- [ ] No memory leaks (check DevTools Memory tab)
- [ ] No excessive console warnings

## Grep Audit (Code Review)

Run these checks in terminal:

```bash
cd ~/repos/second-movement

# Check for broken inline references (should return nothing)
grep -r "function \w\+(" builder/index.html

# Verify no duplicate script tags
grep -c "<script" builder/index.html  # Should be 3: importmap, Sortable.js, app.js

# Check module imports are correct
grep "import.*from" builder/js/*.js

# Verify no undefined globals
grep -E "const|let|var" builder/index.html | grep -v "CSS variables"
```

## Issue Tracking

Document any failures below:

### Bugs Found
- [ ] None (or list here)

### Console Errors
- [ ] None (or paste here)

### Visual Differences
- [ ] None (or describe here)

## Sign-Off

- **Tested by:** _________________
- **Date:** _________________
- **Build hash:** _________________
- **Pass/Fail:** _________________
- **Notes:**

---

## Next Steps (Phase 2)

Once Phase 1 passes all tests:
- [ ] Merge `feature/hybrid-3d-builder` → `phase4bc-playlist-dispatch`
- [ ] Start Phase 2: Split-pane layout + active 3D rendering
- [ ] Update HYBRID_3D_BUILDER_PLAN.md with learnings
