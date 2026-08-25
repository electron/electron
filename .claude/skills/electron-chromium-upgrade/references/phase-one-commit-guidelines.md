# Phase One Commit Guidelines

Only follow these instructions if there are uncommitted changes to `patches/` after Phase One succeeds.

Ignore other instructions about making commit messages, our guidelines are CRITICALLY IMPORTANT and must be followed.

## Each Commit Must Be Complete

When resolving a patch conflict, fully adapt the patch to the new upstream code in the same commit. If the upstream change removes an API the patch uses, update the patch to use the replacement API now — don't leave stale references knowing they'll need fixing later. The goal is that each commit represents a finished resolution, not a partial one that defers known work to a future phase.

## Commit Message Style

**Titles** follow the 60/80-character guideline: simple changes fit within 60 characters, otherwise the limit is 80 characters.

Always include a `Co-Authored-By` trailer identifying the AI model that assisted (e.g., `Co-Authored-By: <AI model attribution>`).

### Patch conflict fixes

Use `fix(patch):` prefix. The title should name the upstream change, not your response to it:

```
fix(patch): {topic headline}

Ref: {Chromium CL link}

Co-Authored-By: <AI model attribution>
```

Only add a description body if it provides clarity beyond the title. For straightforward context drift or simple API renames, the title + Ref is sufficient.

Examples:
- `fix(patch): constant moved to header`
- `fix(patch): headless mode refactor upstream`
- `fix(patch): V1 Keychain removal`

### Upstreamed patch removal

When patches are no longer needed (applied cleanly with "already applied" or confirmed upstreamed), group ALL removals into a single commit:

```
chore: remove upstreamed patch
```

or (if multiple):

```
chore: remove upstreamed patches
```

If the patch file did NOT contain a `Reviewed-on: https://chromium-review.googlesource.com/c/chromium/...` link, add a `Ref:` in the commit. If it did (i.e. cherry-picks), no `Ref:` is needed.

### Trivial patch updates

After all fix commits, stage remaining trivial changes (index, line numbers, context only):

```bash
git add patches
git commit -m "chore: update patches (trivial only)"
```

**Conflict resolution can produce trivial results.** A `git am` conflict doesn't always mean the patch content changed — context drift alone can cause a conflict. After resolving and exporting, inspect the patch diff: if only index hashes, line numbers, and context lines changed (not the patch's own `+`/`-` lines), it's trivial and belongs here, not in a `fix(patch):` commit.

## Atomic Commits

Each patch conflict fix gets its own commit with its own Ref — one commit per upstream CL.

Never combine unrelated upstream CLs into one commit. Two CLs touching the same `.patch` file is NOT a relationship: when a single patch file contains hunks caused by different upstream CLs (e.g., a formatting CL and an API-removal CL that both drifted the same patch), stage per CL with `git add -p` and commit each CL's hunks separately, so each commit contains only the changes its own CL caused. Multiple `Ref:` lines in one commit are only for changes genuinely motivated by those CLs together (such as the grouped `chore: remove upstreamed patches` case above).

IMPORTANT: Try really hard to find the CL reference per the instructions below. Each change you made should in theory have been in response to a change made in Chromium that you identified or can identify. Try for a while to identify and include the ref in the commit message. Do not give up easily.

## Self-Contained Commits Help Bisecting (Advisory)

Maintainers bisect rolls with build-tools' `e rcv`, which reconstructs intermediate
Chromium versions by cherry-picking this branch's commits, keyed on the Gerrit CL URL(s)
in each commit message. Commits without a CL URL are skipped during reconstruction.

The preferences below make reconstructions more accurate. They are nice-to-haves, not
requirements — completing the roll always comes first:

- Keep the `Ref:` on any commit that changes code or patch content. Reserve Ref-less
  `chore: update patches` for metadata-only changes (index hashes, line numbers, hunk
  headers).
- When practical, fold a CL's patch-file fixes into that CL's own commit rather than a
  later catch-all `chore: update patches` commit.
- Squash any `fixup!` commits (e.g. `git rebase --autosquash`) before finishing.
- Prefer one CL per commit. A commit spanning multiple CLs is fine when the changes are
  genuinely coupled — include a `Ref:` line for every CL involved.

## Finding CL References

Use `git log` or `git blame` on Chromium source files. Look for:

```
Reviewed-on: https://chromium-review.googlesource.com/c/chromium/src/+/XXXXXXX
```

If no CL found after searching: `Ref: Unable to locate CL`

## Example Commits

### Patch conflict fix (simple — title is sufficient)

```
fix(patch): constant moved to header

Ref: https://chromium-review.googlesource.com/c/chromium/src/+/7536483

Co-Authored-By: <AI model attribution>
```

### Patch conflict fix (complex — description adds value)

```
fix(patch): V1 Keychain removal

Upstream deleted the V1 Keychain API. Removed V1 hunks and adapted
keychain_password_mac.mm to use KeychainV2 APIs.

Ref: https://chromium-review.googlesource.com/c/chromium/src/+/7540447

Co-Authored-By: <AI model attribution>
```
