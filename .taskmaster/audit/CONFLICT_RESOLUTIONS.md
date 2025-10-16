# Conflict Resolutions - Captain's Decisions

**Date:** 2025-10-15
**Decision Maker:** Captain SpectraSynq
**Context:** PRD vs CANON cross-check revealed 3 conflicts

---

## ✅ **RESOLVED CONFLICTS**

### **Conflict 1: LED Count**

**PRD Value:** 150 LEDs (lines 113, 316)
**CANON Value:** 320 LEDs (ADR-003)

**Captain's Decision:** ✅ **320 LEDs**

**Rationale:** "There is only ever one version. 320."

**Actions Required:**
1. Update PRD line 113: `#define LED_COUNT 150` → `320`
2. Update PRD line 316: Test hardware reference to 150 pixels → 320 pixels
3. No change to CANON (already correct)

**Impact:**
- Frame buffer size: 450 bytes → 960 bytes (2.13x increase)
- Memory calculations throughout PRD need validation
- All LED-related tasks must use 320

---

### **Conflict 2: WebSocket Buffer Size**

**PRD Value:** 8KB (line 109)
**CANON Value:** 4KB (ADR-002)

**Captain's Decision:** ✅ **4KB (CANON)**

**Rationale:** CANON is evidence-based with fragmentation research showing 8KB degrades after 12 hours

**Actions Required:**
1. Update PRD line 109: `#define WS_BUFFER_SIZE 8192` → `4096`
2. No change to CANON (already correct)
3. Update any throughput calculations based on 4KB buffer

**Impact:**
- WebSocket throughput may be slightly lower
- But reliability significantly better (98% vs 85% success rate)

---

### **Conflict 3: Pattern Count**

**PRD Value:** 25-35 patterns (lines 16, 166)
**CANON Value:** 25 minimum (ADR-004 line 117)

**Captain's Decision:** ✅ **15 templates minimum**

**Rationale:** "No idea where the fuck 25-35 came from, but 15 is enough, we can expand it - if remaining memory permits"

**Actions Required:**
1. Update PRD line 16: "25-35 patterns" → "15+ templates"
2. Update PRD line 166: "25+ patterns fit" → "15+ templates fit"
3. Update CANON ADR-004: Change `pattern_min_count: 25` → `15`
4. Regenerate CANON after ADR-004 update

**Impact:**
- Reduces minimum storage requirement
- Allows more headroom per template
- 15 templates × ~100KB avg = 1.5MB (fits comfortably)

---

### **Clarification 4: Structural Efficiency / Compression**

**PRD Reference:** Line 32 mentions "structural efficiency: 40% size reduction"

**Captain's Context:** "The original design included a compression method (which ended up becoming one of the points of design mismatch)"

**Understanding:**
- Compression WAS part of original design
- But caused conflicts/drift between agents
- Need clarity on whether 256KB max is compressed or uncompressed

**Actions Required:**
1. Create ADR clarifying compression approach
2. Define if 256KB limit is pre-compression or post-compression
3. Document compression algorithm (if still using)
4. Or document why compression was removed

**Impact:**
- Affects storage calculations
- Affects upload validation
- Affects pattern format specification

---

## 📊 **STORAGE MATH (UPDATED)**

**With 15 templates minimum:**

```
Storage Available: 1.5MB = 1,572,864 bytes
Pattern Max Size: 256KB = 262,144 bytes (per ADR-004)
Templates Required: 15 minimum

Average size per template: 1.5MB ÷ 15 = 104,857 bytes (~102KB)

✅ 15 templates × 102KB avg = 1,530KB ≈ 1.5MB (FITS!)

Example distribution:
- 3 large templates @ 200KB = 600KB
- 7 medium templates @ 100KB = 700KB
- 5 small templates @ 40KB = 200KB
Total: 1,500KB ✅
```

**Conclusion:** Math works if 256KB is **maximum ceiling**, not typical size.

---

## 🎯 **NEXT STEPS**

### **Immediate (Today)**

1. ✅ **Document decisions** (this file)
2. **Update ADR-004** with correct pattern count (15 not 25)
3. **Create compression clarification ADR** (if needed)
4. **Regenerate CANON** from updated ADRs
5. **Update PRD** with corrected values (or note as deprecated)

### **High Priority (This Week)**

6. **Validate 56 deprecated tasks** against updated CANON
7. **Identify 5-10% drift** in tasks
8. **Validate template specifications** against CANON
9. **Create missing ADRs:**
   - ADR-006: WebSocket Protocol Specification
   - ADR-007: Template System Architecture
   - ADR-008: Performance Targets
   - ADR-009: Storage Implementation Details
   - ADR-010: Hardware Configuration

### **Medium Priority (Next Week)**

10. **Create clean task breakdown** with CANON references
11. **Set up autonomous multi-agent loop**
12. **Begin implementation**

---

## 🔄 **AUTHORITY UPDATE**

**Previous State:**
- PRD and CANON had conflicting specifications
- Unclear which was authoritative

**Current State:**
- ✅ **CANON is authoritative for technical specs**
- ✅ **PRD updated to match CANON (or deprecated)**
- ✅ **Single source of truth established**

**Process Going Forward:**
- Business requirements → PRD
- Technical decisions → ADRs → CANON
- When conflict → Research → ADR resolves it
- CANON always wins for implementation

---

## ✅ **VALIDATION CHECKLIST**

**Before proceeding with task validation:**

- [x] LED count decided (320)
- [x] WebSocket buffer decided (4KB)
- [x] Pattern count decided (15)
- [ ] Compression approach clarified (pending ADR)
- [ ] ADR-004 updated with pattern count
- [ ] CANON regenerated
- [ ] PRD updated (or deprecated)
- [ ] Cross-check validation complete

---

**Status:** CONFLICTS RESOLVED (3 of 3)
**Blocker Status:** UNBLOCKED - Can proceed with task validation
**Next Action:** Update ADR-004 and regenerate CANON

---

**Approved By:** Captain SpectraSynq
**Date:** 2025-10-15
