# Security Review: PR #63 - Zone Configuration UI

**Reviewer:** Security Specialist Agent  
**Date:** 2026-02-20  
**Branch:** phase3-pr5-builder-ui  
**PR:** https://github.com/dlorp/second-movement/pull/63  
**Scope:** Zone Configuration UI (builder/index.html)

---

## Executive Summary

**CRITICAL SECURITY ISSUES FOUND** - PR #63 requires fixes before approval.

The zone configuration UI introduced in PR #63 has **REGRESSED** from a previously committed security fix (ddff5dd). The current HEAD has removed critical input validation that was added in commit ddff5dd, reintroducing multiple security vulnerabilities.

**Status:** ❌ **SECURITY APPROVAL DENIED** - Must fix critical issues before merge

---

## Critical Issues (MUST FIX)

### 1. ❌ Missing Input Validation in Zone Slider Event Handlers
**Severity:** CRITICAL  
**Location:** `builder/index.html` lines ~2770-2790  
**Status:** REGRESSION (fix was in ddff5dd but removed)

**Issue:**
```javascript
// CURRENT (VULNERABLE):
document.getElementById('zone-emergence-max').addEventListener('input', e => {
    const val = parseInt(e.target.value, 10);  // No validation!
    state.zoneEmergenceMax = val;
    document.getElementById('zone-emergence-val').textContent = val;
    updateZonePreview();
    updateHash();
});
```

**Attack Vector:**
- User can manipulate DOM/dev tools to send out-of-range values
- `parseInt()` can return `NaN` for invalid input
- No bounds checking allows values outside safe ranges (15-35, 40-60, 65-85)
- Invalid values propagate to state → URL hash → build inputs

**Fix (from ddff5dd):**
```javascript
// CORRECT (with validation):
document.getElementById('zone-emergence-max').addEventListener('input', e => {
    let val = parseInt(e.target.value, 10);
    if (isNaN(val) || val < 15 || val > 35) val = 25;  // Clamp to safe range
    state.zoneEmergenceMax = val;
    document.getElementById('zone-emergence-val').textContent = val;
    updateZonePreview();
    updateHash();
});
```

**Required Action:**
- ✅ Restore validation for `zone-emergence-max` (clamp to 15-35)
- ✅ Restore validation for `zone-momentum-max` (clamp to 40-60)
- ✅ Restore validation for `zone-active-max` (clamp to 65-85)

---

### 2. ❌ Missing Validation in Zone Preview Score Input
**Severity:** CRITICAL  
**Location:** `builder/index.html` line ~2800  
**Status:** REGRESSION (fix was in ddff5dd but removed)

**Issue:**
```javascript
// CURRENT (VULNERABLE):
document.getElementById('zone-preview-score').addEventListener('input', updateZonePreview);
// No validation before calling updateZonePreview()
```

**Attack Vector:**
- Preview score input accepts any value (no type="number" or bounds)
- `updateZonePreview()` uses value directly in calculations
- Out-of-range values (negative, >100) can break zone detection logic

**Fix (from ddff5dd):**
```javascript
// CORRECT (with validation):
document.getElementById('zone-preview-score').addEventListener('input', e => {
    let score = parseInt(e.target.value, 10);
    if (isNaN(score) || score < 0 || score > 100) score = 50;
    e.target.value = score;  // Clamp input
    updateZonePreview();
});
```

**Required Action:**
- ✅ Restore validation for zone preview score input (clamp to 0-100)
- ✅ Sanitize input value before processing

---

### 3. ❌ Missing URL Encoding for Zone Parameters
**Severity:** HIGH  
**Location:** `builder/index.html` lines ~3220-3230 (`updateHash()`)  
**Status:** REGRESSION (fix was in ddff5dd but removed)

**Issue:**
```javascript
// CURRENT (VULNERABLE):
if (state.zoneEmergenceMax !== undefined && state.zoneEmergenceMax !== 25) {
    params.push('zem=' + state.zoneEmergenceMax);  // Not encoded!
}
```

**Attack Vector:**
- Zone values are inserted directly into URL hash without encoding
- If validation is bypassed, special characters could break URL parsing
- While numeric values are lower risk, encoding is a defense-in-depth best practice

**Fix (from ddff5dd):**
```javascript
// CORRECT (with encoding):
if (state.zoneEmergenceMax !== undefined && state.zoneEmergenceMax !== 25) {
    params.push('zem=' + encodeURIComponent(state.zoneEmergenceMax));
}
```

**Required Action:**
- ✅ Restore `encodeURIComponent()` for all zone hash parameters (zem, zmm, zam)

