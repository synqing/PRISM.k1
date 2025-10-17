# Studio PRs (Tauri/React)

## PR #1 — Pattern Builder (Node UI) + ToK1
- Add a new route: `studio/src/routes/PatternBuilder.tsx`
- Add node library: `studio/src/lib/graph/library.ts`
- Add types: `studio/src/lib/graph/types.ts` (matching MemoryDump)
- Add publish schemas: `studio/src/lib/publish/tok1.ts`
- Wire "Publish" split button (Clip vs Program).

## PR #2 — Compile (Bake & IR)
- `studio/src/lib/graph/compile-bake.ts` → render N frames then pack `.prism`
- `studio/src/lib/graph/compile-ir.ts` → graph → IR bytes (v0.1)

## PR #3 — Tauri device commands
- `src-tauri/src/commands/device_upload.rs`
- `src-tauri/src/commands/device_play.rs`
- `src-tauri/src/commands/device_stop.rs`
- `src-tauri/src/commands/device_set_param.rs`
- Register in `src-tauri/src/lib.rs`

## PR #4 — Device Panel actions
- Add Upload/Play/Stop to device table.
