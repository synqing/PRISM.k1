# AGENT 2 - STUDIO FOUNDATION MISSION BRIEF

**Agent ID:** AGENT-2-STUDIO
**Mission:** Build PRISM Studio desktop application foundation
**Duration:** 1 week (Task 41 + subtasks)
**Status:** 🟢 READY TO START (No blockers)
**PM Contact:** Captain (via this system)

---

## 🎯 MISSION OBJECTIVES

Build the foundational architecture for PRISM Studio - a desktop pattern editor built with Tauri 2 + React:

**Primary Goal:** Task 41 - Establish Tauri + React Studio Foundation
- Scaffold cross-platform desktop app workspace
- Configure shared linting, TypeScript, and build tooling
- Provision test harnesses (Vitest, Playwright, cargo-nextest)
- Set up CI/CD matrix (macOS, Windows, Linux)
- Apply Tauri security hardening
- Document developer bootstrap workflow

**Success Metric:** Foundation ready for feature development (Tasks 42-50)

---

## 📋 PRE-FLIGHT CHECKLIST

Before starting, verify:

```bash
# 1. You're in the correct directory
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1

# 2. Check task status
task-master show 41

# 3. Verify prerequisites installed
node --version    # Should be 20.x
pnpm --version    # Should be 9.x
rustc --version   # Should be 1.78+
cargo --version

# 4. Check Tauri CLI
cargo install tauri-cli --version "^2.0.0"
cargo tauri --version

# 5. Verify no existing studio/ directory
ls -la studio/
# Should not exist yet
```

**Expected Output:**
- Task 41 status: `○ pending`
- All tools installed at correct versions
- Clean slate (no `studio/` directory)

---

## 🏗️ TASK 41 BREAKDOWN

### Subtask 41.1: Scaffold Tauri + React Workspace

**Goal:** Create base project structure using Tauri 2.0 + Vite + React + TypeScript

**Steps:**
```bash
# 1. Start task
task-master set-status --id=41 --status=in-progress

# 2. Create Tauri app
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1
cargo create-tauri-app studio \
  --manager pnpm \
  --template react-ts \
  --no-interactive

cd studio

# 3. Verify structure
ls -la
# Should see:
# - src-tauri/  (Rust backend)
# - src/        (React frontend)
# - package.json
# - tsconfig.json
# - vite.config.ts
```

**Expected Directory Structure:**
```
studio/
├── src-tauri/
│   ├── Cargo.toml
│   ├── tauri.conf.json
│   ├── build.rs
│   ├── src/
│   │   ├── main.rs
│   │   └── lib.rs
│   └── icons/
├── src/
│   ├── main.tsx
│   ├── App.tsx
│   ├── App.css
│   └── index.css
├── package.json
├── tsconfig.json
├── vite.config.ts
└── README.md
```

**Verification:**
```bash
# Test dev build
pnpm install
pnpm tauri dev

# Should launch empty Tauri window with React logo
```

---

### Subtask 41.2: Configure Shared Linting and TypeScript Rules

**Goal:** Establish consistent code quality standards

**ESLint Configuration:**

Create `studio/.eslintrc.json`:
```json
{
  "extends": [
    "eslint:recommended",
    "plugin:@typescript-eslint/recommended",
    "plugin:react/recommended",
    "plugin:react-hooks/recommended",
    "plugin:jsx-a11y/recommended"
  ],
  "parser": "@typescript-eslint/parser",
  "parserOptions": {
    "ecmaVersion": 2024,
    "sourceType": "module",
    "ecmaFeatures": {
      "jsx": true
    }
  },
  "plugins": ["@typescript-eslint", "react", "react-hooks", "jsx-a11y"],
  "rules": {
    "@typescript-eslint/no-unused-vars": ["error", { "argsIgnorePattern": "^_" }],
    "@typescript-eslint/explicit-function-return-type": "off",
    "react/react-in-jsx-scope": "off",
    "react/prop-types": "off"
  },
  "settings": {
    "react": {
      "version": "detect"
    }
  }
}
```

