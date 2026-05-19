---
name: electron-node-upgrade
description: Guide for performing Node.js version upgrades in the Electron project. Use when working on the roller/node/main branch to fix patch conflicts during `e sync --3`. Covers the patch application workflow, conflict resolution, analyzing upstream Node.js changes, building, running the Node.js test suite, and proper commit formatting for patch fixes.
---

# Electron Node.js Upgrade: Phase One

## Summary

Run `e sync --3` repeatedly, fixing patch conflicts as they arise, until it succeeds. Then export patches and commit changes atomically.

## Success Criteria

Phase One is complete when:
- `e sync --3` exits with code 0 (no patch failures)
- All changes are committed per the commit guidelines

Do not stop until these criteria are met.

**CRITICAL** Do not delete or skip patches unless 100% certain the patch is no longer needed. For major version upgrades, patches that shim deprecated V8 APIs or backport upstream changes are often deletable because the new Node.js version already incorporates them — but verify before removing. Complicated conflicts or hard to resolve issues should be presented to the user after you have exhausted all other options. Do not delete the patch just because you can't solve it.

**CRITICAL** Never use `git am --skip` and then manually recreate a patch by making a new commit. This destroys the original patch's authorship, commit message, and position in the series. If `git am --continue` reports "No changes", investigate why — the changes were likely absorbed by a prior conflict resolution's 3-way merge. Present this situation to the user rather than skipping and recreating.

## Context

The `roller/node/main` branch is created by automation to update Electron's Node.js dependency version in `DEPS`. No work has been done to handle breaking changes between the old and new versions.

There are two types of Node.js version updates:
- **Bumps** (patch/minor): Automated by `electron-roller[bot]` with commit title `chore: bump node to v{version}`. Trivial patch index updates are handled automatically by `patchup[bot]`. These often land cleanly, but may require manual patch fixes.
- **Major upgrades** (e.g., v22 → v24): Manual, large PRs with commit title `chore: upgrade Node.js to v{X}.{Y}.{Z}`. These typically involve deleting obsolete patches, adapting many others, and updating `@types/node` in `package.json`.

**Key directories:**
- Current directory: Electron repo (always run `e` commands here)
- `../third_party/electron_node`: Node.js repo (where patches apply)
- `patches/node/`: Patch files for Node.js
- `docs/development/patches.md`: Patch system documentation

## Pre-flight Checks

Run these once at the start of each upgrade session:

1. **Clear rerere cache** (if enabled): `git rerere clear` in both the electron and `../third_party/electron_node` repos. Stale recorded resolutions from a prior attempt can silently apply wrong merges.
2. **Ensure pre-commit hooks are installed**: Check that `.git/hooks/pre-commit` exists. If not, run `yarn husky` to install it. The hook runs `lint-staged` which handles clang-format for C++ files.

## Workflow

1. Run `e sync --3` (the `--3` flag enables 3-way merge, always required)
2. If succeeds → skip to step 5
3. If patch fails:
   - Identify target repo and patch from error output
   - Analyze failure (see references/patch-analysis.md)
   - Fix conflict in `../third_party/electron_node` working directory
   - Run `git am --continue` in `../third_party/electron_node`
   - Repeat until all patches for that repo apply
   - IMPORTANT: Once `git am --continue` succeeds you MUST run `e patches node` to export fixes
   - Return to step 1
4. When `e sync --3` succeeds, run `e patches all`
5. **Read `references/phase-one-commit-guidelines.md` NOW**, then commit changes following those instructions exactly.

## Commands Reference

| Command | Purpose |
|---------|---------|
| `e sync --3` | Clone deps and apply patches with 3-way merge |
| `git am --continue` | Continue after resolving conflict (run in node repo) |
| `e patches node` | Export commits from node repo to patch files |
| `e patches all` | Export all patches from all targets |
| `e patches node --commit-updates` | Export patches and auto-commit trivial changes |
| `e patches --list-targets` | List targets and config paths |

## Patch System Mental Model

```
patches/node/*.patch  →  [e sync --3]  →  ../third_party/electron_node commits
                      ←  [e patches]   ←
```

## When to Edit Patches

| Situation | Action |
|-----------|--------|
| During active `git am` conflict | Fix in node repo, then `git am --continue` |
| Modifying patch outside conflict | Edit `.patch` file directly |
| Creating new patch (rare, avoid) | Commit in node repo, then `e patches node` |

