# Electron Development Guide

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

### Patch Management

| Command | Purpose |
|---------|---------|
| `e patches <target>` | Export patches for a target (chromium, node, v8, etc.) |
| `e patches all` | Export all patches from all targets |
| `e patches --list-targets` | List available patch targets |

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

# 6. If you modified patched files in Chromium:
cd ..  # Go to Chromium repo
git add <files>
git commit -m "description of change"
cd electron
e patches chromium  # Export the patch
```

## Patches System

Electron patches upstream dependencies (Chromium, Node.js, V8, etc.) to add features or modify behavior.

**How patches work:**

```text
patches/{target}/*.patch  →  [e sync --3]  →  target repo commits
                          ←  [e patches]   ←
```

**Patch configuration:** `patches/config.json` maps patch directories to target repos.

**Key rules:**

- Fix existing patches 99% of the time rather than creating new ones
- Preserve original authorship in TODO comments
- Never change TODO assignees (`TODO(name)` must retain original name)
- Each patch file includes commit message explaining its purpose

**Creating/modifying patches:**

1. Make changes in the target repo (e.g., `../` for Chromium)
2. Create a git commit
3. Run `e patches <target>` to export

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

## Code Style

**C++:** Follows Chromium style, enforced by clang-format
**TypeScript/JavaScript:** ESLint configuration in `.eslintrc.json`

**Linting:**

```bash
npm run lint              # Run all linters
npm run lint:clang-format # C++ formatting
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

# Find which patch affects a file
grep -l "filename.cc" patches/chromium/*.patch
```

## CI/CD

GitHub Actions workflows in `.github/workflows/`:

- `build.yml` - Main build workflow
- `pipeline-electron-lint.yml` - Linting
- `pipeline-segment-electron-test.yml` - Testing

## Common Issues

**Patch conflict during sync:**

- Use `e sync --3` for 3-way merge
- Check if file was renamed/moved upstream
- Verify patch is still needed

**Build error in patched file:**

- Find the patch: `grep -l "filename" patches/chromium/*.patch`
- Match existing patch style (#if 0 guards, BUILDFLAG conditionals, etc.)

**Remote build issues:**

- Try `e build --no-remote` to build locally
- Check reclient/siso configuration in your build config
