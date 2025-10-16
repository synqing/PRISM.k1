# CONTEXT PROMPT FOR SUCCESSOR AGENTS
## Feed this entire document to any LLM to enable instant continuation

```
You are taking over implementation of the "PRISM K1 Knowledge Fortress" - a critical system to prevent specification drift that nearly destroyed a firmware project.

MISSION CONTEXT:
The PRISM K1 project (ESP32-S3 LED controller firmware) suffered from three agents creating three conflicting "source of truth" documents during planning. This caused:
- WebSocket buffer size conflicts (8192 vs 4096)
- LED count conflicts (150 vs 320)
- Partition table conflicts (different layouts)
- Storage path conflicts (/littlefs vs /prism)

Result: Nobody knew which specifications were correct. Development was blocked.

YOUR MISSION:
Implement a military-grade knowledge management system with:

1. CANON.md - Single auto-generated source of truth (NEVER manually edited)
2. ADRs (Architecture Decision Records) - Immutable decisions with evidence and rationale
3. Research validation framework - RFC-style peer review before research becomes truth
4. Automated enforcement - CI/CD prevents code contradicting CANON
5. Audit trail - Every decision timestamped and signed

IMPLEMENTATION PHASES:
- Phase 1 (2-4 hours): Emergency triage - Create 5 ADRs, fix code conflicts, generate CANON
- Phase 2 (1 day): Research reorganization - Classify and validate existing research
- Phase 3 (2-3 days): Automation - Scripts, git hooks, CI/CD, code generation
- Phase 4 (2 days): Documentation and training

PROJECT LOCATION:
/Users/spectrasynq/Workspace_Management/Software/PRISM.k1/

CRITICAL FILES:
- .taskmaster/continuity/IMPLEMENTATION_BRIEF.md (20+ pages, full details)
- .taskmaster/continuity/QUICK_START.md (3 pages, quick reference)
- .taskmaster/CANON.md (to be created - the single source of truth)
- .taskmaster/decisions/ (ADRs go here)
- firmware/sdkconfig.defaults (needs WS_BUFFER_SIZE fix: 8192→4096)
- firmware/partitions.csv (needs offset fix: 0x320000→0x311000)

CURRENT STATE:
Check IMPLEMENTATION_BRIEF.md section "CURRENT PROGRESS TRACKER" to see what's done.
Start at first incomplete phase.

YOUR FIRST ACTIONS:
1. Read QUICK_START.md (3 minutes)
2. Check progress tracker in IMPLEMENTATION_BRIEF.md
3. Create directory structure if missing:
   mkdir -p .taskmaster/{decisions,audit,specs/{firmware,protocol,format}}
   mkdir -p .taskmaster/research/{[PROPOSED],[IN_REVIEW],[VALIDATED],[REJECTED],[ARCHIVED]}
   mkdir -p .taskmaster/scripts
4. Begin implementation at your phase

CRITICAL RULES:
- NEVER edit CANON.md manually (auto-generated only)
- ALWAYS create ADR before changing specs
- ALWAYS get Captain SpectraSynq approval for ADRs
- NEVER leave conflicts unresolved
- ALWAYS update progress tracker as you work

RESOURCES AVAILABLE TO YOU:
- Complete ADR template in IMPLEMENTATION_BRIEF.md
- Example ADRs in IMPLEMENTATION_BRIEF.md
- All script specifications in IMPLEMENTATION_BRIEF.md
- Peer review process documented
- Full system architecture documented

SUCCESS METRICS:
You'll know it's working when:
1. Zero ambiguity - one answer to every spec question
2. Zero drift - code always matches CANON (automated)
3. Zero conflicts - impossible to have competing specs
4. Full traceability - every constant traces to ADR
5. Immutable history - every decision auditable

ESCALATION:
If stuck, ask Captain SpectraSynq for:
- ADR approval
- Conflict resolution decisions
- Authority to proceed

YOUR CAPABILITIES NEEDED:
- Read/write markdown files
- Execute bash scripts (or translate to your environment)
- Edit YAML/JSON
- Create directory structures
- Parse and validate file formats

START COMMAND:
Read this first: /Users/spectrasynq/Workspace_Management/Software/PRISM.k1/.taskmaster/continuity/QUICK_START.md

Then read full details: /Users/spectrasynq/Workspace_Management/Software/PRISM.k1/.taskmaster/continuity/IMPLEMENTATION_BRIEF.md

Then check progress: grep -A 40 "CURRENT PROGRESS TRACKER" IMPLEMENTATION_BRIEF.md

Then begin at your incomplete phase.

REMEMBER:
This system prevents the crisis from ever happening again. Every decision is:
1. Evidence-based (research)
2. Peer-reviewed (validation)
3. Explicitly decided (ADR)
4. Auto-enforced (CANON + CI/CD)
5. Fully auditable (immutable trail)

You have complete documentation. The design is solid. The path is clear.

Begin.
```

---

## HOW TO USE THIS PROMPT

**For a new agent taking over:**

1. Copy everything in the code block above
2. Paste into your conversation with the agent
3. Agent will have full context instantly
4. Agent can immediately continue from last checkpoint

**For Captain:**

Keep this file updated with:
- Current phase completion status
- Any blockers encountered
- Any deviations from plan
- Latest progress checkpoint

**For documentation:**

This prompt is the "executive summary" that enables zero-downtime handoffs between agents.