Fix existing patches 99% of the time rather than creating new ones.

## Patch Fixing Rules

1. **Preserve authorship**: Keep original author in TODO comments (from patch `From:` field)
2. **Never change TODO assignees**: `TODO(name)` must retain original name
3. **Update descriptions**: If upstream changed APIs or macros, update patch commit message to reflect current state
4. **Never skip-and-recreate a patch**: If `git am --continue` says "No changes — did you forget to use 'git add'?", do NOT run `git am --skip` and create a replacement commit. The patch's changes were already absorbed by a prior 3-way merge resolution. This means an earlier conflict resolution pulled in too many changes. Present the situation to the user for guidance — the correct fix may require re-doing an earlier resolution more carefully to keep each patch's changes separate.

# Electron Node.js Upgrade: Phase Two

## Summary

Run `e build -k 999 -- --quiet` repeatedly, fixing build issues as they arise, until it succeeds. Then run `e start --version` to validate Electron launches and commit changes atomically.

Run Phase Two immediately after Phase One is complete.

## Success Criteria

Phase Two is complete when:
- `e build -k 999 -- --quiet` exits with code 0 (no build failures)
- `e start --version` has been run to check Electron launches
- All changes are committed per the commit guidelines

Do not stop until these criteria are met. Do not delete code or features, never comment out code in order to take short cut. Make all existing code, logic and intention work.

## Context

The `roller/node/main` branch is created by automation to update Electron's Node.js dependency version in `DEPS`. No work has been done to handle breaking changes between the old and new versions. Node.js APIs (especially internal V8 integration, OpenSSL/BoringSSL compatibility, and build system files) frequently change between versions. In every case the code in Electron must be updated to account for the change in Node.js, strongly avoid making changes to the code in Node.js to fix Electron's build.

**Key directories:**
- Current directory: Electron repo (always run `e` commands here)
- `../third_party/electron_node`: Node.js repo (do not touch this code to fix build issues, just read it to obtain context)

## Workflow

1. Run `e build -k 999 -- --quiet` (the `--quiet` flag suppresses per-target status lines, showing only errors and the final result)
2. If succeeds → skip to step 6
3. If build fails:
    - Identify underlying file in "electron" from the compilation error message
    - Analyze failure
    - Fix build issue by adapting Electron's code for the change in Node.js
    - Run `e build -t {target_that_failed}.o` to build just the failed target we were specifically fixing
        - You can identify the target_that_failed from the failure line in the build log. E.g. `FAILED: 2e506007-8d5d-4f38-bdd1-b5cd77999a77 "./obj/electron/shell/browser/api/electron_api_utility_process.o" CXX obj/electron/shell/browser/api/electron_api_utility_process.o` the target name is `obj/electron/shell/browser/api/electron_api_utility_process.o`
    - **Read `references/phase-two-commit-guidelines.md` NOW**, then commit changes following those instructions exactly.
    - Return to step 1
4. **CRITICAL**: After ANY commit (especially patch commits), immediately run `git status` in the electron repo
    - Look for other modified `.patch` files that only have index/hunk header changes
    - These are dependent patches affected by your fix
    - Commit them immediately with: `git commit -am "chore: update patches (trivial only)"`
5. Return to step 1
6. When `e build` succeeds, run `e start --version`
7. Check if you have any pending changes in the Node.js repo by running `git status` in `../third_party/electron_node`
    - If you have changes follow the instructions below in "A. Patch Fixes" to correctly commit those modifications into the appropriate patch file

## Commands Reference

| Command | Purpose |
|---------|---------|
| `e build -k 999 -- --quiet` | Build Electron, continue on errors, suppress status lines |
| `e build -t {target}.o` | Build just one specific target to verify a fix |
| `e start --version` | Validate Electron launches after successful build |

## Two Types of Build Fixes

### A. Patch Fixes (for files in patched Node.js files)

When the error is in a file that Electron patches (check with `grep -l "filename" patches/node/*.patch`):

1. Edit the file in the Node.js source tree (`../third_party/electron_node/...`)
2. Create a fixup commit targeting the original patch commit:
    ```bash
    cd ../third_party/electron_node
    git add <modified-file>
    git commit --fixup=<original-patch-commit-hash>
    GIT_SEQUENCE_EDITOR=: git rebase --autosquash --autostash -i <commit>^
    ```