---

### 4. ❌ Missing Re-validation Before Build Submission
**Severity:** CRITICAL  
**Location:** `builder/index.html` lines ~3410-3420 (`triggerBuild()`)  
**Status:** REGRESSION (fix was in ddff5dd but removed)

**Issue:**
```javascript
// CURRENT (VULNERABLE):
if (state.zoneEmergenceMax !== undefined && state.zoneEmergenceMax !== 25) {
    inputs.zone_emergence_max = String(state.zoneEmergenceMax);  // No validation!
}
```

**Attack Vector:**
- Final defense layer is missing - state values sent to backend without validation
- If front-end validation is bypassed (dev tools, tampered script), invalid values reach GitHub Actions
- Could cause build failures or unexpected firmware behavior

**Fix (from ddff5dd):**
```javascript
// CORRECT (with re-validation):
// Re-validate zone inputs before sending (PR #63 security fix)
const clampZone = (val, def, min, max) => {
    const n = parseInt(val, 10);
    return (Number.isFinite(n) && n >= min && n <= max) ? n : def;
};
const zem = clampZone(state.zoneEmergenceMax || 25, 25, 15, 35);
const zmm = clampZone(state.zoneMomentumMax || 50, 50, 40, 60);
const zam = clampZone(state.zoneActiveMax || 75, 75, 65, 85);

// Use validated values
if (zem !== 25) {
    inputs.zone_emergence_max = String(zem);
}
if (zmm !== 50) {
    inputs.zone_momentum_max = String(zmm);
}
if (zam !== 75) {
    inputs.zone_active_max = String(zam);
}
```

**Required Action:**
- ✅ Restore re-validation logic in `triggerBuild()` before sending to backend
- ✅ Use validated zone values (`zem`, `zmm`, `zam`) instead of raw state

---

## Medium Issues (Should Fix)

### 5. ⚠️ HTML Input Attributes Missing
**Severity:** MEDIUM  
**Location:** Zone slider inputs in HTML markup (lines ~1159-1170)

**Issue:**
While HTML sliders have `min` and `max` attributes, the preview score input is missing type validation:
```html
<input type="number" id="zone-preview-score" value="50" min="0" max="100" ...>
```

**Recommendation:**
- Sliders already have correct HTML attributes (good!)
- Preview score input should have explicit `type="number"`, `min="0"`, `max="100"`
- Note: HTML attributes are bypassable, so JavaScript validation is still required

---

### 6. ⚠️ Missing XSS Protection in Zone Preview Display
**Severity:** LOW  
**Location:** `updateZonePreview()` function (lines ~2540-2555)

**Issue:**
```javascript
document.getElementById('zone-preview-name').textContent = zoneName;
```

**Analysis:**
- ✅ Uses `.textContent` instead of `.innerHTML` (safe!)
- ✅ Zone names are hardcoded strings ('EMERGENCE', 'MOMENTUM', etc.)
- ✅ No user input is inserted into DOM

**Status:** ✅ PASS - Correct use of `.textContent` prevents XSS

---

## Security Controls PASSING ✅

### Positive Findings:

1. ✅ **parseHash() has proper validation:**
   - Zone parameters from URL hash are validated with `clampZone()`
   - Bounds checking: 15-35, 40-60, 65-85
   - NaN handling: defaults to safe values (25, 50, 75)

2. ✅ **HTML escaping in UI rendering:**
   - Uses `escapeHTML()` helper for user-supplied strings
   - DOM manipulation uses `.textContent` not `.innerHTML`
   - No `eval()` or `Function()` calls with user input

3. ✅ **CSP header present:**
   - Content-Security-Policy restricts script sources
   - `script-src 'self' 'unsafe-inline' cdn.jsdelivr.net`
   - Note: `'unsafe-inline'` required for inline scripts

4. ✅ **Input validation for other features:**
   - Homebase lat/lon: proper float validation with range checks
   - Timezone: regex validation with safe format
   - LED values: clamped to 0-15
   - Active hours: clamped to 0-95 quarter-hours

5. ✅ **No client-side storage of secrets:**
   - GitHub token stored in sessionStorage (not localStorage)
   - Token cleared on page unload
   - No tokens in URL hash

---

## Root Cause Analysis

**Why did this happen?**

The security fix in commit ddff5dd was **correctly implemented** and addressed all validation issues. However, a subsequent commit (e96c81b - PR 6 documentation) appears to have either:
1. Merged an older version of the file
2. Manually edited the file and accidentally removed the validation
3. Cherry-picked changes without including the security fix

