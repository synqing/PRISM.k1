# PRISM Studio

Desktop pattern editor for PRISM K1 LED controller.

## Prerequisites

- Node.js 20+
- pnpm 9+
- Rust (stable)
- Tauri CLI: `cargo install tauri-cli`

### Platform-Specific

macOS:

```bash
xcode-select --install
```

Linux (Ubuntu/Debian):

```bash
sudo apt-get update
sudo apt-get install -y \
  libwebkit2gtk-4.1-dev \
  build-essential \
  curl \
  wget \
  file \
  libssl-dev \
  libayatana-appindicator3-dev \
  librsvg2-dev
```

Windows:
- Visual Studio 2022 with C++ tools
- WebView2 Runtime

## Quick Start

```bash
cd studio
pnpm install
pnpm tauri dev
```

## Scripts

```bash
pnpm dev           # Vite dev server
pnpm tauri dev     # Tauri dev (desktop)
pnpm build         # Vite build
pnpm tauri build   # Tauri build (bundles)
pnpm lint          # ESLint
pnpm lint:fix      # ESLint with fixes
pnpm format        # Prettier format
pnpm format:check  # Prettier check
pnpm type-check    # TypeScript
pnpm test          # Unit tests (Vitest)
pnpm test:e2e      # E2E tests (Playwright)
pnpm test:rust     # Rust tests (nextest)
```

## Project Structure

```
studio/
├── src/                  # React frontend
├── src-tauri/            # Rust backend (Tauri)
├── e2e/                  # Playwright tests
└── public/               # Static assets
```

