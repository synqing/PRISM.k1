# PRISM.k1 PRD — Phase B: Program (IR) Runtime

Date: 2025-10-17
Owner: Captain
Status: Draft (pending Phase A completion)

## Goal
Compile node DAG to a compact intermediate representation (IR) executed per‑frame on device for “instrument mode.”

## Scope
- IR emitter on host (topo‑sorted ops, constant folding).
- Device IR executor (Program, Renderer), palette bank lookup, uniforms (time, macros).
- Transport: same TLV PUT_* frames but payload is IR blob.

## Non‑Goals
- Audio, multi‑device sync, persistent param automation.

## Acceptance
- Program blobs load and render at 120 FPS with ≤ 3.3 ms CPU per frame on S3.
- SET_PARAM updates apply < 1 frame for four global macros.

