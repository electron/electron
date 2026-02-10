---
name: electron-chromium-upgrade
description: Guide for performing Chromium version upgrades in the Electron project. Use when working on the roller/chromium/main branch to fix patch conflicts during `e sync --3`. Covers the patch application workflow, conflict resolution, analyzing upstream Chromium changes, and proper commit formatting for patch fixes.
---

# Electron Chromium Upgrade: Phase One

## Summary

Run `e sync --3` repeatedly, fixing patch conflicts as they arise, until it succeeds. Then run `e patches all` and commit changes atomically.

## Success Criteria

Phase One is complete when:
- `e sync --3` exits with code 0 (no patch failures)
- `e patches all` has been run to export all changes
- All changes are committed per the commit guidelines below

Do not stop until these criteria are met.

**CRITICAL** Do not delete or skip patches unless 100% certain the patch is no longer needed. Complicated conflicts or hard to resolve issues should be presented to the user after you have exhausted all other options. Do not delete the patch just because you can't solve it.

## Context

The `roller/chromium/main` branch is created by automation to update Electron's Chromium dependency SHA. No work has been done to handle breaking changes between the old and new versions.

**Key directories:**
- Current directory: Electron repo (always run `e` commands here)
- `..` (parent): Chromium repo (where most patches apply)
- `patches/`: Patch files organized by target
- `docs/development/patches.md`: Patch system documentation

## Workflow

1. Delete the `.git/rr-cache` in both the `electron` and `..` folder to ensure no accidental rerere replays occur from before this upgrade phase attempt started
2. Run `e sync --3` (the `--3` flag enables 3-way merge, always required)
3. If succeeds → skip to step 6
4. If patch fails:
   - Identify target repo and patch from error output
   - Analyze failure (see references/patch-analysis.md)
   - Fix conflict in target repo's working directory
   - Run `git am --continue` in affected repo
   - Repeat until all patches for that repo apply
   - IMPORTANT: Once `git am --continue` succeeds you MUST run `e patches {target}` to export fixes
   - Return to step 1
5. When `e sync --3` succeeds, run `e patches all`
6. **Read `references/phase-one-commit-guidelines.md` NOW**, then commit changes following those instructions exactly.

Before committing any Phase One changes, you MUST read `references/phase-one-commit-guidelines.md` and follow its instructions exactly.

## Commands Reference

| Command | Purpose |
|---------|---------|
| `e sync --3` | Clone deps and apply patches with 3-way merge |
| `git am --continue` | Continue after resolving conflict (run in target repo) |
| `e patches {target}` | Export commits from target repo to patch files |
| `e patches all` | Export all patches from all targets |
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

## Final Deliverable

After Phase One, write a summary of every change: what was fixed, why, reasoning, and Chromium CL links.

# Electron Chromium Upgrade: Phase Two

## Summary

Run `e build -k 999` repeatedly, fixing build issues as they arise, until it succeeds. Then run `e start --version` to validate Electron launches and commit changes atomically.

Run Phase Two immediately after Phase One is complete.

## Success Criteria

Phase Two is complete when:
- `e build -k 999` exits with code 0 (no build failures)
- `e start --version` has been run to check Electron launches
- All changes are committed per the commit guidelines below

Do not stop until these criteria are met. Do not delete code or features, never comment out code in order to take short cut. Make all existing code, logic and intention work.

## Context

The `roller/chromium/main` branch is created by automation to update Electron's Chromium dependency SHA. No work has been done to handle breaking changes between the old and new versions. Chromium APIs frequently are renamed or refactored. In every case the code in Electron must be updated to account for the change in Chromium, strongly avoid making changes to the code in chromium to fix Electrons build.

**Key directories:**
- Current directory: Electron repo (always run `e` commands here)
- `..` (parent): Chromium repo (do not touch this code to fix build issues, just read it to obtain context)

## Workflow

1. Run `e build -k 999` (the `-k 999` flag is a flag to ninja to say "do not stop until you find that many errors" it is an attempt to get as much error
  context as possible for each time we run build)
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
    - Commit them immediately with: `git commit -am "chore: update patch hunk headers"`
    - This prevents losing track of necessary updates
5. Return to step 1
6. When `e build` succeeds, run `e start --version`
7. Check if you have any pending changes in the Chromium repo by running `git status`
    - If you have changes follow the instructions below in "A. Patch Fixes" to correctly commit those modifications into the appropriate patch file

Before committing any Phase Two changes, you MUST read `references/phase-two-commit-guidelines.md` and follow its instructions exactly.

## Build Error Detection

When monitoring `e build -k 999` output, filter for errors using this regex pattern:
error:|FAILED:|fatal:|subcommand failed|build finished

The build output is extremely verbose. Filtering is essential to catch errors quickly.

## Commands Reference

| Command | Purpose |
|---------|---------|
| `e build -k 999` | Builds Electron and won't stop until either all targets attempted or 999 errors found |
| `e build -t {target}.o` | Build just one specific target to verify a fix |
| `e start --version` | Validate Electron launches after successful build |

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
3. Export the updated patch: e patches chromium
4. Commit the updated patch file in the electron repo following the `references/phase-one-commit-guidelines.md`, then commit changes following those instructions exactly. **READ THESE GUIDELINES BEFORE COMMITTING THESE CHANGES**

To find the original patch commit to fixup: `git log --oneline | grep -i "keyword from patch name"`

The base commit for rebase is the Chromium commit before patches were applied. Find it by checking the `refs/patches/upstream-head` ref.

B. Electron Code Fixes (for files in shell/, electron/, etc.)

When the error is in Electron's own source code:

1. Edit files directly in the electron repo
2. Commit directly (no patch export needed)

Dependent Patch Updates

IMPORTANT: When you modify a patch, other patches that apply to the same file may have their hunk headers invalidated. After committing a patch fix:

1. Run git status in the electron repo
2. Look for other modified .patch files with just index/hunk header changes
3. Commit these with: git commit -m "chore: update patch hunk headers"

# Critical: Read Before Committing

- Before ANY Phase One commits: Read `references/phase-one-commit-guidelines.md`
- Before ANY Phase Two commits: Read `references/phase-two-commit-guidelines.md`

# Skill Directory Structure
This skill has additional reference files in `references/`:
- patch-analysis.md - How to analyze patch failures
- phase-one-commit-guidelines.md - Commit format for Phase One
- phase-two-commit-guidelines.md - Commit format for Phase Two

Read these when referenced in the workflow steps.