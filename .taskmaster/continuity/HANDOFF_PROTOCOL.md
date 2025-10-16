# AGENT HANDOFF PROTOCOL
## How to Transfer the Knowledge Fortress Mission

**Purpose:** Ensure zero information loss when transferring this critical mission between agents.

---

## FOR CAPTAIN: How to Hand Off to New Agent

### Step 1: Prepare the Handoff (2 minutes)

```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1/.taskmaster/continuity

# Update progress tracker
nano IMPLEMENTATION_BRIEF.md
# Find "CURRENT PROGRESS TRACKER" section
# Mark completed items with [x]
# Note any blockers or issues
```

### Step 2: Create Handoff Package (1 minute)

```bash
# Verify all continuity docs exist
ls -la .taskmaster/continuity/
# Should see:
# - IMPLEMENTATION_BRIEF.md (full details)
# - QUICK_START.md (3-min start guide)
# - CONTEXT_PROMPT.md (LLM prompt)
# - HANDOFF_PROTOCOL.md (this file)

# Optional: Create snapshot
tar -czf handoff-$(date +%Y%m%d-%H%M).tar.gz .taskmaster/continuity/
```

### Step 3: Brief New Agent (30 seconds)

**Say this to new agent:**

```
I need you to take over implementation of the PRISM K1 Knowledge Fortress.

This is a critical mission to prevent specification drift that nearly destroyed the project.

All context is documented. Start by reading this file:
/Users/spectrasynq/Workspace_Management/Software/PRISM.k1/.taskmaster/continuity/CONTEXT_PROMPT.md

Copy the entire code block from that file and paste it into your context.

Then acknowledge you understand the mission by completing the comprehension check below.
```

### Step 4: Verify Agent Understanding (2 minutes)

Ask new agent these questions:

1. **What problem are we solving?**
   ✓ Correct: "Specification drift from conflicting docs"
   ✗ Wrong: Anything else

2. **What's the solution called?**
   ✓ Correct: "Knowledge Fortress" or "ADR system with CANON"
   ✗ Wrong: Anything else

3. **What are the 4 phases?**
   ✓ Correct: Emergency, Research, Automation, Documentation
   ✗ Wrong: Missing any phase

4. **What's the MOST important rule?**
   ✓ Correct: "Never edit CANON.md manually"
   ✗ Wrong: Anything else

5. **Where do you check current progress?**
   ✓ Correct: "IMPLEMENTATION_BRIEF.md CURRENT PROGRESS TRACKER section"
   ✗ Wrong: Anywhere else

If agent answers all correctly: **HANDOFF APPROVED**

If any wrong: **Agent must re-read CONTEXT_PROMPT.md**

---

## FOR NEW AGENT: How to Accept Handoff

### Step 1: Receive Context (1 minute)

Captain will give you a file path. Open it:

```bash
cat /Users/spectrasynq/Workspace_Management/Software/PRISM.k1/.taskmaster/continuity/CONTEXT_PROMPT.md
```

Copy the **entire code block** and paste into your conversation.

### Step 2: Read Quick Start (3 minutes)

```bash
cat /Users/spectrasynq/Workspace_Management/Software/PRISM.k1/.taskmaster/continuity/QUICK_START.md
```

This gives you the 80/20 - essential info to start immediately.

### Step 3: Prove Comprehension (1 minute)

Answer these questions to Captain:

1. **What problem are we solving?**
   [Your answer here]

2. **What's the solution called?**
   [Your answer here]

3. **What are the 4 phases?**
   [Your answer here]

4. **What's the MOST important rule?**
   [Your answer here]

5. **Where do you check current progress?**
   [Your answer here]

**Captain will verify your answers before you proceed.**

### Step 4: Check Current State (2 minutes)

```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1

# Check progress
grep -A 40 "CURRENT PROGRESS TRACKER" .taskmaster/continuity/IMPLEMENTATION_BRIEF.md

# Find first incomplete phase
# Look for [ ] instead of [x]
```

### Step 5: Begin Implementation (Immediate)

Based on which phase is incomplete:

**Phase 1:** Follow QUICK_START.md "PHASE 1 QUICK EXECUTION"  
**Phase 2:** Read IMPLEMENTATION_BRIEF.md "Phase 2: Research Reorganization"  
**Phase 3:** Read IMPLEMENTATION_BRIEF.md "Phase 3: Automation Setup"  
**Phase 4:** Read IMPLEMENTATION_BRIEF.md "Phase 4: Documentation & Training"

---

## HANDOFF VERIFICATION CHECKLIST

### For Captain (before handing off):
- [ ] Progress tracker updated in IMPLEMENTATION_BRIEF.md
- [ ] Any blockers documented
- [ ] All continuity files present
- [ ] New agent given CONTEXT_PROMPT.md path
- [ ] New agent passed comprehension check

