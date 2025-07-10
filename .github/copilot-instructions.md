# Copilot Instructions for Electron

## Build System

Electron uses `@electron/build-tools` (`e` CLI). Install with `npm i -g @electron/build-tools`.

```bash
e sync              # Fetch sources and apply patches
e build             # Build Electron (GN + Ninja)
e build -k 999      # Build, continuing through errors
e start             # Run built Electron
e start --version   # Verify Electron launches
e test              # Run full test suite
e debug             # Run in debugger (lldb on macOS, gdb on Linux)
```

### Linting

```bash
npm run lint              # Run all linters (JS, C++, Python, GN, docs)
npm run lint:js           # JavaScript/TypeScript only
npm run lint:clang-format # C++ formatting only
npm run lint:cpp          # C++ linting only
npm run lint:docs         # Documentation only
```

### Running a Single Test

```bash
npm run test -- -g "pattern"   # Run tests matching a regex pattern
# Example: npm run test -- -g "ipc"
```

### Running a Single Node.js Test

```bash
node script/node-spec-runner.js parallel/test-crypto-keygen
```

## Architecture

Electron embeds Chromium (rendering) and Node.js (backend) to enable desktop apps with web technologies. The parent directory (`../`) is the Chromium source tree.

### Process Model

Electron has two primary process types, mirroring Chromium:

- **Main process** (`shell/browser/` + `lib/browser/`): Controls app lifecycle, creates windows, system APIs
- **Renderer process** (`shell/renderer/` + `lib/renderer/`): Runs web content in BrowserWindows

### Native ↔ JavaScript Bridge

Each API is implemented as a C++/JS pair:

- C++ side: `shell/browser/api/electron_api_{name}.cc/.h` — uses `gin::Wrappable` and `ObjectTemplateBuilder`
- JS side: `lib/browser/api/{name}.ts` — exports the module, registered in `lib/browser/api/module-list.ts`
- Binding: `NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_{name}, Initialize)` in C++ and registered in `shell/common/node_bindings.cc`
- Type declaration: `typings/internal-ambient.d.ts` maps `process._linkedBinding('electron_browser_{name}')`

### Patches System

Electron patches upstream dependencies (Chromium, Node.js, V8, etc.) rather than forking them. Patches live in `patches/` organized by target, with `patches/config.json` mapping directories to repos.

```text
patches/{target}/*.patch  →  [e sync]   →  target repo commits
                          ←  [e patches] ←
```

Key rules:

- Fix existing patches rather than creating new ones
- Preserve original authorship in TODO comments — never change `TODO(name)` assignees
- Each patch commit message must explain why the patch exists
- After modifying patches, run `e patches {target}` to export

When working on the `roller/chromium/main` branch for Chromium upgrades, use `e sync --3` for 3-way merge conflict resolution.

## Conventions

### File Naming

- JS/TS files: kebab-case (`file-name.ts`)
- C++ files: snake_case with `electron_api_` prefix (`electron_api_safe_storage.cc`)
- Test files: `api-{module-name}-spec.ts` in `spec/`
- Source file lists are maintained in `filenames.gni` (with platform-specific sections)

### JavaScript/TypeScript

- Semicolons required (`"semi": ["error", "always"]`)
- `const` and `let` only (no `var`)
- Arrow functions preferred
- Import order enforced: `@electron/internal` → `@electron` → `electron` → external → builtin → relative
- API naming: `PascalCase` for classes (`BrowserWindow`), `camelCase` for module APIs (`globalShortcut`)
- Prefer getters/setters over jQuery-style `.text([text])` patterns

### C++

- Follows Chromium coding style, enforced by `clang-format` and `clang-tidy`
- Uses Chromium abstractions (`base::`, `content::`, etc.)
- Header guards: `#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_{NAME}_H_`
- Platform-specific files: `_mac.mm`, `_win.cc`, `_linux.cc`

### Testing

- Framework: Mocha + Chai + Sinon
- Test helpers in `spec/lib/` (e.g., `spec-helpers.ts`, `window-helpers.ts`)
- Use `defer()` from spec-helpers for cleanup, `closeAllWindows()` for window teardown
- Tests import from `electron/main` or `electron/renderer`

### Documentation

- API docs in `docs/api/` as Markdown, parsed by `@electron/docs-parser` to generate `electron.d.ts`
- API history tracked via YAML blocks in HTML comments within doc files
- Docs must pass `npm run lint:docs`

### Build Configuration

- `BUILD.gn`: Main GN build config
- `buildflags/buildflags.gni`: Feature flags (PDF viewer, extensions, spellchecker)
- `build/args/`: Build argument profiles (`testing.gn`, `release.gn`, `all.gn`)
- `DEPS`: Dependency versions and checkout paths
- `chromium_src/`: Chromium source file overrides (compiled instead of originals)