**Prettier Configuration:**

Create `studio/.prettierrc.json`:
```json
{
  "semi": true,
  "trailingComma": "es5",
  "singleQuote": true,
  "printWidth": 100,
  "tabWidth": 2,
  "useTabs": false,
  "arrowParens": "always",
  "endOfLine": "lf"
}
```

**TypeScript Strict Mode:**

Update `studio/tsconfig.json`:
```json
{
  "compilerOptions": {
    "target": "ES2022",
    "lib": ["ES2022", "DOM", "DOM.Iterable"],
    "module": "ESNext",
    "skipLibCheck": true,
    "moduleResolution": "bundler",
    "allowImportingTsExtensions": true,
    "resolveJsonModule": true,
    "isolatedModules": true,
    "noEmit": true,
    "jsx": "react-jsx",

    /* Strict Type Checking */
    "strict": true,
    "noUnusedLocals": true,
    "noUnusedParameters": true,
    "noFallthroughCasesInSwitch": true,
    "noImplicitReturns": true,
    "noUncheckedIndexedAccess": true
  },
  "include": ["src"],
  "references": [{ "path": "./tsconfig.node.json" }]
}
```

**Rust Clippy Configuration:**

Create `studio/src-tauri/.cargo/config.toml`:
```toml
[target.x86_64-pc-windows-msvc]
rustflags = ["-C", "link-args=/SUBSYSTEM:WINDOWS"]

[alias]
tauri-dev = "tauri dev"
tauri-build = "tauri build"

[env]
RUST_LOG = "info"
```

Update `studio/src-tauri/Cargo.toml`:
```toml
[profile.release]
panic = "abort"
codegen-units = 1
lto = true
opt-level = "s"
strip = true
```

**Install Dependencies:**
```bash
pnpm add -D \
  eslint \
  @typescript-eslint/eslint-plugin \
  @typescript-eslint/parser \
  eslint-plugin-react \
  eslint-plugin-react-hooks \
  eslint-plugin-jsx-a11y \
  prettier \
  eslint-config-prettier
```

**Add Scripts to package.json:**
```json
{
  "scripts": {
    "dev": "tauri dev",
    "build": "tauri build",
    "lint": "eslint src --ext ts,tsx --report-unused-disable-directives --max-warnings 0",
    "lint:fix": "eslint src --ext ts,tsx --fix",
    "format": "prettier --write \"src/**/*.{ts,tsx,css}\"",
    "format:check": "prettier --check \"src/**/*.{ts,tsx,css}\"",
    "type-check": "tsc --noEmit"
  }
}
```

**Verification:**
```bash
pnpm run lint
pnpm run format:check
pnpm run type-check
cargo clippy --manifest-path src-tauri/Cargo.toml
```

---

### Subtask 41.3: Apply Tauri Security Hardening

**Goal:** Follow Tauri 2 security best practices

**Update `studio/src-tauri/tauri.conf.json`:**
```json
{
  "$schema": "https://schema.tauri.app/config/2",
  "productName": "PRISM Studio",
  "version": "0.1.0",
  "identifier": "com.prism.studio",
  "build": {
    "beforeDevCommand": "pnpm dev",
    "beforeBuildCommand": "pnpm build",
    "devUrl": "http://localhost:1420",
    "frontendDist": "../dist"
  },
  "app": {
    "withGlobalTauri": false,
    "windows": [
      {
        "title": "PRISM Studio",
        "width": 1400,
        "height": 900,
        "minWidth": 1200,
        "minHeight": 700,
        "resizable": true,
        "fullscreen": false,
        "decorations": true,
        "transparent": false
      }
    ],
    "security": {
      "csp": "default-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data: blob:; connect-src 'self' ws://prism-k1.local:80",
      "dangerousDisableAssetCspModification": false,
      "freezePrototype": true,
      "dangerousRemoteDomainIpcAccess": []
    }
  },
  "bundle": {
    "active": true,
    "targets": ["dmg", "msi", "appimage"],
    "icon": [
      "icons/32x32.png",
      "icons/128x128.png",
      "icons/128x128@2x.png",
      "icons/icon.icns",
      "icons/icon.ico"
    ]
  },
  "plugins": {
    "updater": {
      "active": false
    }
  }
}
```