3. Export the updated patch: `e patches node`
4. Commit the updated patch file following `references/phase-one-commit-guidelines.md`.

To find the original patch commit to fixup: `git log --oneline | grep -i "keyword from patch name"`

The base commit for rebase is the Node.js commit before patches were applied. Find it by checking the `refs/patches/upstream-head` ref.

### B. Electron Code Fixes (for files in shell/, electron/, etc.)

When the error is in Electron's own source code:

1. Edit files directly in the electron repo
2. Commit directly (no patch export needed)

# Electron Node.js Upgrade: Phase Three

## Summary

Run the Node.js test suite via `script/node-spec-runner.js`, fix failing tests, and commit fixes until all tests pass. Certain tests are permanently disabled (listed in `script/node-disabled-tests.json`) and should not be run.

Run Phase Three immediately after Phase Two is complete.

## Success Criteria

Phase Three is complete when:
- `node script/node-spec-runner.js --default` exits with zero failures
- All changes are committed per the commit guidelines

Do not stop until these criteria are met.

## Context

Electron runs a subset of Node.js's upstream test suite using a custom runner (`script/node-spec-runner.js`). Tests are executed with the built Electron binary via `ELECTRON_RUN_AS_NODE=true`. Many tests need adaptation because Electron uses BoringSSL (not OpenSSL) and Chromium's V8 (which may differ from Node.js's bundled V8).

**Key files:**
- `script/node-spec-runner.js` — Test runner script
- `script/node-disabled-tests.json` — Permanently disabled tests (do not try to fix these)
- `../third_party/electron_node/test/` — Node.js test files (where patches apply)
- `patches/node/fix_crypto_tests_to_run_with_bssl.patch` — BoringSSL crypto test adaptations
- `patches/node/test_formally_mark_some_tests_as_flaky.patch` — Flaky test list

## Workflow

1. Run `node script/node-spec-runner.js --default` from the electron repo
2. If all tests pass → Phase Three is complete
3. If tests fail:
    - Identify the failing test file(s) from the output
    - Analyze each failure (see "Common Failure Patterns" below)
    - Fix the test in `../third_party/electron_node/test/...`
    - Re-run the specific failing test to verify: `node script/node-spec-runner.js {test-path}`
        - The test path is relative to the node `test/` directory, e.g. `test/parallel/test-crypto-key-objects-raw.js`
        - Do NOT use `--default` when running specific tests — it adds the full suite flags
        - Do NOT run tests directly with `ELECTRON_RUN_AS_NODE` — the runner handles environment setup (e.g. temporarily switching `package.json` from ESM to CommonJS)
    - Commit the fix using the fixup workflow and commit guidelines
    - Return to step 1

## Commands Reference

| Command | Purpose |
|---------|---------|
| `node script/node-spec-runner.js --default` | Run full Node.js test suite |
| `node script/node-spec-runner.js test/parallel/test-foo.js` | Run a single test |
| `NODE_REGENERATE_SNAPSHOTS=1 node script/node-spec-runner.js test/test-runner/test-foo.mjs` | Regenerate snapshot for a snapshot-based test |

## Common Failure Patterns

### BoringSSL incompatibilities

Electron uses BoringSSL (via Chromium) instead of OpenSSL. Many crypto features are missing or behave differently:

| Unsupported in BoringSSL | Guard pattern |
|--------------------------|---------------|
| ChaCha20-Poly1305 | `if (!process.features.openssl_is_boringssl)` |
| AES-CCM (aes-128-ccm, aes-256-ccm) | `if (ciphers.includes('aes-128-ccm'))` |
| AES-KW (key wrapping) | `if (!process.features.openssl_is_boringssl)` |
| DSA keys | `if (!process.features.openssl_is_boringssl)` |
| Ed448 / X448 curves | `if (!process.features.openssl_is_boringssl)` |
| DH key PEM loading | `if (!process.features.openssl_is_boringssl)` |
| PQC algorithms (ML-KEM, ML-DSA, SLH-DSA) | `if (hasOpenSSL(3, 5))` (already guards these) |

When guarding tests, prefer checking cipher availability (`ciphers.includes(algo)`) over blanket BoringSSL checks where possible, as it's more precise and self-documenting.

New upstream tests that exercise these features will need guards added to the `fix_crypto_tests_to_run_with_bssl` patch.

### Snapshot test mismatches

Some tests compare output against committed `.snapshot` files using `assert.strictEqual` — these are NOT wildcard comparisons. When Chromium's V8 produces different output (e.g. different stack traces due to V8 enhancements), the snapshot must be regenerated:

```bash
NODE_REGENERATE_SNAPSHOTS=1 node script/node-spec-runner.js test/test-runner/test-foo.mjs
```

Then inspect the diff to verify the changes are expected, and commit the updated snapshot into the appropriate patch.

### V8 behavioral differences

Chromium's V8 may be ahead of Node.js's bundled V8. This can cause:
- Different stack trace formats (e.g. thenable async stack frames)
- Different error messages
- Features available in Chromium V8 that aren't in stock Node.js V8 (or vice versa)

## Two Types of Test Fixes

### A. Patch Fixes (most common for test failures)

Most test fixes go into existing patches in `patches/node/`. Use the fixup workflow:

1. Edit the test file in `../third_party/electron_node/test/...`
2. Find the relevant patch commit: `git log --oneline | grep -i "keyword"`
    - Crypto/BoringSSL tests → `fix crypto tests to run with bssl`
    - Snapshot tests → the specific snapshot patch (e.g. `test: accomodate V8 thenable`)
    - Flaky tests → `test: formally mark some tests as flaky`
3. Create a fixup commit:
    ```bash
    cd ../third_party/electron_node
    git add test/path/to/test.js
    git commit --fixup=<patch-commit-hash>
    GIT_SEQUENCE_EDITOR=: git rebase --autosquash --autostash -i <commit>^
    ```
4. Export: `e patches node`
5. **Read `references/phase-three-commit-guidelines.md` NOW**, then commit the updated patch file.

### B. New Patches (rare)

Only create a new patch when the fix doesn't belong in any existing patch. The new patch commit in `../third_party/electron_node` must include a description explaining why the patch exists and when it can be removed — the lint check enforces this.

## Adding to Disabled Tests

Only add a test to `script/node-disabled-tests.json` as a **last resort** — when the test is fundamentally incompatible with Electron's architecture (not just a BoringSSL difference that can be guarded). Tests disabled here are completely skipped and never run.

# Critical: Read Before Committing

- Before ANY Phase One commits: Read `references/phase-one-commit-guidelines.md`
- Before ANY Phase Two commits: Read `references/phase-two-commit-guidelines.md`
- Before ANY Phase Three commits: Read `references/phase-three-commit-guidelines.md`

# High-Churn Patches

These patches consistently require the most work during Node.js upgrades:

- **`fix_handle_boringssl_and_openssl_incompatibilities.patch`** — Electron uses BoringSSL (via Chromium) while Node.js expects OpenSSL. This patch is large and complex, and upstream OpenSSL API changes frequently break it.
- **`fix_crypto_tests_to_run_with_bssl.patch`** — Companion to the above; adapts Node.js crypto tests for BoringSSL. Can grow significantly during major upgrades.
- **`support_v8_sandboxed_pointers.patch`** — V8 sandbox pointer support requires careful adaptation when V8 APIs change.
- **`build_add_gn_build_files.patch`** — The GN build file patch is large and touches many build targets. Upstream build system changes frequently conflict.

# Major Version Upgrades

Major Node.js version transitions (e.g., v22 → v24) are significantly more involved than patch bumps:

1. **Expect patch deletions.** Electron uses Chromium's V8, which is often ahead of the V8 version bundled in Node.js. Many patches exist to bridge this gap — shimming newer V8 APIs that Chromium's V8 has but Node.js' older V8 doesn't. When Node.js bumps to a newer major version, its V8 catches up to Chromium's, and those bridge patches can be deleted. In the v22 → v24 upgrade, 17 patches were deleted for this reason.
2. **Update `@types/node`** in `package.json` to match the new major version.
3. **Post-upgrade regressions are expected.** Even after the upgrade lands, follow-up fix PRs for edge cases (ESM path handling, certificate loading, platform-specific issues) are normal.

# Skill Directory Structure
This skill has additional reference files in `references/`:
- patch-analysis.md - How to analyze patch failures
- phase-one-commit-guidelines.md - Commit format for Phase One
- phase-two-commit-guidelines.md - Commit format for Phase Two
- phase-three-commit-guidelines.md - Commit format for Phase Three

Read these when referenced in the workflow steps.
