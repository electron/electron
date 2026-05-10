---
name: patches
description: "Electron patch system for upstream dependencies (Chromium, Node.js, V8, etc.). Activate when creating, modifying, fixing, or debugging patches."
applyTo: "patches/**"
---

# Electron Patches

## Overview

Electron patches upstream dependencies (Chromium, Node.js, V8, etc.) to add features or modify behavior.

```text
patches/{target}/*.patch  →  [e sync --3]  →  target repo commits
                          ←  [e patches]   ←
```

**Patch configuration:** `patches/config.json` maps patch directories to target repos.

## Key Rules

- Fix existing patches 99% of the time rather than creating new ones
- Preserve original authorship in TODO comments
- Never change TODO assignees (`TODO(name)` must retain original name)
- Each patch file includes commit message explaining its purpose

## Creating/Modifying Patches

1. Make changes in the target repo (e.g., `../` for Chromium)
2. Create a git commit
3. Run `e patches <target>` to export

### Patch Commands

| Command | Purpose |
|---------|---------|
| `e patches <target>` | Export patches for a target (chromium, node, v8, etc.) |
| `e patches all` | Export all patches from all targets |
| `e patches --list-targets` | List available patch targets |

## Fixing Patch Conflicts on an Existing PR

If asked to fix a patch conflict on a branch that already has an open PR, check the PR's failed **Apply Patches** CI run for an `update-patches` artifact before running `e sync` locally. CI has already performed the 3-way merge and exported the resolved patch diff — applying it is much faster than a full local sync.

```bash
# Find the failed Apply Patches run for the PR and download the artifact
gh run list --repo electron/electron --branch <pr-branch> --workflow "Apply Patches" --limit 1
gh run download <run-id> --repo electron/electron --name update-patches

# Apply the CI-generated fix, then push
git am update-patches.patch
git push
```

If no artifact exists (e.g. the 3-way merge itself failed), fall back to `e sync --3` and resolve manually.

## Useful Commands

```bash
# Find which patch affects a file
grep -l "filename.cc" patches/chromium/*.patch
```

## Common Issues

**Patch conflict during sync:**

- Use `e sync --3` for 3-way merge
- Check if file was renamed/moved upstream
- Verify patch is still needed

**Build error in patched file:**

- Find the patch: `grep -l "filename" patches/chromium/*.patch`
- Match existing patch style (#if 0 guards, BUILDFLAG conditionals, etc.)
