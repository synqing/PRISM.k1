# PRISM Studio – Tests, Timeouts, and CI

This doc explains the fast test/CI setup, the runtime timeouts exposed for the uploader, and when/why to use them.

## Fast Test & CI Flow

- Node/TypeScript
  - Type‑check: `pnpm -C studio run type-check`
  - Lint: `pnpm -C studio run lint`
  - Unit tests: `pnpm -C studio run test`

- Rust/Tauri (nextest)
  - Standard: `pnpm -C studio run test:rust`
  - Fast (short timeouts): `pnpm -C studio run test:rust:fast`

- Full fast suite (all of the above):
  - `pnpm -C studio run ci:fast`

CI runs the fast suite on every PR/push via `.github/workflows/ci.yml`.

## Uploader Timing Overrides (Env Vars)

The Tauri uploader (WebSocket TLV) can be tuned via environment variables for tests/dev. These reduce flakiness and prevent long hangs.

- `ACK_TIMEOUT_MS` (default `1500`)
  - Wait time for a server ack after `PUT_BEGIN` (and after a reconnect `PUT_BEGIN`).

- `RETRY_BASE_MS` (default `100`)
  - Base backoff for send failures (exponential for first few attempts). Lower in tests to retry quickly.

- `FINAL_ACK_TIMEOUT_MS` (default `500`)
  - Wait time for a final ack after `PUT_END` (best‑effort).

Example (fast timing):

```
ACK_TIMEOUT_MS=250 RETRY_BASE_MS=50 FINAL_ACK_TIMEOUT_MS=250 pnpm -C studio run test:rust
```

These are read in `studio/src-tauri/src/lib.rs`.

## Scripts (Convenience)

In `studio/package.json`:

- `test:rust:fast`
  - Runs nextest with fast timing env.
- `ci:fast`
  - Runs type‑check, lint, Vitest, and fast Rust tests.
- `tauri:dev:fast` (optional, dev only)
  - Launches Tauri with fast timing env for development.

## Feature Gating & Modes

- Publish Mode: Clip vs Program
  - UI supports both; Program compiles an IR scaffold but currently falls back to Clip upload until the firmware VM is ready.
  - Toggle is persisted; no risk when Program is selected—fallback keeps workflows working.

- Palette Pipeline: Host LUT vs Device Blend
  - Default is host LUT (always supported). If `STATUS.caps` includes the palette bit, UI enables device blend and disables palette header embedding.
  - Device blend reduces payload sizes and enables on‑device transitions.

## When to Use What

- Local dev/CI: use fast timing (`ACK_TIMEOUT_MS=250`, `RETRY_BASE_MS=50`, `FINAL_ACK_TIMEOUT_MS=250`).
- Production builds: keep defaults unless your network is extremely slow.
- Program mode: safe to keep visible; users remain on Clip upload until the firmware VM lands.
- Palette blend: leave caps‑gated; users get best behavior automatically when devices support it.

