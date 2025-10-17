# Network Component (WiFi, Portal, Metrics)

This component manages WiFi (AP+STA), captive portal HTTP server, WebSocket handling, and developer metrics export.

## Captive Portal

- Starts an HTTP server in AP‑portal mode.
- Serves a simple credential page at `/` and accepts `POST /connect` to save SSID/password to NVS.

## Developer Metrics Export (Profiling)

When profiling and metrics exposure are enabled via Kconfig (Components → PRISM Playback), the portal exposes the following endpoints:

- `/metrics/wave` — JSON snapshot
- `/metrics` — Prometheus text exposition (when enabled)
- `/metrics.csv` — CSV snapshot (when enabled)

JSON example (abbreviated):

```json
{
  "samples": 120,
  "cycles": { "min": 330, "max": 610, "avg": 355 },
  "dcache": { "hits": 123456, "misses": 2345, "hit_rate": 98 },
  "icache": { "hits": 456789, "misses": 123, "hit_rate": 99 },
  "insn": { "count": 1234567, "ipc_x100": 87 }
}
```

CLI: `prism_metrics` prints the same snapshot (if enabled).

Optional Push
- Enable `PRISM_METRICS_PUSH` and configure `PRISM_METRICS_PUSH_URL` + `PRISM_METRICS_PUSH_INTERVAL_SEC` to periodically POST JSON snapshots.

## Kconfig Switches

- `PRISM_PROFILE_TEMPORAL` — master profiling toggle
- `PRISM_METRICS_HTTP` — enable HTTP endpoints
- `PRISM_METRICS_PROMETHEUS` — add `/metrics` endpoint
- `PRISM_METRICS_CSV` — add `/metrics.csv` endpoint
- `PRISM_METRICS_CLI` — register CLI command
- `PRISM_METRICS_PUSH` — push JSON snapshots (plus URL + interval)

## cURL Examples

- JSON:
  - `curl http://<device-ip>:<port>/metrics/wave`
- Prometheus:
  - `curl http://<device-ip>:<port>/metrics`
- CSV:
  - `curl http://<device-ip>:<port>/metrics.csv`

## Production Guidance

- These diagnostics are intended for development/soak tests. Disable all metrics exposure/push in production builds.

