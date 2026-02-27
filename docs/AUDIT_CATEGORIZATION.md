# Documentation Audit - Categorization Plan
**Date:** 2026-02-26  
**Auditor:** Subagent 7f8d9f01  
**Goal:** Clean up docs to reflect current state, remove obsolete content

---

## File Categorization

### ✅ KEEP - Current, Relevant (15 files)

#### Core Documentation
- `README.md` - Index (needs update after cleanup)
- `PHASE_ENGINE_COMPLETION.md` - Recent completion summary (Feb 20)
- `PHASE_ENGINE_DATA_ARCHITECTURE.md` - Data systems (Feb 26, NEW)

#### Architecture (5 files - all current)
- `architecture/circadian-score-algorithm-v2.md`
- `architecture/optical-rx-specification.md`
- `architecture/sensor-watch-active-hours-spec.md`
- `architecture/sensor-watch-comms-architecture.md`
- `architecture/sleep-algorithm-decision.md`

#### Research (4 files - all relevant)
- `research/CALIBRATION_CHECKLIST.md`
- `research/COMPETITIVE_ANALYSIS.md`
- `research/TUNING_PARAMETERS.md`
- `research/sleep_wearables_research_summary.md`

#### Features (2 files - current implementations)
- `features/SLEEP_TRACKER_INTEGRATION.md`
- `features/TAP_TO_WAKE.md`

#### Reviews (2 files - forward-looking)
- `reviews/ENHANCEMENT_ROADMAP.md`
- `reviews/DOGFOOD_REPORT.md` (recent, Feb 20+)

---

### 🔄 UPDATE - Needs Consolidation (0 files)

*All consolidation will be handled via archiving and updating README.md*

---

### ❌ ARCHIVE - Obsolete/Historical (41 files)

#### Phase Completion Summaries (11 files → archive/phase_completions/)
*One-time summaries of completed work - historical reference only*

- `PHASE1_COMPLETION_SUMMARY.md`
- `PHASE3_BASELINE_RESEARCH.md`
- `PHASE3_BUDGET_REPORT.md`
- `PHASE3_COMPLETE.md`
- `PHASE3_DOGFOOD_REPORT.md`
- `PHASE3_EMULATOR_TEST_REPORT.md`
- `PHASE3_IMPLEMENTATION_PLAN.md`
- `PHASE3_MERGE_STRATEGY.md`
- `PHASE3_PREWORK.md`
- `PHASE3_SECURITY_FIXES_COMPLETE.md`
- `PHASE3_STUB_AUDIT.md`

#### Phase 4 Planning Docs (3 files → archive/phase4_planning/)
*Planning documents for work in progress/completed*

- `PHASE4A_SENSOR_PLAN.md`
- `PHASE4E_IMPLEMENTATION.md`
- `PHASE4_TUNING_PLAN.md`

#### Task/PR Completion Summaries (7 files → archive/task_completions/)
*One-time PR descriptions - no longer needed*

- `TASK_COMPLETE.md`
- `TASK_COMPLETE_PR2.md`
- `TASK_COMPLETE_PR3.md`
- `TASK_COMPLETE_PR5.md`
- `TASK_PR6_SUMMARY.md`
- `ZONE_FACES_FIX_COMPLETE.md`
- `ZONE_FACES_LCD_FIX_SUMMARY.md`

#### Stream Completion Summaries (4 files → archive/stream_completions/)
*Historical stream completions - duplicates what's in features/*

- `STREAM1_IMPLEMENTATION.md` (duplicate of features/STREAM1_IMPLEMENTATION.md)
- `STREAM4_COMPLETION_SUMMARY.md` (duplicate of features/STREAM4_COMPLETION_SUMMARY.md)
- `features/STREAM1_COMPLETION_SUMMARY.md`
- `features/STREAM2_COMPLETION_SUMMARY.md`
- `features/STREAM3_COMPLETION_SUMMARY.md`
- `features/STREAM4_COMPLETION_SUMMARY.md`

*Note: features/STREAM3_IMPLEMENTATION.md - check if current or archive*

