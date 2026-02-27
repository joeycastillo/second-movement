# Documentation Archive

This directory contains historical documentation that is no longer actively maintained but preserved for reference.

**Archived:** 2026-02-26  
**Reason:** Documentation cleanup to focus on current, relevant content

---

## Archive Structure

### `/phase_completions` - Phase Completion Summaries
One-time summaries of completed phases (Phase 1-3):
- Phase 1 completion summary
- Phase 3 baseline research, implementation plan, prework
- Phase 3 budget report, emulator test report
- Phase 3 merge strategy, security fixes, stub audit
- Phase 3 dogfood report, completion summary

**Status:** All work described is completed and merged into main codebase.

---

### `/phase4_planning` - Phase 4 Planning Documents
Planning documents for Phase 4 development:
- Phase 4A sensor plan
- Phase 4E implementation details
- Phase 4 tuning plan

**Status:** Work in progress or completed, planning docs superseded by actual implementation.

---

### `/task_completions` - Task & PR Completion Summaries
One-time deliverables documenting specific PR completions:
- TASK_COMPLETE*.md (PR 1-6 summaries)
- Zone faces fix completion summaries

**Status:** All PRs merged, completion summaries archived for historical reference.

---

### `/stream_completions` - Stream Implementation Summaries
Completion summaries for Active Hours implementation streams (1-4):
- STREAM1: Core sleep logic
- STREAM2: Settings UI
- STREAM3: Smart alarm
- STREAM4: Sleep tracking

**Status:** All streams completed, implementation details moved to features/ or merged into code.

---

### `/security_reviews` - PR Security Reviews
Security audits and reviews for specific pull requests:
- PR59-64 security reviews
- Security fix scripts and verification
- Code review findings

**Status:** All PRs merged, security issues resolved.

---

### `/test_reports` - Build & Test Reports
One-time test, build, and dogfooding reports:
- Emulator build/test reports
- Online builder test verification
- Integration audits
- Dogfood findings, fixes, and summaries

**Status:** Testing completed, findings addressed in code.

---

### `/implementations` - Implementation Detail Documents
Detailed implementation specs for completed features:
- Anomaly detection spec
- Comfort/lunar/EM-SAD integration details
- Optical RX review and validation
- First boot defaults plan
- Tap-to-wake sleep mode analysis
- Pages review
- Lunar alternatives analysis

**Status:** Features implemented, specs archived after completion.

---

## When to Reference Archive

**Use archive docs when:**
- Investigating historical decisions
- Understanding why certain approaches were chosen
- Reviewing what was tried and discarded
- Onboarding and wanting full project context

**Don't use archive for:**
- Current implementation details (see `/architecture` and `/features`)
- Active development guidance (see main docs/)
- Research backing current decisions (see `/research`)

---

## Retention Policy

Archive documents are retained indefinitely for historical reference. They are not actively maintained but can be retrieved if needed.

Git history provides full context and diffs for all archived documents.

---

## Return to Current Documentation

See [../README.md](../README.md) for current, maintained documentation.