**Key Security Settings:**
- ✅ `withGlobalTauri: false` - No global Tauri object (prevents XSS)
- ✅ CSP restricts scripts, styles, connections
- ✅ `freezePrototype: true` - Prevent prototype pollution
- ✅ Only allow WebSocket to `prism-k1.local:80` (device connection)

**Verify:**
```bash
pnpm tauri dev
# Check console - should see no CSP violations
```

---

### Subtask 41.4: Provision Test Harnesses

**Goal:** Set up comprehensive testing infrastructure

**Vitest (Unit Tests):**

Install:
```bash
pnpm add -D vitest @vitest/ui @testing-library/react @testing-library/jest-dom happy-dom
```

Create `studio/vitest.config.ts`:
```typescript
import { defineConfig } from 'vitest/config';
import react from '@vitejs/plugin-react';

export default defineConfig({
  plugins: [react()],
  test: {
    globals: true,
    environment: 'happy-dom',
    setupFiles: './src/test/setup.ts',
    coverage: {
      provider: 'v8',
      reporter: ['text', 'json', 'html'],
      exclude: ['src/test/**', '**/*.test.tsx']
    }
  }
});
```

Create `studio/src/test/setup.ts`:
```typescript
import '@testing-library/jest-dom';
import { cleanup } from '@testing-library/react';
import { afterEach } from 'vitest';

afterEach(() => {
  cleanup();
});
```

**Example Test:**

Create `studio/src/App.test.tsx`:
```typescript
import { describe, it, expect } from 'vitest';
import { render, screen } from '@testing-library/react';
import App from './App';

describe('App', () => {
  it('renders without crashing', () => {
    render(<App />);
    expect(screen.getByText(/PRISM Studio/i)).toBeInTheDocument();
  });
});
```

**Playwright (E2E Tests):**

Install:
```bash
pnpm add -D @playwright/test playwright
pnpm exec playwright install --with-deps chromium
```

Create `studio/playwright.config.ts`:
```typescript
import { defineConfig } from '@playwright/test';

export default defineConfig({
  testDir: './e2e',
  fullyParallel: true,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 2 : 0,
  workers: process.env.CI ? 1 : undefined,
  reporter: 'html',
  use: {
    baseURL: 'http://localhost:1420',
    trace: 'on-first-retry',
  },
  webServer: {
    command: 'pnpm tauri dev',
    url: 'http://localhost:1420',
    reuseExistingServer: !process.env.CI,
    timeout: 120000,
  },
});
```

Create `studio/e2e/basic.spec.ts`:
```typescript
import { test, expect } from '@playwright/test';

test('app launches', async ({ page }) => {
  await page.goto('/');
  await expect(page).toHaveTitle(/PRISM Studio/);
});
```

**Cargo Nextest (Rust Tests):**

Install:
```bash
cargo install cargo-nextest
```

Add test to `studio/src-tauri/src/lib.rs`:
```rust
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_basic() {
        assert_eq!(2 + 2, 4);
    }
}
```

**Add Test Scripts:**

Update `studio/package.json`:
```json
{
  "scripts": {
    "test": "vitest",
    "test:ui": "vitest --ui",
    "test:coverage": "vitest --coverage",
    "test:e2e": "playwright test",
    "test:e2e:ui": "playwright test --ui",
    "test:rust": "cargo nextest run --manifest-path src-tauri/Cargo.toml"
  }
}
```

