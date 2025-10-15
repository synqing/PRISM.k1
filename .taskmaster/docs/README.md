# PRISM K1 Documentation Index - SOURCE OF TRUTH
*Last Updated: 2025-10-15*

## 📌 Document Hierarchy & Authority

This README serves as the **SOURCE OF TRUTH** for PRISM K1 documentation. All technical decisions must reference these documents in priority order.

### 🔴 PRIMARY AUTHORITATIVE DOCUMENTS (Use These First)

1. **[PRISM_AUTHORITATIVE_SPECIFICATION.md](./PRISM_AUTHORITATIVE_SPECIFICATION.md)** ⭐
   - **Status:** FINAL - Based on forensic evidence analysis
   - **Purpose:** Reconciled technical specification after conflict resolution
   - **Authority:** Supersedes all conflicting specifications
   - **Contents:** Partition table, filesystem, protocols, memory allocation
   - **Use For:** ALL implementation decisions

2. **[prism-firmware-prd.txt](./prism-firmware-prd.txt)**
   - **Status:** ORIGINAL REQUIREMENTS
   - **Purpose:** Product requirements from stakeholders
   - **Authority:** Source of business requirements
   - **Contents:** Features, success criteria, milestones
   - **Use For:** Understanding "what" and "why"

### 🟡 RESEARCH & ANALYSIS DOCUMENTS

3. **[research/forensic_specification_analysis.md](./research/forensic_specification_analysis.md)** 📊
   - **Status:** EVIDENCE TRAIL
   - **Purpose:** Documents the forensic investigation that led to authoritative spec
   - **Authority:** Explains WHY technical decisions were made
   - **Contents:** Evidence analysis, conflict resolution, decision rationale
   - **Use For:** Understanding technical choices

4. **[research/esp32_constraints_research.md](./research/esp32_constraints_research.md)**
   - **Status:** HARDWARE FACTS
   - **Purpose:** Measured ESP32-S3 constraints and capabilities
   - **Authority:** Hardware truth based on testing
   - **Contents:** Memory limits, fragmentation data, performance metrics
   - **Use For:** Hardware capability reference

5. **[research/document_conflict_analysis.md](./research/document_conflict_analysis.md)**
   - **Status:** CONFLICT REPORT
   - **Purpose:** Identifies all specification conflicts
   - **Authority:** Historical record of issues found
   - **Contents:** Conflict matrix, overlap analysis
   - **Use For:** Understanding what was reconciled

### 🟢 IMPLEMENTATION DOCUMENTS

6. **[firmware_architecture.md](./firmware_architecture.md)** ⚠️
   - **Status:** PARTIALLY SUPERSEDED
   - **Purpose:** Detailed module architecture
   - **Authority:** Use ONLY sections not contradicted by AUTHORITATIVE_SPEC
   - **Contents:** Module definitions, state machines, memory details
   - **Note:** Mount paths and some details need updating per authoritative spec

7. **[template_specifications.md](./template_specifications.md)**
   - **Status:** ACTIVE
   - **Purpose:** 15 template pattern specifications
   - **Authority:** Complete and accurate
   - **Contents:** Pattern algorithms, parameters, visual representations
   - **Use For:** Template implementation

8. **[websocket_protocol.md](./websocket_protocol.md)** ⚠️
   - **Status:** USE BINARY VERSION
   - **Purpose:** WebSocket communication protocol
   - **Authority:** Binary TLV version is correct (not JSON)
   - **Contents:** Message formats, protocol details
   - **Note:** Ignore any JSON protocol references

### ⚫ DEPRECATED DOCUMENTS (Do Not Use)

9. ~~**storage_layout.md**~~ ❌
   - **Status:** DEPRECATED - Conflicts with authoritative spec
   - **Issues:** Wrong partition layout (no OTA), some paths incorrect
   - **Replacement:** Use PRISM_AUTHORITATIVE_SPECIFICATION.md

10. ~~**template_catalog.md**~~ ❌
    - **Status:** DEPRECATED - Wrong template count
    - **Issues:** Has 20 templates instead of 15
    - **Replacement:** Use template_specifications.md

### 📋 QUICK DECISION REFERENCE

| Topic | Authoritative Source | Key Decision |
|-------|---------------------|--------------|
| **Partition Table** | PRISM_AUTHORITATIVE_SPEC §1.1 | Use OTA layout with 1.5MB storage |
| **Filesystem Mount** | PRISM_AUTHORITATIVE_SPEC §2.1 | `/littlefs` not `/prism` |
| **WebSocket Protocol** | websocket_protocol.md (binary) | Binary TLV, not JSON |
| **Storage Size** | PRISM_AUTHORITATIVE_SPEC §2.3 | 1.5MB (0x180000) |
| **Template Count** | PRISM_AUTHORITATIVE_SPEC §5.1 | Exactly 15 templates |
| **Template Storage** | PRISM_AUTHORITATIVE_SPEC §5.3 | Embedded in firmware |
| **Binary Magic** | PRISM_AUTHORITATIVE_SPEC §4.1 | 'PRSM' (0x5053524D) |
| **Memory Budget** | PRISM_AUTHORITATIVE_SPEC §6.1 | 200KB heap available |
| **Error Codes** | PRISM_AUTHORITATIVE_SPEC §8 | 0x01xx storage, 0x02xx network |
| **LED Count** | PRISM_AUTHORITATIVE_SPEC §7 | 320 LEDs |

## 🔍 How to Use This Documentation

### For Implementation:
1. **START** with [PRISM_AUTHORITATIVE_SPECIFICATION.md](./PRISM_AUTHORITATIVE_SPECIFICATION.md)
2. **CHECK** the Quick Decision Reference above
3. **VERIFY** against [prism-firmware-prd.txt](./prism-firmware-prd.txt) for requirements
4. **UNDERSTAND** decisions via [forensic_specification_analysis.md](./research/forensic_specification_analysis.md)

### For Debugging Conflicts:
1. **READ** [document_conflict_analysis.md](./research/document_conflict_analysis.md)
2. **REFER** to [forensic_specification_analysis.md](./research/forensic_specification_analysis.md)
3. **USE** only the authoritative specification for resolution

### For New Features:
1. **ENSURE** compatibility with authoritative spec
2. **UPDATE** this README if adding new documents
3. **DOCUMENT** decisions in research folder

## ⚠️ CRITICAL WARNINGS

1. **DO NOT** use deprecated documents (storage_layout.md, template_catalog.md)
2. **DO NOT** use JSON WebSocket protocol - use Binary TLV
3. **DO NOT** use `/prism` mount point - use `/littlefs`
4. **DO NOT** store templates as files - embed in firmware
5. **DO NOT** use 1MB storage size - use 1.5MB

## 📝 Document Maintenance

### Adding New Documents:
- Place in appropriate subfolder
- Update this README immediately
- Specify authority level and purpose
- Note any documents it supersedes

### Updating Existing Documents:
- Mark sections as superseded if replaced
- Update this README with new status
- Document reason for change in research/

### Resolving New Conflicts:
1. Conduct forensic analysis
2. Document evidence in research/
3. Update PRISM_AUTHORITATIVE_SPECIFICATION.md
4. Update this README

## 🎯 Key Takeaway

**When in doubt, use [PRISM_AUTHORITATIVE_SPECIFICATION.md](./PRISM_AUTHORITATIVE_SPECIFICATION.md)**

This document represents the final, evidence-based technical specification that reconciles all conflicts. It is the single source of truth for all technical implementation decisions.

---

*This index is maintained as the master documentation guide for PRISM K1 firmware development.*