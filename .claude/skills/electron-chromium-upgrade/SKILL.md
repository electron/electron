---
name: electron-chromium-upgrade
description: Guide for performing Chromium version upgrades in the Electron project. Use when working on the roller/chromium/main branch to fix patch conflicts during `e sync --3`. Covers the patch application workflow, conflict resolution, analyzing upstream Chromium changes, and proper commit formatting for patch fixes.
---

# Electron Chromium Upgrade: Phase One

## Summary

Run `e sync --3` repeatedly, fixing patch conflicts as they arise, until it succeeds. Then export patches and commit changes atomically.

## Success Criteria

**Linter Requirements**:
- Code/patch edits: Title must be exactly `{CL-Number}: {upstream CL original title}` with `Ref: {URL}` in the body.
- Metadata-only patch updates (hashes/line numbers): Message must be exactly `chore: update patches` with no body.

Phase One is complete when:
- `e sync --3` exits with code 0 (no patch failures)
- All changes are committed per the commit guidelines

Do not stop until these criteria are met.

**CRITICAL** Do not delete or skip patches unless 100% certain the patch is no longer needed. Complicated conflicts or hard to resolve issues should be presented to the user after you have exhausted all other options. Do not delete the patch just because you can't solve it.

**CRITICAL** Never use `git am --skip` and then manually recreate a patch by making a new commit. This destroys the original patch's authorship, commit message, and position in the series. If `git am --continue` reports "No changes", investigate why — the changes were likely absorbed by a prior conflict resolution's 3-way merge. Present this situation to the user rather than skipping and recreating.

## Context

The `roller/chromium/main` branch is created by automation to update Electron's Chromium dependency SHA. No work has been done to handle breaking changes between the old and new versions.

**Key directories:**
- Current directory: Electron repo (always run `e` commands here)
- `..` (parent): Chromium repo (where most patches apply)
- `patches/`: Patch files organized by target
- `docs/development/patches.md`: Patch system documentation

## Pre-flight Checks

Run these once at the start of each upgrade session:

1. **Clear rerere cache** (if enabled): `git rerere clear` in both the electron and `..` repos. Stale recorded resolutions from a prior attempt can silently apply wrong merges.
2. **Ensure pre-commit hooks are installed**: Check that `git config core.hooksPath` prints `.husky/_` and that `.husky/_/pre-commit` exists (husky does not use `.git/hooks/`). If not, run `yarn husky` to install it. The hook runs `lint-staged`, which handles clang-format for C++ files and runs `script/lint-roller-chromium-changes.mjs` on every commit.

## Workflow

1. Run `e sync --3` (the `--3` flag enables 3-way merge, always required)
2. If succeeds → skip to step 5
3. If patch fails:
   - Identify target repo and patch from error output
   - Analyze failure (see references/patch-analysis.md)
   - Fix conflict in target repo's working directory
   - Run `git am --continue` in affected repo
   - Repeat until all patches for that repo apply
   - IMPORTANT: Once `git am --continue` succeeds you MUST run `e patches {target}` to export fixes
   - Return to step 1
4. When `e sync --3` succeeds, run `e patches all`
5. **Regenerate the derived file lists.** Electron checks in lists that mirror Chromium's DEPS pins, and the gn-check CI job fails the roll on every platform if any of them is stale. The roller bot bumps `DEPS` without running the pre-commit hook that normally regenerates them, so run the generators by hand now (the gn-check job runs the `--check` form of each):
   ```bash
   node script/gen-hunspell-filenames.js   # filenames.hunspell.gni (from ../third_party/hunspell_dictionaries)
   node script/gen-libc++-filenames.js     # filenames.libcxx.gni / filenames.libcxxabi.gni
   node script/gen-siso-revision.js        # build/siso_revision (the gen_siso_revision sync hook usually did this already)
   node script/gen-filenames.mts --check   # fails only if lib/ or docs/api changed; regenerate without --check if it does
   ```
   Then `git status` and commit each changed file on its own:
   - `build/siso_revision` → `chore: update siso revision` (exact title, no body). CI builds siso from this file.
   - `filenames.libcxx.gni` → `chore: update libc++ filenames` (exact title, no body, that file only).
   - `filenames.hunspell.gni` (and `filenames.libcxxabi.gni`, which has no dedicated pattern) → a `{CL-Number}: {upstream CL original title}` commit with a `Ref:` line, where the CL is the upstream commit that moved the pin. Find it from the dep's new SHA in Chromium's `DEPS` (hunspell is `src/third_party/hunspell_dictionaries`, libc++abi is `src/third_party/libc++abi/src`):
     ```bash
     cd .. && git log --format='%h %s%n%(trailers:key=Reviewed-on,valueonly)' refs/patches/upstream-head -S<new-dep-sha> -- DEPS
     ```
