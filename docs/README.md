# Sensor Watch Pro Documentation

This directory contains research, architecture, and feature documentation for the Sensor Watch Pro Active Hours + Sleep Tracking project.

**Last Updated:** 2026-02-26 (Documentation cleanup - see [archive/](archive/) for historical docs)

---

## Quick Links

- **New to the project?** Start with [Project Overview](#project-overview) below
- **Implementing a feature?** See [architecture/](architecture/) for system specs
- **Research backing decisions?** Check [research/](research/)
- **Latest status?** Read [PHASE_ENGINE_COMPLETION.md](PHASE_ENGINE_COMPLETION.md)

---

## Current Documentation

### Core Documents

| Document | Purpose | Last Updated |
|----------|---------|--------------|
| [PHASE_ENGINE_COMPLETION.md](PHASE_ENGINE_COMPLETION.md) | Phase engine testing & integration completion | 2026-02-20 |
| [PHASE_ENGINE_DATA_ARCHITECTURE.md](PHASE_ENGINE_DATA_ARCHITECTURE.md) | Data flow, persistence, and sensor integration | 2026-02-26 |

---

### `/architecture` - System Specifications

Current architecture and protocol specifications:

| Document | Description |
|----------|-------------|
| [sensor-watch-comms-architecture.md](architecture/sensor-watch-comms-architecture.md) | Unified comms system (optical RX + acoustic TX) |
| [sensor-watch-active-hours-spec.md](architecture/sensor-watch-active-hours-spec.md) | Active Hours system specification |
| [circadian-score-algorithm-v2.md](architecture/circadian-score-algorithm-v2.md) | Circadian Score (0-100) algorithm - 75% evidence-based |
| [sleep-algorithm-decision.md](architecture/sleep-algorithm-decision.md) | Sleep tracking algorithm choice (Cole-Kripke + Light Enhancement) |
| [optical-rx-specification.md](architecture/optical-rx-specification.md) | Optical receiver protocol specification |

---

### `/research` - Research & Validation

Academic and industry research backing our design decisions:

| Document | Description |
|----------|-------------|
| [sleep_wearables_research_summary.md](research/sleep_wearables_research_summary.md) | Commercial sleep tracker research (Oura, Fitbit, Apple, WHOOP) |
| [CALIBRATION_CHECKLIST.md](research/CALIBRATION_CHECKLIST.md) | System calibration procedures and validation |
| [COMPETITIVE_ANALYSIS.md](research/COMPETITIVE_ANALYSIS.md) | Competitive landscape analysis |
| [TUNING_PARAMETERS.md](research/TUNING_PARAMETERS.md) | Tunable parameters and their effects |

---

### `/features` - Feature Documentation

Feature-specific documentation and integration guides:

| Document | Description |
|----------|-------------|
| [TAP_TO_WAKE.md](features/TAP_TO_WAKE.md) | Tap-to-wake implementation and behavior |
| [SLEEP_TRACKER_INTEGRATION.md](features/SLEEP_TRACKER_INTEGRATION.md) | Cole-Kripke + Light enhancement implementation guide |

---

### `/reviews` - Analysis & Planning

Forward-looking analysis and enhancement planning:

| Document | Description |
|----------|-------------|
| [DOGFOOD_REPORT.md](reviews/DOGFOOD_REPORT.md) | Real-world usage findings and improvements |
| [ENHANCEMENT_ROADMAP.md](reviews/ENHANCEMENT_ROADMAP.md) | Future enhancement plans |

---

### `/archive` - Historical Documentation

Archived documentation includes:
- **Phase completion summaries** (Phase 1-3)
- **Phase 4 planning docs** (superseded by current implementation)
- **Task/PR completion summaries** (historical deliverables)
- **Stream completion reports** (Active Hours implementation streams)
- **Security reviews** (PR-specific security audits)
- **Build/test reports** (one-time testing and dogfooding reports)
- **Implementation details** (completed implementation specs)

See [archive/](archive/) for full historical context.

---

## Project Overview

**Goal:** Transform Sensor Watch Pro into a circadian health tracker with:
- Active Hours sleep detection (0400-2300 default)
- Smart alarm (window-based, light sleep wake)
- Circadian Score (0-100, timing + light + duration + efficiency)
- Sleep Score (0-100, single-night quality)
- Six biological metrics (Sleep Debt, Energy, Mood, Wake Momentum, Comfort, Jet Lag*)
- Adaptive playlist controller (zone-based face rotation)
- Unified comms (optical RX for config, acoustic TX for data export)
- HealthKit integration (sleep analysis, light exposure, activity proxy)

**Hardware:**
- LIS2DW12 accelerometer (6D orientation detection)
- Light sensor (ADC, 0-255 log-scaled lux)
- NO HRV, heart rate, SpO2, skin temp

**Constraints:**
- Embedded C implementation
- <20µA power budget during sleep
- No cloud dependencies
- Local-first data ownership

**Current Status:** Phase 4BC (Playlist Dispatch) - Active development

---

## Resource Impact

**Flash:** ~18 KB total (~7% of 256 KB budget)  
**RAM:** 72 bytes total (<1% of 32 KB budget)  
**BKUP:** 5 bytes across 2 registers  

---

## Documentation Standards

### When to Create New Docs

**Create in root or subdirectories for:**
- Architecture specifications (→ `architecture/`)
- Research summaries (→ `research/`)
- Feature implementation guides (→ `features/`)
- Forward-looking analysis (→ `reviews/`)

**Create in archive/ for:**
- Phase/PR completion summaries
- One-time test reports
- Historical planning documents
- Obsolete specifications

### Documentation Maintenance

**Update existing docs when:**
- Architecture changes
- Research findings change decisions
- Feature behavior is modified

**Archive docs when:**
- Work is completed and merged
- Planning docs are superseded
- Information is duplicated elsewhere

---

## Contributing

When adding documentation:
1. Place in appropriate subdirectory
2. Update this README.md index
3. Link to related documents
4. Archive obsolete docs instead of deleting

---

## License

See main repository LICENSE file.
