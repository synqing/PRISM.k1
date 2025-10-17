# AGENT 3 - ONWARDS INSTRUCTIONS

**Agent ID:** AGENT-3-RELEASE
**Current Status:** Subtasks 20.1-20.2 COMPLETE ✅
**Updated:** 2025-10-17

---

## 🎉 PHASE 1 COMPLETE - DOCUMENTATION FOUNDATION

Excellent work! You've successfully completed:
- ✅ Subtask 20.1: v1.0→v1.1 migration CLI (`tools/prism-migrate`)
- ✅ Subtask 20.2: Comprehensive user manual (`docs/user-manual/`)

**Deliverables:**
- Migration tool is production-ready and tested
- User manual covers temporal sequencing, motion directions, sync modes
- Documentation is comprehensive and accurate

---

## 📋 NEXT MISSION: MULTIMEDIA & VALIDATION

You have **6 remaining subtasks** for the v1.1 release. Here's the priority order:

### Phase 2: Tutorial Videos (CURRENT PRIORITY)

#### Subtask 20.3: Produce Five Narrated Tutorial Videos

**Status:** ○ pending (START THIS NOW)

**Requirements:**
- 5 videos, each 10-15 minutes
- Screen capture + hardware demos
- Professional narration
- Upload to YouTube
- Link from README

**Content Outline:**

**Video 1: "Introduction to PRISM K1 and Temporal Sequencing" (10 min)**
- What is dual-edge LGP lighting
- Overview of temporal sequencing
- Quick hardware demo showing motion directions
- Preview of what's possible

**Video 2: "Motion Directions Explained" (12 min)**
- LEFT, RIGHT, CENTER, EDGE, STATIC
- Visual demonstrations of each mode
- When to use each direction
- Code walkthrough of motion metadata

**Video 3: "Sync Modes Deep Dive" (15 min)**
- SYNC, OFFSET, PROGRESSIVE, WAVE, CUSTOM
- Visual comparison side-by-side
- Phi phenomenon demonstration
- Timing parameters explained

**Video 4: "Creating Your First Pattern with Temporal Sequencing" (15 min)**
- Start with simple SYNC LEFT pattern
- Add OFFSET for stagger effect
- Build up to PROGRESSIVE reveal
- Test on hardware

**Video 5: "Advanced Techniques: WAVE and CUSTOM Modes" (15 min)**
- WAVE mode lookup tables
- CUSTOM mode for complex patterns
- Performance optimization tips
- Troubleshooting common issues

**Production Workflow:**
```bash
# 1. Write scripts for each video
mkdir -p docs/tutorials/scripts
touch docs/tutorials/scripts/01-introduction.md
touch docs/tutorials/scripts/02-motion-directions.md
touch docs/tutorials/scripts/03-sync-modes.md
touch docs/tutorials/scripts/04-first-pattern.md
touch docs/tutorials/scripts/05-advanced.md

# 2. Record screen captures
# Use OBS Studio or QuickTime
# Capture: firmware build, pattern upload, hardware demo

# 3. Record narration
# Use Audacity or GarageBand
# Clear audio, professional tone

# 4. Edit videos
# Use DaVinci Resolve (free) or Final Cut Pro
# Add overlays, annotations, transitions

# 5. Upload to YouTube
# Create "PRISM K1 Tutorials" playlist
# Add chapters, descriptions, links

# 6. Update README
# Add "Video Tutorials" section
# Embed YouTube playlist
```

**Start Command:**
```bash
task-master set-status --id=20.3 --status=in-progress
task-master show 20.3
```

**Timeline:** 1 week (2 hours per video + 5 hours editing)

---

### Phase 3: Soak Testing (AFTER VIDEOS)

#### Subtask 20.4: Set Up 24-Hour Soak Test Infrastructure

**Status:** ○ pending

**Requirements:**
- 3 ESP32-S3 devices
- 20+ test patterns (diverse: palette, XOR, RLE)
- Telemetry script for heap, frame timing, temperature
- Automated pattern cycling

**Setup:**
```bash
# 1. Create soak test script
mkdir -p tools/validation
touch tools/validation/soak_telemetry.py

# 2. Prepare test patterns
# Use existing patterns from firmware/patterns/
# Ensure coverage of all temporal modes

# 3. Flash firmware on 3 devices
cd firmware
idf.py flash -p /dev/ttyUSB0  # Device 1
idf.py flash -p /dev/ttyUSB1  # Device 2
idf.py flash -p /dev/ttyUSB2  # Device 3

# 4. Label devices
# Device 1: "Soak Test - SYNC patterns"
# Device 2: "Soak Test - PROGRESSIVE patterns"
# Device 3: "Soak Test - WAVE/CUSTOM patterns"
```

