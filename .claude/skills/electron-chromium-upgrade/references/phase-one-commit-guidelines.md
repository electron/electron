# Phase One Commit Guidelines

Only follow these instructions if there are uncommitted changes to `patches/` after Phase One succeeds.

Ignore other instructions about making commit messages, our guidelines are CRITICALLY IMPORTANT and must be followed.

## Each Commit Must Be Complete

When resolving a patch conflict, fully adapt the patch to the new upstream code in the same commit. If the upstream change removes an API the patch uses, update the patch to use the replacement API now — don't leave stale references knowing they'll need fixing later. The goal is that each commit represents a finished resolution, not a partial one that defers known work to a future phase.

## Lint Enforcement

Commits on `roller/chromium/*` branches are validated by `script/lint-roller-chromium-changes.mjs`. Every commit MUST match one of the allowed patterns below — there is no fallback. If a commit cannot be made to match an allowed pattern, add a `Skip-Lint: <reason>` trailer as a last-resort escape hatch.

Allowed commit patterns:

1. `fixup! <existing-commit-title>` — fixup against another commit on the branch
2. `chore: bump chromium in DEPS to <version>` — must be the *only* change to `DEPS`, no other files
3. `chore: update patches` — exact title, no commit body, only non-content patch changes (index hashes, line numbers, hunk headers)
4. A commit referencing one Chromium CL: title must be exactly `<CL_NUMBER>: <upstream CL's original title>`
5. A commit referencing multiple Chromium CLs: title is not enforced

A Chromium CL is referenced by including the full Gerrit URL anywhere in the commit message, e.g. `Ref: https://chromium-review.googlesource.com/c/chromium/src/+/7536483`. Appending `#nolint` to the URL excludes that CL from validation (and from the single-CL title check).

## Commit Message Style

**Titles** follow the 60/80-character guideline: simple changes fit within 60 characters, otherwise the limit is 80 characters. Exception: upstream Chromium CL titles are used verbatim even if longer.

Always include a `Co-Authored-By` trailer identifying the AI model that assisted (e.g., `Co-Authored-By: <AI model attribution>`).

### Patch conflict fixes

Use the upstream CL number and original title as the commit title:

```
<CL-Number>: <upstream CL's original title>

Ref: <Chromium CL link>

Co-Authored-By: <AI model attribution>
```

Use the **upstream CL's original commit title** verbatim — do not paraphrase or rewrite it. To find it: `git log -1 --format=%s <chromium-commit-hash>`.

For a CL link in the format `https://chromium-review.googlesource.com/c/chromium/src/+/7536483` the `CL-Number` is `7536483`.

Only add a description body if it provides clarity beyond the title (e.g., when the patch adaptation is non-obvious). For straightforward context drift or simple API renames, the title + Ref is sufficient.

### Upstreamed patch removal

When patches are no longer needed (applied cleanly with "already applied" or confirmed upstreamed), group ALL removals into a single commit:

```
chore: remove upstreamed patches

Ref: <Chromium CL link for patch 1>
Ref: <Chromium CL link for patch 2>
...

Co-Authored-By: <AI model attribution>
```

Including multiple `Ref:` lines satisfies the linter (multi-CL commits skip the title format check). Extract each CL URL from the patch file's `Reviewed-on:` trailer (for cherry-picks) or by searching the upstream history (for non-cherry-pick patches).

If only one patch is being removed and you can identify its corresponding CL, prefer the single-CL format from "Patch conflict fixes" above so the commit conforms to the standard `<CL-Number>: <subject>` pattern. Fall back to `chore: remove upstreamed patches` (plural) only when grouping multiple removals.

### Trivial patch updates

After all fix commits, stage remaining trivial changes (index hashes, line numbers, context lines only):

```bash
git add patches
git commit -m "chore: update patches"
```

The commit message MUST be exactly `chore: update patches` — no qualifier, no body, no trailers. The linter rejects this commit if any patch file contains `+`/`-` content lines (lines that are not diff headers, hunk markers, or `index` lines).

**Conflict resolution can produce trivial results.** A `git am` conflict doesn't always mean the patch content changed — context drift alone can cause a conflict. After resolving and exporting, inspect the patch diff: if only index hashes, line numbers, and context lines changed (not the patch's own `+`/`-` lines), it's trivial and belongs here, not in a `<CL-Number>: <subject>` commit.

## Atomic Commits

Each patch conflict fix gets its own commit with its own Ref.

IMPORTANT: Try really hard to find the CL reference per the instructions below. Each change you made should in theory have been in response to a change made in Chromium that you identified or can identify. Try for a while to identify and include the ref in the commit message. Do not give up easily.

## Finding CL References

Use `git log` or `git blame` on Chromium source files. Look for:

```
Reviewed-on: https://chromium-review.googlesource.com/c/chromium/src/+/XXXXXXX
```

If no CL can be found after thorough searching, the commit must use a `Skip-Lint` trailer (it will otherwise fail lint):

```
<short description of the patch fix>

Co-Authored-By: <AI model attribution>
Skip-Lint: <reason — e.g., "no upstream CL; local build fix">
```

Skip-Lint is a last resort. Do not reach for it before genuinely exhausting the search.

## Example Commits

### Patch conflict fix (simple — title is sufficient)

```
7536483: [DomStorage] Share PurgeOrigins() between LevelDB and SQLite

Ref: https://chromium-review.googlesource.com/c/chromium/src/+/7536483

Co-Authored-By: <AI model attribution>
```

### Patch conflict fix (complex — description adds value)

```
7540447: Cleanup Keychain V1

Upstream deleted the V1 Keychain API. Removed V1 hunks and adapted
keychain_password_mac.mm to use KeychainV2 APIs.

Ref: https://chromium-review.googlesource.com/c/chromium/src/+/7540447

Co-Authored-By: <AI model attribution>
```

### Upstreamed patch removal (multiple)

```
chore: remove upstreamed patches

Ref: https://chromium-review.googlesource.com/c/chromium/src/+/7531001
Ref: https://chromium-review.googlesource.com/c/chromium/src/+/7531234

Co-Authored-By: <AI model attribution>
```

### Trivial patch updates

```
chore: update patches
```

(No body, no trailers.)