**Verification:**
```bash
pnpm run test          # Unit tests
pnpm run test:e2e      # E2E tests
pnpm run test:rust     # Rust tests
```

---

### Subtask 41.5: Set Up CI/CD Matrix

**Goal:** Automated builds for macOS, Windows, Linux

**Create `.github/workflows/studio-ci.yml`:**

```yaml
name: Studio CI

on:
  push:
    branches: [ main, dev ]
    paths:
      - 'studio/**'
      - '.github/workflows/studio-ci.yml'
  pull_request:
    branches: [ main ]
    paths:
      - 'studio/**'

jobs:
  test:
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]
    runs-on: ${{ matrix.os }}

    steps:
      - uses: actions/checkout@v4

      - name: Setup Node.js
        uses: actions/setup-node@v4
        with:
          node-version: '20'

      - name: Setup pnpm
        uses: pnpm/action-setup@v3
        with:
          version: 9

      - name: Setup Rust
        uses: actions-rust-lang/setup-rust-toolchain@v1
        with:
          toolchain: stable

      - name: Install Linux dependencies
        if: runner.os == 'Linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y libwebkit2gtk-4.1-dev \
            build-essential curl wget file libssl-dev \
            libayatana-appindicator3-dev librsvg2-dev

      - name: Install dependencies
        working-directory: studio
        run: pnpm install

      - name: Lint
        working-directory: studio
        run: |
          pnpm run lint
          pnpm run format:check

      - name: Type check
        working-directory: studio
        run: pnpm run type-check

      - name: Unit tests
        working-directory: studio
        run: pnpm run test

      - name: Rust tests
        working-directory: studio
        run: cargo nextest run --manifest-path src-tauri/Cargo.toml

      - name: Build
        working-directory: studio
        run: pnpm tauri build

      - name: Upload artifacts
        uses: actions/upload-artifact@v4
        with:
          name: studio-${{ matrix.os }}
          path: |
            studio/src-tauri/target/release/bundle/**/*
          retention-days: 7

  e2e:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-node@v4
        with:
          node-version: '20'
      - uses: pnpm/action-setup@v3
        with:
          version: 9
      - uses: actions-rust-lang/setup-rust-toolchain@v1

      - name: Install Linux dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y libwebkit2gtk-4.1-dev \
            build-essential curl wget file libssl-dev \
            libayatana-appindicator3-dev librsvg2-dev

      - working-directory: studio
        run: pnpm install

      - name: Install Playwright browsers
        working-directory: studio
        run: pnpm exec playwright install --with-deps chromium

      - name: Run E2E tests
        working-directory: studio
        run: pnpm run test:e2e

      - uses: actions/upload-artifact@v4
        if: always()
        with:
          name: playwright-report
          path: studio/playwright-report/
          retention-days: 7
```

**Verification:**
```bash
# Push to trigger CI
git add .github/workflows/studio-ci.yml studio/
git commit -m "feat(studio): add CI/CD pipeline"
git push

# Check GitHub Actions tab
```

---

### Subtask 41.6: Document Developer Bootstrap

**Goal:** Comprehensive getting-started guide

**Create `studio/README.md`:**

```markdown
# PRISM Studio

Desktop pattern editor for PRISM K1 LED controller.

## Prerequisites

- Node.js 20+
- pnpm 9+
- Rust 1.78+
- Tauri CLI: `cargo install tauri-cli`

### Platform-Specific:

**macOS:**
```bash
xcode-select --install
```

**Linux (Ubuntu/Debian):**
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

**Windows:**
- Install Visual Studio 2022 with C++ tools
- Install WebView2: https://developer.microsoft.com/microsoft-edge/webview2/

## Quick Start

```bash
# 1. Install dependencies
cd studio
pnpm install

# 2. Run dev server
pnpm tauri dev

