# Phase 3 Merge Strategy

**Created:** 2026-02-20  
**Status:** Active  
**Purpose:** Document sequential merge workflow for Phase 3 PRs

---

## Overview

Phase 3 consists of 6 PRs with sequential dependencies. This document defines the merge order, conflict resolution strategy, and workflow expectations.

## PR Dependency Chain

```
main
 ↓
#59: Metric Engine Core (SD + Comfort)
 ↓
#60: Remaining Metrics (EM, WK, Energy)
 ↓
#61: Playlist Controller
 ↓
#62: Zone Faces
 ↓
#63: Builder UI
 ↓
#64: Integration & Polish
```

## Merge Order (MANDATORY)

**PRs MUST be merged in this exact order:**

1. **PR #59** - Metric Engine Core
   - **Why first:** Establishes core metric engine APIs and BKUP storage
   - **Dependencies:** None (builds on main)
   - **Blocks:** #60, #61, #62

2. **PR #60** - Remaining Metrics
   - **Why second:** Extends metric engine with EM, WK, Energy
   - **Dependencies:** #59 (uses metric engine core)
   - **Blocks:** #61, #62

3. **PR #61** - Playlist Controller
   - **Why third:** Needs all metrics to compute zone weights
   - **Dependencies:** #59, #60 (uses all metrics)
   - **Blocks:** #62, #64

4. **PR #62** - Zone Faces
   - **Why fourth:** Displays metrics from engine
   - **Dependencies:** #59, #60 (reads metrics)
   - **Blocks:** #64

5. **PR #63** - Builder UI
   - **Why fifth:** Configures zones, but doesn't depend on C code
   - **Dependencies:** None (JavaScript/HTML only)
   - **Blocks:** #64

6. **PR #64** - Integration & Polish
   - **Why last:** Brings everything together
   - **Dependencies:** #59-#63 (integrates all components)
   - **Blocks:** None (final PR)

---

## Workflow Per PR

### Step 1: Pre-Merge Checks
- [ ] CI passing (all builds)
- [ ] Security review complete (@security-specialist)
- [ ] No merge conflicts (rebase if needed)
- [ ] PR description updated with review status

### Step 2: Merge
- [ ] dlorp merges PR (NOT lorp bot)
- [ ] Verify merge to main successful
- [ ] Check main branch CI passes

### Step 3: Post-Merge Actions
- [ ] Immediately check next PR for conflicts
- [ ] If conflicts: spawn agent to rebase
- [ ] Update next PR security review if needed (rebase changes code)
- [ ] Repeat workflow for next PR

---

## Conflict Resolution Strategy

### When Conflicts Occur

**Expected conflicts:**
- PR #60 and #61 will conflict with #59 until it merges
- Any rebase will trigger new security reviews

**Resolution process:**
1. Wait for blocking PR to merge to main
2. Spawn @general-purpose to rebase conflicting PR
3. Verify rebase successful (git push -f)
4. Trigger CI rebuild
5. Spawn @security-specialist for re-review (code changed)
6. Mark PR ready for merge

### Rebase Command Pattern
```bash
git checkout phase3-pr<N>-<name>
git fetch origin
git rebase origin/main
# Resolve conflicts if any
git push -f origin phase3-pr<N>-<name>
```

---

## Current Status (2026-02-20 16:10 AKST)

| PR # | Status | CI | Mergeable | Security Review | Blocking |
|------|--------|-----|-----------|-----------------|----------|
| #59 | ✅ MERGED | ✅ | ✅ | ✅ Approved | - |
| #60 | ✅ MERGED | ✅ | ✅ | ✅ Approved | - |
| #61 | Ready | 🔄 Running | ✅ | ✅ Approved | - |
| #62 | Ready | 🔄 Running | ✅ | ✅ Approved | #61 |
| #63 | Ready | 🔄 Running | ✅ | ✅ Approved | #62 |
| #64 | Ready | 🔄 Running | ✅ | ✅ Approved | #61, #62, #63 |

