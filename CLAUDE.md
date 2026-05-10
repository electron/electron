# Electron Development Guide

## Running node_modules binaries

**Never use `npx`.** It is considered dangerous because it can silently fetch and execute arbitrary packages from the registry. Always run binaries through one of these safer mechanisms instead:

1. **Preferred** — spawn the executable directly from `node_modules/.bin/<tool>` (or the platform equivalent on Windows). This is what `script/lint.js` does for `oxlint`.
2. **Acceptable** — invoke via `yarn <tool>` or `yarn run <tool>`, which resolves to the locally installed version without the registry fallback that `npx` performs.

This rule applies to shell commands you run yourself and to any scripts you author or modify in this repo.

## Project Overview

Electron is a framework for building cross-platform desktop applications using web technologies. It embeds Chromium for rendering and Node.js for backend functionality.

## Directory Structure

```text
electron/                 # This repo (run `e` commands here)
├── shell/               # Core C++ application code
│   ├── browser/         # Main process implementation (107+ API modules)
│   ├── renderer/        # Renderer process code
│   ├── common/          # Shared code between processes
│   ├── app/             # Application entry points
│   └── services/        # Node.js service integration
├── lib/                 # TypeScript/JavaScript library code
│   ├── browser/         # Main process JS (47 API implementations)
│   ├── renderer/        # Renderer process JS
│   └── common/          # Shared JS modules
├── patches/             # Patches for upstream dependencies
│   ├── chromium/        # ~159 patches to Chromium
│   ├── node/            # ~48 patches to Node.js
│   └── ...              # Other targets (v8, boringssl, etc.)
├── spec/                # Test suite (1189+ TypeScript test files)
├── docs/                # API documentation and guides
├── build/               # Build configuration
├── script/              # Build and automation scripts
└── chromium_src/        # Chromium source overrides
../                      # Parent directory is Chromium source
```

## Build Tools Setup

Electron uses `@electron/build-tools` for development. The `e` command is the primary CLI.

**Installation:**

```bash
npm i -g @electron/build-tools
```

**Configuration location:** `~/.electron_build_tools/configs/`

## Essential Commands

### Configuration Management

| Command | Purpose |
|---------|---------|
| `e init <name> --root=<path> --bootstrap testing` | Create new build config and sync |
| `e use <name>` | Switch to a different build configuration |
| `e show current` | Display active configuration name |
| `e show configs` | List all available configurations |

### Build & Development Loop

| Command | Purpose |
|---------|---------|
| `e sync` | Fetch/update all source code and apply patches |
| `e sync --3` | Sync with 3-way merge (required for Chromium upgrades) |
| `e build` | Build Electron (runs GN + Ninja) |
| `e build -k 999` | Build and continue on errors (up to 999) |
| `e build -t <target>` | Build specific target (e.g., `electron:node_headers`) |
| `e start` | Run the built Electron executable |
| `e start --version` | Verify Electron launches and print version |
| `e test` | Run the test suite |
| `e debug` | Run Electron in debugger (lldb on macOS, gdb on Linux) |

## Typical Development Workflow

```bash
# 1. Ensure you're on the right config
e show current

# 2. Sync to get latest code
e sync

# 3. Make your changes in shell/ or lib/ or ../

# 4. Build
e build

# 5. Test your changes (Leave the user to do this, don't run these commands unless asked)
e start
e test

# 6. If you modified patched files, activate the "Patches" skill
```

## Patches System

When working with patches (creating, modifying, fixing conflicts), activate the "Patches" skill.

## Testing

**Test location:** `spec/` directory

**Running tests:**

```bash
e test                    # Run full test suite
```

**Test frameworks:** Mocha, Chai, Sinon

## Build Configuration

**GN build arguments:** Located in `build/args/`:

- `testing.gn` - Debug/testing builds
- `release.gn` - Release builds
- `all.gn` - Common arguments for all builds

**Main build file:** `BUILD.gn`

**Feature flags:** `buildflags/buildflags.gni`

## Chromium Upgrade Workflow

When working on the `roller/chromium/main` branch to upgrade Chromium activate the "Electron Chromium Upgrade" skill.

## Node.js Upgrade Workflow

When working on the `roller/node/main` branch to upgrade Node.js activate the "Electron Node.js Upgrade" skill.

## Pull Requests

PR bodies must always include a `Notes:` section as the **last line** of the body. This is a consumer-facing release note for Electron app developers — describe the user-visible fix or change, not internal implementation details. Use `Notes: none` if there is no user-facing change.

When creating PRs, activate the "PR Labeling" skill.

## Code Style

**C++:** Follows Chromium style, enforced by clang-format
**TypeScript/JavaScript:** [oxlint](https://oxc.rs/docs/guide/usage/linter) configuration in `.oxlintrc.json`

**Linting:**

```bash
npm run lint              # Run all linters
npm run lint:js           # Run oxlint over all JS/TS/MJS sources
npm run lint:clang-format # C++ formatting
npm run lint:api-history  # Validate API history YAML blocks in docs
```

## Key Files

| File | Purpose |
|------|---------|
| `BUILD.gn` | Main GN build configuration |
| `DEPS` | Dependency versions and checkout paths |
| `patches/config.json` | Patch target configuration |
| `filenames.gni` | Source file lists by platform |
| `package.json` | Node.js dependencies and scripts |

## Environment Variables

| Variable | Purpose |
|----------|---------|
| `GN_EXTRA_ARGS` | Additional GN arguments (useful in CI) |
| `ELECTRON_RUN_AS_NODE=1` | Run Electron as Node.js |

## Useful Git Commands for Chromium

```bash
# Find CL that changed a file
cd ..
git log --oneline -10 -- {file}
git blame -L {start},{end} -- {file}

# Look for Chromium CL reference in commit
git log -1 {commit_sha}  # Find "Reviewed-on:" line
```

## CI/CD

GitHub Actions workflows in `.github/workflows/`:

- `build.yml` - Main build workflow
- `pipeline-electron-lint.yml` - Linting
- `pipeline-segment-electron-test.yml` - Testing

## Common Issues

**Remote build issues:**

- Try `e build --no-remote` to build locally
- Check reclient/siso configuration in your build config