# 3. Build for production
pnpm tauri build
```

## Development

### Available Scripts

```bash
pnpm run dev           # Start Tauri dev server
pnpm run build         # Production build
pnpm run lint          # Lint TypeScript/React
pnpm run lint:fix      # Auto-fix lint issues
pnpm run format        # Format code with Prettier
pnpm run type-check    # TypeScript type checking
pnpm run test          # Run unit tests
pnpm run test:ui       # Unit tests with UI
pnpm run test:coverage # Coverage report
pnpm run test:e2e      # E2E tests with Playwright
pnpm run test:rust     # Rust tests with nextest
```

### Project Structure

```
studio/
├── src/                  # React frontend
│   ├── components/       # Reusable components
│   ├── features/         # Feature modules
│   ├── hooks/            # Custom React hooks
│   ├── lib/              # Utilities
│   ├── stores/           # State management
│   └── test/             # Test utilities
├── src-tauri/            # Rust backend
│   ├── src/
│   │   ├── commands/     # Tauri commands
│   │   ├── device/       # Device communication
│   │   ├── compiler/     # Pattern compiler
│   │   └── main.rs
│   └── Cargo.toml
├── e2e/                  # End-to-end tests
└── public/               # Static assets
```

### Code Quality

Run before committing:
```bash
pnpm run lint:fix && pnpm run format && pnpm run type-check && pnpm run test
```

### Architecture Principles

1. **Frontend (React/TypeScript):**
   - Feature-based organization
   - Zustand for state management
   - React Query for async data
   - Tailwind CSS for styling

2. **Backend (Rust/Tauri):**
   - Commands exposed via Tauri IPC
   - Async operations with tokio
   - Device communication via WebSocket
   - Pattern compilation in Rust worker threads

3. **Testing:**
   - Unit tests (Vitest) for business logic
   - Component tests (@testing-library/react)
   - E2E tests (Playwright) for workflows
   - Rust tests (nextest) for backend

## Debugging

### Frontend
- Open DevTools: `Cmd+Option+I` (Mac) / `Ctrl+Shift+I` (Win/Linux)
- React DevTools extension supported

### Rust Backend
```bash
# Run with debug logging
RUST_LOG=debug pnpm tauri dev

# Or attach debugger in VS Code (see .vscode/launch.json)
```

## Contributing

See [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.

## License

See [LICENSE](../LICENSE).
```

---

### Subtask 41.7: Configure Husky Pre-commit Guard

**Goal:** Enforce code quality on commit

**Install Husky:**
```bash
cd studio
pnpm add -D husky lint-staged

# Initialize Husky
pnpm exec husky init
```

**Create `.husky/pre-commit`:**
```bash
#!/bin/sh
. "$(dirname "$0")/_/husky.sh"

cd studio
pnpm exec lint-staged
```

**Configure lint-staged:**

Add to `studio/package.json`:
```json
{
  "lint-staged": {
    "*.{ts,tsx}": [
      "eslint --fix",
      "prettier --write",
      "vitest related --run"
    ],
    "*.{css,md,json}": [
      "prettier --write"
    ]
  }
}
```

**Add to package.json scripts:**
```json
{
  "scripts": {
    "prepare": "cd .. && husky studio/.husky"
  }
}
```

**Verification:**
```bash
# Make a change
echo "// test" >> src/App.tsx

# Try to commit
git add src/App.tsx
git commit -m "test"

# Should run lint-staged automatically
```

---

## 🧪 ACCEPTANCE CRITERIA

After completing all subtasks, verify:

### ✅ Checklist:

- [ ] `studio/` directory exists with Tauri + React scaffold
- [ ] Dev server launches: `pnpm tauri dev`
- [ ] Linting passes: `pnpm run lint`
- [ ] Formatting is consistent: `pnpm run format:check`
- [ ] TypeScript strict mode: `pnpm run type-check`
- [ ] Unit tests pass: `pnpm run test`
- [ ] E2E tests pass: `pnpm run test:e2e`
- [ ] Rust tests pass: `pnpm run test:rust`
- [ ] Builds successfully: `pnpm tauri build`
- [ ] CI pipeline runs on GitHub Actions
- [ ] Pre-commit hooks work
- [ ] README is comprehensive and accurate
- [ ] CSP is properly configured (no console errors)