**Latest action:** Cleaned and rebased PRs #61-64 after #60 merge. All now mergeable with clean history. CI running.

**Next action:** Wait for CI to pass on #61, then merge in sequence: #61 → #62 → #63 → #64 (rebasing each after previous merges).

---

## Why This Order Matters

### Technical Dependencies
- **#60 needs #59**: Uses `metrics_engine_t` and core APIs
- **#61 needs #59+#60**: Computes zone scores from all 5 metrics
- **#62 needs #59+#60**: Displays metric values on watch faces
- **#64 needs all**: Integrates full system

### Risk Management
- Merge stable foundation first (#59)
- Build up complexity incrementally
- Catch integration issues early
- Each PR is independently testable

### Workflow Clarity
- Clear blocking relationships
- Predictable conflict patterns
- Easy to track progress
- Simpler rollback if needed

---

## Post-Merge Verification

After each PR merges:
1. **Build test:** `make clean && make BOARD=sensorwatch_red DISPLAY=classic PHASE_ENGINE_ENABLED=1`
2. **Size check:** Verify firmware < 256 KB
3. **Simulator test:** Quick functional check if possible
4. **Documentation:** Update this file with new status

---

## Emergency Rollback

If a merged PR causes critical issues:

```bash
# Revert the merge commit
git revert -m 1 <merge-commit-sha>
git push origin main

# Fix the issue in the PR branch
git checkout phase3-pr<N>-<name>
# ... fix ...
git commit -m "fix: critical issue from merge"
git push origin phase3-pr<N>-<name>

# Re-open PR and repeat workflow
```

---

## Conflict Resolution Log

### 2026-02-20 16:10 AKST - Post-PR60 Cleanup

**Issue:** After PR #60 merged, PRs #61-64 all showed CONFLICTING status due to shared commit history.

**Root cause:** Original branches contained duplicate commits:
- PR #61 included metric commits already in PR #60 + zone face commits from PR #62
- PR #62 included playlist commits from PR #61
- Branches created before proper split, not rebased after earlier PRs merged

**Resolution:**
1. Identified PR-specific commits via `git log --oneline | grep <keyword>`
2. Created clean branches from current main
3. Cherry-picked only unique commits per PR:
   - PR #61: 4 playlist controller commits
   - PR #62: 8 zone face commits  
   - PR #63: 2 builder UI commits
   - PR #64: 4 integration/docs commits
4. Force-pushed clean branches to PR heads
5. Result: All PRs now MERGEABLE with clean history

**Commands used:**
```bash
# Example for PR #61
git checkout main && git pull
git checkout -b phase3-pr3-clean
git cherry-pick 5e0e2c4 f636b6a f2f59e7 a34f5cf  # Playlist commits only
git push -f origin phase3-pr3-clean:phase3-pr3-playlist-controller
```

**Lesson:** When creating dependent PRs, ensure each branch is created from the correct base and contains ONLY its unique work. Rebase frequently as upstream PRs merge.

---

## Lessons Learned (Update After Merge)

### What Worked
- Sequential merge order with explicit dependencies prevented integration chaos
- Security reviews completed before merge attempts
- Force-push cleanup recovered from branch management issues

### What Could Be Better
- AXIOM violation: Should have proactively rebased #61-64 when #60 merged
- Better branch management: Create each PR from correct upstream base, not copying all work
- Monitoring: Need automated alerts when upstream PRs merge affecting dependent PRs

### For Future Phases
- Use `git rebase --onto` for cleaner branch management
- Set up GitHub Actions to notify when dependent PRs need rebasing
- Create dependent PRs from main + cherry-pick, not from each other

---

**Last Updated:** 2026-02-20 13:26 AKST  
**Maintained By:** lorp bot  
**Review Schedule:** Update after each PR merge