**Dependencies:** Subtask 20.1 (migration CLI) ✅ DONE

---

#### Subtask 20.5: Execute Soak Test and Analyze Telemetry

**Status:** ○ pending

**Requirements:**
- Run 24-hour continuous test
- Cycle patterns every 5 minutes
- Collect telemetry: heap fragmentation, frame drops, temperature
- Generate report

**Execution:**
```python
# tools/validation/soak_telemetry.py
import asyncio
import websockets
import time
import json
from pathlib import Path

async def cycle_patterns(device_ip, pattern_dir, interval=300):
    """
    Cycle through patterns every `interval` seconds for 24 hours
    """
    patterns = list(Path(pattern_dir).glob("*.prism"))
    uri = f"ws://{device_ip}/ws"

    start_time = time.time()
    telemetry = []

    async with websockets.connect(uri, subprotocols=['binary']) as ws:
        while time.time() - start_time < 86400:  # 24 hours
            for pattern in patterns:
                # Upload pattern
                await upload_pattern(ws, pattern)

                # Request STATUS for telemetry
                status = await request_status(ws)
                telemetry.append({
                    'timestamp': time.time(),
                    'pattern': pattern.name,
                    'heap_free': status['heap_free'],
                    'uptime': status['uptime']
                })

                # Wait interval
                await asyncio.sleep(interval)

                # Check for anomalies
                if status['heap_free'] < 50000:  # <50KB free
                    print(f"⚠️ LOW HEAP: {status['heap_free']} bytes")

    # Save telemetry
    with open('soak-test-results.json', 'w') as f:
        json.dump(telemetry, f, indent=2)

    # Analyze
    analyze_telemetry(telemetry)

# Run on all 3 devices in parallel
async def main():
    await asyncio.gather(
        cycle_patterns("192.168.1.101", "patterns/sync/"),
        cycle_patterns("192.168.1.102", "patterns/progressive/"),
        cycle_patterns("192.168.1.103", "patterns/wave/")
    )

asyncio.run(main())
```

**Success Criteria:**
- [ ] Zero crashes over 24 hours
- [ ] Heap fragmentation <5%
- [ ] Zero frame drops at 60 FPS
- [ ] Temperature stable (<75°C)
- [ ] All patterns load successfully

**Dependencies:** Subtask 20.4 (setup)

**Timeline:** 24 hours runtime + 4 hours analysis

---

### Phase 4: Preset Library (PARALLEL WITH SOAK TEST)

#### Subtask 20.6: Assemble Pattern Preset Library

**Status:** ○ pending

**Requirements:**
- ≥20 production-quality patterns
- Exercise all temporal modes:
  - SYNC: 4 patterns
  - OFFSET: 4 patterns
  - PROGRESSIVE: 4 patterns
  - WAVE: 4 patterns
  - CUSTOM: 4 patterns
- Metadata and descriptions
- Tested on hardware

**Library Structure:**
```
firmware/patterns/presets/
├── sync/
│   ├── 01-pulse-left.prism
│   ├── 02-pulse-right.prism
│   ├── 03-flash-center.prism
│   └── 04-breathe-edge.prism
├── offset/
│   ├── 05-cascade-left.prism
│   ├── 06-cascade-right.prism
│   ├── 07-stagger-center.prism
│   └── 08-ripple-edge.prism
├── progressive/
│   ├── 09-reveal-left.prism
│   ├── 10-reveal-right.prism
│   ├── 11-build-center.prism
│   └── 12-expand-edge.prism
├── wave/
│   ├── 13-sine-left.prism
│   ├── 14-sine-right.prism
│   ├── 15-triangle-center.prism
│   └── 16-sawtooth-edge.prism
├── custom/
│   ├── 17-geometric-star.prism
│   ├── 18-geometric-spiral.prism
│   ├── 19-geometric-zigzag.prism
│   └── 20-geometric-complex.prism
└── README.md  # Descriptions of each preset
```