#### PR/Security Reviews (13 files → archive/security_reviews/)
*PR-specific security reviews - completed and merged*

- `reviews/CODE_REVIEW_FINDINGS.md`
- `reviews/PR1_METRICS_CORE.md`
- `reviews/PR2_REMAINING_METRICS.md`
- `reviews/PR3_PLAYLIST_CONTROLLER.md`
- `reviews/PR59_FIXES_REQUIRED.md`
- `reviews/PR63-SECURITY-VERIFICATION.md`
- `reviews/PR6_DELIVERABLES.txt`
- `reviews/SECURITY_FIX_PR63.sh`
- `reviews/SECURITY_REVIEW_PR59.md`
- `reviews/SECURITY_REVIEW_PR60.md`
- `reviews/SECURITY_REVIEW_PR61.md`
- `reviews/SECURITY_REVIEW_PR62.md`
- `reviews/SECURITY_REVIEW_PR63.md`
- `reviews/SECURITY_REVIEW_PR63_SUMMARY.md`
- `reviews/SECURITY_REVIEW_PR64.md`
- `reviews/SECURITY_REVIEW_SUMMARY.md`
- `reviews/SECURITY_REVIEW_SUMMARY_PR62.md`

#### Build/Test Reports (10 files → archive/test_reports/)
*One-time test/build reports*

- `BUILD_REPORT.md`
- `DOGFOOD_FINDINGS.md`
- `DOGFOOD_FIXES.md`
- `DOGFOOD_SUMMARY.md`
- `EMULATOR_BUILD_REPORT.md`
- `EMULATOR_TASK_SUMMARY.md`
- `EMULATOR_TEST.md`
- `INTEGRATION_AUDIT.md`
- `ONLINE_BUILDER_TEST.md`

#### Implementation Details (6 files → archive/implementations/)
*Completed implementation notes - no longer needed for reference*

- `ANOMALY_DETECTION_SPEC.md`
- `COMFORT_LUNAR_INTEGRATION.md`
- `EM_SAD_INTEGRATION.md`
- `FIRST_BOOT_DEFAULTS_PLAN.md`
- `LUNAR_ALTERNATIVES.md`
- `OPTICAL_RX_REVIEW.md`
- `OPTICAL_RX_VALIDATION.md`
- `PAGES_REVIEW.md`
- `features/TAP_TO_WAKE_SLEEP_MODE_ANALYSIS.md`

---

## Archive Directory Structure

```
archive/
├── phase_completions/          # Phase 1-3 completion summaries
├── phase4_planning/            # Phase 4 planning docs
├── task_completions/           # PR/task completion summaries
├── stream_completions/         # Stream implementation completions
├── security_reviews/           # All PR-specific security reviews
├── test_reports/               # Build/test/dogfood reports
└── implementations/            # Completed implementation specs
```

---

## Keep Structure (After Cleanup)

```
docs/
├── README.md                           # Updated index
├── PHASE_ENGINE_COMPLETION.md          # Most recent completion
├── PHASE_ENGINE_DATA_ARCHITECTURE.md   # Current data architecture
├── architecture/                       # 5 current specs
├── features/                           # 2 current implementations
├── research/                           # 4 research docs
├── reviews/                            # 2 forward-looking docs
└── archive/                            # 41 historical docs (organized)
```

---

## Rationale

### Why Archive vs Delete?
- Preserves historical context if ever needed
- Safe recovery if wrong decision made
- Git history still available but docs folder is clean

### What Makes a Doc "Current"?
1. Referenced by active code
2. Needed for onboarding/understanding system
3. Living document (tuning guides, architecture specs)
4. Research backing current decisions

### What Makes a Doc "Archive"?
1. One-time deliverable (PR summaries, completion reports)
2. Describes work already merged/completed
3. Duplicates information available elsewhere
4. Historical planning for completed phases

---

## Next Steps

1. Create archive directory structure
2. Move files to appropriate archive subdirectories
3. Update README.md with new structure
4. Create DOCUMENTATION_CLEANUP_SUMMARY.md
5. Commit all changes