6. **Read `references/phase-one-commit-guidelines.md` NOW**, then commit changes following those instructions exactly.

## Commands Reference

| Command | Purpose |
|---------|---------|
| `e sync --3` | Clone deps and apply patches with 3-way merge |
| `git am --continue` | Continue after resolving conflict (run in target repo) |
| `e patches {target}` | Export commits from target repo to patch files |
| `e patches all` | Export all patches from all targets |
| `e patches {target} --commit-updates` | Export patches and auto-commit trivial changes |
| `e patches --list-targets` | List targets and config paths |

## Patch System Mental Model

```
patches/{target}/*.patch  →  [e sync --3]  →  target repo commits
                          ←  [e patches]   ←
```

## When to Edit Patches

| Situation | Action |
|-----------|--------|
| During active `git am` conflict | Fix in target repo, then `git am --continue` |
| Modifying patch outside conflict | Edit `.patch` file directly |
| Creating new patch (rare, avoid) | Commit in target repo, then `e patches {target}` |

Fix existing patches 99% of the time rather than creating new ones.

## Patch Fixing Rules

1. **Preserve authorship**: Keep original author in TODO comments (from patch `From:` field)
2. **Never change TODO assignees**: `TODO(name)` must retain original name
3. **Update descriptions**: If upstream changed (e.g., `DCHECK` → `CHECK_IS_TEST`), update patch commit message to reflect current state
4. **Never skip-and-recreate a patch**: If `git am --continue` says "No changes — did you forget to use 'git add'?", do NOT run `git am --skip` and create a replacement commit. The patch's changes were already absorbed by a prior 3-way merge resolution. This means an earlier conflict resolution pulled in too many changes. Present the situation to the user for guidance — the correct fix may require re-doing an earlier resolution more carefully to keep each patch's changes separate.

# Electron Chromium Upgrade: Phase Two

## Summary

Run `e build -k 999 -- --quiet` repeatedly, fixing build issues as they arise, until it succeeds. Then run `e start --version` to validate Electron launches and commit changes atomically.

Run Phase Two immediately after Phase One is complete.

## Success Criteria

**Linter Requirements**:
- Code/patch edits: Title must be exactly `{CL-Number}: {upstream CL original title}` with `Ref: {URL}` in the body.
- Metadata-only patch updates (hashes/line numbers): Message must be exactly `chore: update patches` with no body.

Phase Two is complete when:
- `e build -k 999 -- --quiet` exits with code 0 (no build failures)
- `e start --version` has been run to check Electron launches
- `npm run gn-check` exits with code 0 and every `script/gen-*` `--check` passes (see step 7 — this is what the gn-check CI job runs, and it fails the roll independently of the build)
- All changes are committed per the commit guidelines

Do not stop until these criteria are met. Do not delete code or features, never comment out code in order to take short cut. Make all existing code, logic and intention work.

## Context

The `roller/chromium/main` branch is created by automation to update Electron's Chromium dependency SHA. No work has been done to handle breaking changes between the old and new versions. Chromium APIs frequently are renamed or refactored. In every case the code in Electron must be updated to account for the change in Chromium, strongly avoid making changes to the code in chromium to fix Electrons build.

**Key directories:**
- Current directory: Electron repo (always run `e` commands here)
- `..` (parent): Chromium repo (do not touch this code to fix build issues, just read it to obtain context)

## Workflow

1. Run `e build -k 999 -- --quiet` (the `--quiet` flag suppresses per-target status lines, showing only errors and the final result)
2. If succeeds → skip to step 6
3. If build fails:
    - Identify underlying file in "electron" from the compilation error message
    - Analyze failure
    - Fix build issue by adapting Electron's code for the change in Chromium
    - Run `e build -t {target_that_failed}.o` to build just the failed target we were specifically fixing
        - You can identify the target_that_failed from the failure line in the build log. E.g. `FAILED: 2e506007-8d5d-4f38-bdd1-b5cd77999a77 "./obj/electron/chromium_src/chrome/process_singleton_posix.o" CXX obj/electron/chromium_src/chrome/process_singleton_posix.o` the target name is `obj/electron/chromium_src/chrome/process_singleton_posix.o`
    - **Read `references/phase-two-commit-guidelines.md` NOW**, then commit changes following those instructions exactly.
    - Return to step 1