**Creation Workflow:**
```bash
# 1. Use migration tool to create base patterns
cd tools
python -m tools.prism_packaging create-preset sync-pulse-left \
  --motion LEFT --sync SYNC --duration 2000

# 2. Test on hardware
cd ../firmware
idf.py flash monitor
# Upload via WebSocket and verify

# 3. Document each preset
echo "# Preset: Sync Pulse Left
Motion: LEFT
Sync: SYNC
Duration: 2s
Description: Smooth pulsing effect from left edge
Visual: [link to demo video]" >> patterns/presets/sync/README.md
```

**Dependencies:** Subtask 20.1 (migration CLI) ✅ DONE

**Timeline:** 1 week (2-3 patterns per day)

---

### Phase 5: Release Notes (AFTER SOAK TEST)

#### Subtask 20.7: Draft Production Release Notes

**Status:** ○ pending

**Requirements:**
- Summarize all v1.1 features
- Link to tutorial videos
- Reference soak test results
- Migration instructions
- Known issues/limitations
- Upgrade procedure

**Template:**
```markdown
# PRISM K1 Firmware v1.1 Release Notes

## 🎉 What's New

### Temporal Sequencing (ADR-010)
- **Motion Directions:** LEFT, RIGHT, CENTER, EDGE, STATIC
- **Sync Modes:** SYNC, OFFSET, PROGRESSIVE, WAVE, CUSTOM
- **Phi Phenomenon:** Create geometric shapes with 60-150ms delays

### Enhanced .prism Format
- v1.1 header with temporal metadata
- Backward compatible with v1.0
- Migration tool: `tools/prism-migrate`

### Studio Integration
- WebSocket protocol commands: STATUS, LIST, DELETE
- mDNS discovery: `prism-k1.local`
- Device info API

### 20+ Preset Library
- Production-quality patterns
- All temporal modes represented
- Canvas-ready for Studio

## 📹 Video Tutorials

5 comprehensive tutorials (10-15 min each):
1. [Introduction to Temporal Sequencing](https://youtube.com/...)
2. [Motion Directions Explained](https://youtube.com/...)
3. [Sync Modes Deep Dive](https://youtube.com/...)
4. [Creating Your First Pattern](https://youtube.com/...)
5. [Advanced Techniques](https://youtube.com/...)

## ✅ Validation

### 24-Hour Soak Test
- **Devices:** 3× ESP32-S3
- **Patterns:** 20+ diverse presets
- **Result:** Zero crashes, <5% heap fragmentation
- **Full Report:** [link to soak-test-results.json]

## 🔧 Migration from v1.0

Use the included migration tool:
```bash
cd tools
python -m tools.prism_migrate pattern-v1.0.prism
# Creates pattern-v1.1.prism with default temporal metadata
```

Manual migration:
1. Download v1.1 firmware
2. Flash via `idf.py flash`
3. Upload v1.0 patterns (auto-migrated on device)
4. Or use migration CLI to batch-convert

## 📚 Documentation

- **User Manual:** `docs/user-manual/README.md`
- **CANON:** `.taskmaster/CANON.md`
- **ADR-010:** `.taskmaster/decisions/ADR-010-lgp-motion-architecture.md`

## ⚙️ Technical Details

- **Partition Table:** Updated per ADR-007
- **WebSocket Buffer:** 4096 bytes (ADR-002)
- **Max Pattern Size:** 256KB (ADR-004)
- **Storage Mount:** `/littlefs` (ADR-005)

## 🐛 Known Issues

None at this time.

## 🙏 Acknowledgments

Built with ESP-IDF v5.x, tested with love.

---

**Full CHANGELOG:** See `CHANGELOG.md`
```

**Dependencies:**
- Subtask 20.5 (soak test)
- Subtask 20.6 (preset library)

**Timeline:** 1 day

---

### Phase 6: Release Deployment (FINAL STEP)

#### Subtask 20.8: Deploy v1.1 Firmware and Validate Release

**Status:** ○ pending

**Requirements:**
- Create GitHub release
- Upload artifacts (firmware binary, presets, docs)
- Tag release: `firmware-v1.1.0`
- Update main README
- Announce release