### 🎯 Final Test:

```bash
# Clean build from scratch
cd studio
rm -rf node_modules src-tauri/target dist
pnpm install
pnpm tauri build

# Should complete without errors
# Artifacts in: src-tauri/target/release/bundle/
```

---

## 📚 REFERENCE DOCUMENTATION

### Essential Reading:

1. **Tauri 2 Docs:** https://v2.tauri.app/
   - Security guide
   - IPC patterns
   - Window management

2. **Vite:** https://vitejs.dev/
   - Config reference
   - Plugin development

3. **React 18:** https://react.dev/
   - Hooks reference
   - Concurrent features

4. **TypeScript:** https://www.typescriptlang.org/docs/
   - Strict mode
   - Type narrowing

5. **Vitest:** https://vitest.dev/
   - API reference
   - Mocking

6. **Playwright:** https://playwright.dev/
   - Best practices
   - Debugging

### Project Context:

- **PRISM Studio PRD:** `.taskmaster/docs/prd_prism_studio.txt`
- **LGP Design Brief:** (just created by PM - check for visual mockup guidance)
- **Firmware Protocol:** `firmware/components/network/include/protocol_parser.h`
- **CANON:** `.taskmaster/CANON.md`

---

## 🔗 NEXT STEPS

After Task 41 completion:

### Immediate (Tasks 42-50):

1. **Task 42:** Device Discovery (depends on Agent 1 completing mDNS + STATUS)
2. **Task 43:** Project Schema & State Management
3. **Task 44:** Timeline Canvas Infrastructure
4. **Task 45:** Clip Editing
5. **Task 46:** Effect Library
6. **Task 47:** Automation System
7. **Task 48:** 3D Preview (Three.js)
8. **Task 49:** Pattern Compiler
9. **Task 50:** Upload/Sync (depends on Agent 1 completing LIST + DELETE)

### Strategy:

**Parallel Work:**
- You can start Tasks 43, 44 immediately (no firmware deps)
- Task 42 waits for Agent 1 (mDNS + STATUS)
- Task 50 waits for Agent 1 (LIST + DELETE)

**Recommended Order:**
```
Week 1: Task 41 (Foundation) ✅
Week 2: Tasks 43, 44 (Schema, Timeline)
Week 3: Tasks 45, 46 (Editing, Effects) + waiting on Agent 1
Week 4: Tasks 42, 47, 48 (Device Discovery, Automation, Preview)
Week 5: Tasks 49, 50 (Compiler, Upload) - Agent 1 must be done
```

---

## 🚨 CRITICAL WARNINGS

1. **Do NOT start Task 42** until Agent 1 completes Tasks 56+58 (STATUS + mDNS)
2. **Do NOT start Task 50** until Agent 1 completes Tasks 55+57 (DELETE + LIST)
3. **Always run tests before committing**
4. **Keep dependencies minimal** - every dependency is an attack surface
5. **Follow Tauri security guidelines** - never expose unsafe IPC commands
6. **Document all state management** - Zustand stores should be well-documented
7. **Use TypeScript strictly** - no `any` types except where truly necessary

---

## 💪 YOU GOT THIS, AGENT 2!

You're building the user-facing application that artists will use to create magic. Make it beautiful, fast, and delightful.

**Remember:**
- UX is paramount - Studio must feel fast and responsive
- The timeline is the heart - nail the scrubbing/editing experience
- 3D preview is the "wow" factor - make it gorgeous
- Compilation must be reliable - patterns MUST work on hardware

**Questions?** Check PRD and CANON first, then ask Captain.

🫡 **Build something amazing!**