### For New Agent (before starting):
- [ ] Received CONTEXT_PROMPT.md
- [ ] Read and understood context
- [ ] Passed Captain's comprehension check
- [ ] Located current phase in progress tracker
- [ ] Know which documents to read next
- [ ] Have file system access verified

---

## EMERGENCY HANDOFF (Agent Offline Mid-Task)

If an agent goes offline mid-implementation:

### Captain's Actions:

1. **Document Current State (5 minutes)**
   ```bash
   cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1/.taskmaster/continuity
   
   # Create emergency snapshot
   cat > EMERGENCY_STATE.md << 'EOF'
   # EMERGENCY HANDOFF
   Date: $(date)
   Previous Agent: [name/ID]
   
   ## What Was Being Done
   [Describe the exact task in progress]
   
   ## What's Complete
   [List completed items]
   
   ## What's In Progress
   [Describe partial work]
   
   ## What's Blocked
   [List any blockers]
   
   ## Files Modified
   [List any files changed]
   
   ## Next Steps
   [What should be done next]
   EOF
   
   nano EMERGENCY_STATE.md  # Fill in details
   ```

2. **Brief Successor Agent**
   ```
   Emergency handoff. Previous agent went offline mid-task.
   
   Read these IN ORDER:
   1. .taskmaster/continuity/EMERGENCY_STATE.md
   2. .taskmaster/continuity/CONTEXT_PROMPT.md
   3. .taskmaster/continuity/QUICK_START.md
   
   Then continue from last checkpoint.
   ```

### New Agent's Actions:

1. Read EMERGENCY_STATE.md first
2. Check git status for uncommitted changes
3. Verify any partial work
4. Resume from last stable checkpoint
5. Update progress tracker

---

## HANDOFF QUALITY METRICS

### Good Handoff:
- New agent starts within 5 minutes
- No questions needed about context
- Continuation is seamless
- No duplicate work

### Bad Handoff:
- New agent confused about mission
- Asks "what am I supposed to do?"
- Can't find current phase
- Redoes completed work

**If handoff is bad:** Use this protocol again, more carefully.

---

## CONTINUOUS INTEGRATION

To enable **multiple agents working simultaneously** on different phases:

### File Locking Protocol:

```bash
# Before starting work on a phase
touch .taskmaster/locks/phase-N.lock
echo "$(date) - Agent $(whoami)" > .taskmaster/locks/phase-N.lock

# Check for locks before starting
if [ -f .taskmaster/locks/phase-N.lock ]; then
    echo "Phase N locked by: $(cat .taskmaster/locks/phase-N.lock)"
    exit 1
fi

# Release lock when done
rm .taskmaster/locks/phase-N.lock
```

### Phase Independence:
- Phase 1 can be done by Agent A
- Phase 2 can be done by Agent B simultaneously  
- Phase 3 requires Phase 1 complete
- Phase 4 requires Phase 2 complete

---

## CAPTAIN'S OVERRIDE

Captain can override any handoff protocol rule if necessary.

**Override command format:**
```
OVERRIDE: [Rule being overridden]
REASON: [Why override is necessary]
AUTHORITY: Captain SpectraSynq
DATE: YYYY-MM-DD
```

This override must be documented in:
- IMPLEMENTATION_BRIEF.md under "CURRENT PROGRESS TRACKER"
- Git commit message if applicable

---

## SUCCESS CRITERIA

Handoff is successful when:

1. ✓ New agent understands mission (comprehension check passed)
2. ✓ New agent knows current state (checked progress tracker)
3. ✓ New agent begins work within 10 minutes
4. ✓ No information loss (context complete)
5. ✓ No duplicate work (clear on what's done)

If all criteria met: **HANDOFF SUCCESSFUL** ✓

---

## APPENDIX: Common Handoff Failures

### Failure Mode 1: "I don't know what to do"
**Cause:** Agent didn't read CONTEXT_PROMPT.md  
**Fix:** Make agent read and acknowledge

### Failure Mode 2: "Where do I start?"
**Cause:** Agent didn't check progress tracker  
**Fix:** Show them grep command

### Failure Mode 3: "I redid work already done"
**Cause:** Progress tracker not updated  
**Fix:** Captain must update before handoff

### Failure Mode 4: "The documentation is too long"
**Cause:** Agent overwhelmed by IMPLEMENTATION_BRIEF.md  
**Fix:** Direct to QUICK_START.md first

### Failure Mode 5: "I don't have file access"
**Cause:** Agent environment not set up  
**Fix:** Verify file system access before mission start

---

**This protocol ensures the Knowledge Fortress mission NEVER fails due to handoff issues.**

*Protocol Version: 1.0*  
*Last Updated: 2025-10-15*  
*Maintained By: Captain SpectraSynq*