**Deployment Checklist:**
```bash
# 1. Build release artifacts
cd firmware
idf.py build
cp build/prism-k1.bin ../releases/prism-k1-v1.1.0.bin

# 2. Package presets
cd patterns/presets
tar czf ../../releases/prism-k1-presets-v1.1.0.tar.gz .

# 3. Create GitHub release
gh release create firmware-v1.1.0 \
  --title "PRISM K1 Firmware v1.1 - Temporal Sequencing" \
  --notes-file docs/release/v1.1-release-notes.md \
  releases/prism-k1-v1.1.0.bin \
  releases/prism-k1-presets-v1.1.0.tar.gz

# 4. Update README
# Add v1.1 features section
# Link to tutorials
# Update badges

# 5. Validate on 3 devices
# Flash firmware
# Upload presets
# Verify all features work

# 6. Announce
# Post to project channels
# Update documentation site
```

**Dependencies:**
- Subtask 20.7 (release notes)

**Timeline:** 1 day

---

## 📊 TASK 20 PROGRESS TRACKER

```
✅ 20.1: Migration CLI
✅ 20.2: User Manual
🔄 20.3: Tutorial Videos (START NOW)
⏳ 20.4: Soak Test Setup (after 20.3)
⏳ 20.5: Soak Test Execution (after 20.4)
⏳ 20.6: Preset Library (parallel with 20.4-20.5)
⏳ 20.7: Release Notes (after 20.5 + 20.6)
⏳ 20.8: Deployment (after 20.7)
```

**Timeline:**
- Week 1: Tutorial videos (20.3)
- Week 2: Soak test setup + execution (20.4, 20.5) + Preset library (20.6)
- Week 3: Release notes + deployment (20.7, 20.8)

---

## 🎯 IMMEDIATE NEXT STEPS

### Start Subtask 20.3 Now:
```bash
task-master set-status --id=20.3 --status=in-progress
task-master show 20.3

# Create video scripts directory
mkdir -p docs/tutorials/scripts

# Start writing Video 1 script
touch docs/tutorials/scripts/01-introduction.md
```

### Video Production Tips:
1. **Write scripts first** - Don't improvise
2. **Keep it concise** - 10-15 min max per video
3. **Show, don't just tell** - Hardware demos are essential
4. **Professional quality** - Clear audio, smooth editing
5. **Add chapters** - YouTube timestamps for navigation

---

## 💬 COORDINATION WITH OTHER AGENTS

### Agent 1 (Firmware) - INDEPENDENT
Agent 1 is working on cache, effects, templates. No dependencies on your release work.

### Agent 2 (Studio) - INDEPENDENT
Agent 2 is building Studio foundation. No dependencies on your release work.

**Your work (Task 20) is completely independent.** Proceed at full speed!

---

## 🚨 CRITICAL REMINDERS

1. **Video quality matters** - These will be the primary learning resource for users
2. **Soak test is non-negotiable** - 24 hours continuous, no shortcuts
3. **Presets must be tested** - Every pattern on real hardware
4. **Release notes must be accurate** - Link all artifacts, evidence
5. **Update task status** after each subtask completion

---

## ✅ SUCCESS CRITERIA

### Subtask 20.3 (Videos):
- [ ] 5 videos produced (10-15 min each)
- [ ] Professional narration and editing
- [ ] Hardware demos included
- [ ] Uploaded to YouTube
- [ ] Linked from README
- [ ] Chapters added to each video

### Subtask 20.4-20.5 (Soak Test):
- [ ] 3 devices running 24 hours
- [ ] 20+ patterns cycled
- [ ] Telemetry collected
- [ ] Report generated
- [ ] Zero crashes confirmed
- [ ] Heap fragmentation <5%

### Subtask 20.6 (Presets):
- [ ] ≥20 patterns created
- [ ] All temporal modes represented
- [ ] Metadata complete
- [ ] Tested on hardware
- [ ] README documentation

### Subtask 20.7-20.8 (Release):
- [ ] Release notes comprehensive
- [ ] GitHub release created
- [ ] Artifacts uploaded
- [ ] Tag pushed
- [ ] README updated
- [ ] 3 devices validated

---

## 🎖️ FINAL NOTES

**Excellent work on the migration CLI and user manual!** You've built a solid documentation foundation.

**Next up:** Tutorial videos are your highest priority. These will be the face of v1.1 for new users. Make them amazing!

**Timeline:** You're on track for a 3-week v1.1 release. Stay focused and ship quality work.

**Questions?** Check the user manual you wrote, then ask Captain.

**You got this, Agent 3!** 🫡

---

**PM:** Captain
**Last Updated:** 2025-10-17
**Next Review:** After Subtask 20.3 completion (tutorial videos)