**Evidence:**
```bash
$ git diff ddff5dd HEAD -- builder/index.html | grep "isNaN(val)"
-        if (isNaN(val) || val < 15 || val > 35) val = 25;
-        if (isNaN(val) || val < 40 || val > 60) val = 50;
-        if (isNaN(val) || val < 65 || val > 85) val = 75;
-        if (isNaN(score) || score < 0 || score > 100) score = 50;
```

All four validation checks were removed between ddff5dd and HEAD.

---

## Remediation Plan

### Immediate Actions (REQUIRED for PR approval):

1. **Restore commit ddff5dd changes:**
   ```bash
   # Option 1: Reapply the security fix patch
   git show ddff5dd -- builder/index.html > /tmp/security-fix.patch
   git apply /tmp/security-fix.patch
   
   # Option 2: Cherry-pick the fix (if no conflicts)
   git cherry-pick ddff5dd
   ```

2. **Verify all four fixes are present:**
   - ✅ Zone slider event handlers have `isNaN()` and bounds checks
   - ✅ Zone preview score input has validation
   - ✅ `updateHash()` uses `encodeURIComponent()` for zone params
   - ✅ `triggerBuild()` has `clampZone()` re-validation logic

3. **Test validation:**
   - Open browser dev tools
   - Attempt to set slider values outside ranges via JavaScript
   - Verify values are clamped to safe defaults
   - Verify build inputs contain only validated values

4. **Add regression test:**
   - Document expected behavior in PR description
   - Add manual QA checklist for validation testing

---

## Testing Recommendations

### Manual Security Tests:

```javascript
// Test 1: Attempt to set invalid zone values via console
document.getElementById('zone-emergence-max').value = 999;
document.getElementById('zone-emergence-max').dispatchEvent(new Event('input'));
// Expected: Value clamped to 25 (default), not 999

// Test 2: Attempt to inject NaN
document.getElementById('zone-momentum-max').value = "hacker";
document.getElementById('zone-momentum-max').dispatchEvent(new Event('input'));
// Expected: Value clamped to 50 (default), not NaN

// Test 3: Check URL hash encoding
// Set zone values, then check URL hash for proper encoding
// Expected: #...&zem=30&zmm=55&zam=80 (URL-encoded numbers)

// Test 4: Inspect build payload
// Trigger build, capture network request, inspect JSON payload
// Expected: zone_emergence_max, zone_momentum_max, zone_active_max are valid integers
```

---

## Comparison with Existing Security Features

**What's working well:**
- Homebase location inputs: Excellent validation (lat: -90 to 90, lon: -180 to 180)
- Timezone input: Regex validation with safe format checking
- LED sliders: Proper bounds clamping (0-15)
- Token handling: Secure storage in sessionStorage, not URL

**Zone inputs should match this quality standard.**

---

## Final Recommendation

**❌ SECURITY APPROVAL: DENIED**

**Blockers:**
1. Critical input validation missing (regression from ddff5dd)
2. Missing URL encoding for zone parameters
3. Missing re-validation before backend submission
4. Missing bounds checking in zone preview input

**Required Actions:**
1. Restore all changes from commit ddff5dd
2. Verify validation is present in ALL four locations
3. Test validation with manual security tests (above)
4. Re-submit for security review

**Once fixed:**
- ✅ Update PR description to document security validation
- ✅ Request re-review from security specialist
- ✅ Add "security-reviewed" label to PR

---

## Additional Notes

**Good practices observed:**
- Use of `escapeHTML()` helper function
- Consistent use of `.textContent` over `.innerHTML`
- Defense-in-depth approach (HTML attributes + JS validation)
- Clear separation of concerns (validation in parseHash, triggerBuild)

**Code quality:**
- Well-structured JavaScript with clear function names
- Good comments explaining validation logic
- Consistent coding style

**Future recommendations:**
- Consider adding automated security tests for input validation
- Document validation ranges in code comments
- Add JSDoc type annotations for state object properties

---

## References

- **Fix commit:** ddff5dd (2026-02-20) - "fix(builder): Add input validation to zone configuration UI"
- **Regression commit:** e96c81b (Phase 3 PR 6 documentation) - inadvertently removed security fixes
- **HTML slider spec:** https://developer.mozilla.org/en-US/docs/Web/HTML/Element/input/range
- **Input validation best practices:** OWASP Input Validation Cheat Sheet

---

**Reviewer Signature:** Security Specialist Agent  
**Review Date:** 2026-02-20 13:45 AKST  
**Next Review Required:** After fixes applied, re-review validation code