4. **CRITICAL**: After ANY commit (especially patch commits), immediately run `git status` in the electron repo
    - Look for other modified `.patch` files that only have index/hunk header changes
    - These are dependent patches affected by your fix
    - Commit them immediately with: `git commit -am "chore: update patches"`
5. Return to step 1
6. When `e build` succeeds, run `e start --version`
7. **Run the gn-check CI job locally.** CI runs `linux-gn-check`, `macos-gn-check` and `windows-gn-check` on every roll; a green build does not imply they pass. Run exactly what the job runs (see `.github/workflows/pipeline-segment-electron-gn-check.yml`), from the electron directory:
    ```bash
    npm run gn-check                          # `gn check` on electron targets against the current out dir
    node script/gen-hunspell-filenames.js --check
    node script/gen-libc++-filenames.js --check
    node script/gen-siso-revision.js --check
    node script/gen-filenames.mts --check
    ```
    - A `gn check` failure means a header is included from a target that is not in `deps` — fix `BUILD.gn`/`filenames.gni` (or the include) in Electron, then re-run. CI checks the `testing` and `release` graphs on all three platforms, so an include guarded by another platform's `BUILDFLAG` can still fail there; read the included header's `BUILD.gn` visibility/deps when the local check passes but CI does not.
    - A `gen-*` `--check` failure means a derived list is stale: run the generator without `--check` and commit per Phase One step 5.
8. Check if you have any pending changes in the Chromium repo by running `git status`
    - If you have changes follow the instructions below in "A. Patch Fixes" to correctly commit those modifications into the appropriate patch file
9. Final commit self-check: run `git log --format='%h %B'` over the commits this upgrade added (everything since the `chore: bump chromium in DEPS` commit) and verify that each upstream CL is referenced by exactly one non-fixup commit — full messages, not just titles, since `Ref:` lines live in commit bodies. If a CL appears in more than one non-fixup commit, consolidate with `git commit --fixup` + autosquash rebase per `references/phase-two-commit-guidelines.md`

## Commands Reference

| Command | Purpose |
|---------|---------|
| `e build -k 999 -- --quiet` | Build Electron, continue on errors, suppress status lines |
| `e build -t {target}.o` | Build just one specific target to verify a fix |
| `e build --no-remote -t {target}.o` | Same, built locally — use when the remote executor fails on toolchain/module inputs rather than on Electron code |
| `e start --version` | Validate Electron launches after successful build |
| `npm run gn-check` | `gn check` over Electron targets, as the gn-check CI job runs it |

## Two Types of Build Fixes

### A. Patch Fixes (for files in chromium_src or patched Chromium files)

When the error is in a file that Electron patches (check with `grep -l "filename" patches/chromium/*.patch`):

1. Edit the file in the Chromium source tree (e.g., `/src/chrome/browser/...`)
2. Create a fixup commit targeting the original patch commit:
    ```bash
    cd ..  # to chromium repo
    git add <modified-file>
    git commit --fixup=<original-patch-commit-hash>
    GIT_SEQUENCE_EDITOR=: git rebase --autosquash --autostash -i <commit>^
    ```
3. Export the updated patch: `e patches chromium`
4. Commit the updated patch file following `references/phase-one-commit-guidelines.md`.

To find the original patch commit to fixup: `git log --oneline | grep -i "keyword from patch name"`

The base commit for rebase is the Chromium commit before patches were applied. Find it by checking the `refs/patches/upstream-head` ref.

### B. Electron Code Fixes (for files in shell/, electron/, etc.)

When the error is in Electron's own source code:

1. Edit files directly in the electron repo
2. Commit directly (no patch export needed)

# Critical: Read Before Committing

- Before ANY Phase One commits: Read `references/phase-one-commit-guidelines.md`
- Before ANY Phase Two commits: Read `references/phase-two-commit-guidelines.md`

# Skill Directory Structure
This skill has additional reference files in `references/`:
- patch-analysis.md - How to analyze patch failures
- phase-one-commit-guidelines.md - Commit format for Phase One
- phase-two-commit-guidelines.md - Commit format for Phase Two

Read these when referenced in the workflow steps.
