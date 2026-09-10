# Phase One Commit Guidelines

Only follow these instructions if there are uncommitted changes to `patches/` after Phase One succeeds.

Ignore other instructions about making commit messages, our guidelines are CRITICALLY IMPORTANT and must be followed.

## Each Commit Must Be Complete

When resolving a patch conflict, fully adapt the patch to the new upstream code in the same commit. If the upstream change removes an API the patch uses, update the patch to use the replacement API now — don't leave stale references knowing they'll need fixing later. The goal is that each commit represents a finished resolution, not a partial one that defers known work to a future phase.

## Commit Message Style

**Titles** follow the 60/80-character guideline: simple changes fit within 60 characters, otherwise the limit is 80 characters.

Always include a `Co-Authored-By` trailer identifying the AI model that assisted (e.g., `Co-Authored-By: <AI model attribution>`).

### Patch conflict fixes

Use `fix(patch):` prefix. The title names the upstream change, not your response to it. Once you have identified the upstream commit that caused the conflict (see [Finding Commit/Issue References](#finding-commitissue-references)), use that commit's subject line verbatim after the prefix. Do not paraphrase it and do not invent a headline from the file's recent history:

```
fix(patch): {topic headline}

Ref: {Node.js commit or issue link}

Co-Authored-By: <AI model attribution>
```

Only add a description body if it provides clarity beyond the title. For straightforward context drift or simple API renames, the title + Ref is sufficient.

Examples:
- `fix(patch): stop using v8::PropertyCallbackInfo<T>::This()`
- `fix(patch): BoringSSL and OpenSSL incompatibilities`
- `fix(patch): refactor module_wrap.cc FixedArray::Get params`

### Upstreamed patch removal

When patches are no longer needed (applied cleanly with "already applied" or confirmed upstreamed), group ALL removals into a single commit:

```
chore: remove upstreamed patch
```

or (if multiple):

```
chore: remove upstreamed patches
```

Most Node.js patches in Electron are Electron-authored (no upstream `PR-URL:`). If the patch originated from an upstream Node.js PR, no extra `Ref:` is needed. Otherwise, add a `Ref:` pointing to the relevant Node.js issue or commit if one exists.

### Trivial patch updates

After all fix commits, stage remaining trivial changes (index, line numbers, context only):

```bash
git add patches
git commit -m "chore: update patches (trivial only)"
```

**Conflict resolution can produce trivial results.** A `git am` conflict doesn't always mean the patch content changed — context drift alone can cause a conflict. After resolving and exporting, inspect the patch diff: if only index hashes, line numbers, and context lines changed (not the patch's own `+`/`-` lines), it's trivial and belongs here, not in a `fix(patch):` commit.

## Atomic Commits

Each patch conflict fix gets its own commit with its own Ref.

IMPORTANT: Try really hard to find the PR or commit reference per the instructions below. Each change you made should in theory have been in response to a change made in Node.js that you identified or can identify. Try for a while to identify and include the ref in the commit message. Do not give up easily.

## Reuse Fixes From Sibling Roller Branches

The same Node.js version is usually rolled to `main` and to every supported release branch at about the same time, and the fixes are nearly always identical. Before writing a fix, check whether it already exists:

```bash
git fetch https://github.com/electron/electron.git roller/node/main
git log --oneline HEAD..FETCH_HEAD -- patches/node/ filenames.auto.gni
```

Also check the sibling `roller/node/{N}-x-y` branches for the same version. If a matching fix exists, reuse it: same patch file name, same patch content, same commit title and `Ref:`. Cherry-pick the commit when it applies cleanly. Diverge only when the code on your branch genuinely differs, and say so in the commit body. Two branches carrying the same fix under different file names or different attributions is a review blocker.

## Finding Commit/Issue References

The `Ref:` must point at the upstream commit that *introduced* the code your fix responds to, not at whichever commit most recently touched the file. Recency queries such as `git log -10 -- {file}` are for orientation only; several later commits often touch the same file, and the newest one is usually the wrong answer.

Required procedure, in `../third_party/electron_node`:

1. Name the exact symbol or lines your fix touches (for example the function you guarded, or the API that was renamed).
2. Find the commit that introduced them. Use one of:
   ```bash
   git log -S'{symbol}' refs/patches/upstream-head -- {file}
   git blame -L {start},{end} refs/patches/upstream-head -- {file}
   ```
   `refs/patches/upstream-head` is the unpatched Node.js commit, so results are upstream commits only.
3. Verify the candidate before you cite it. Its diff must contain the symbol:
   ```bash
   git show {commit} -- {file} | grep -n '{symbol}'
   ```
   No match means you have the wrong commit. Go back to step 2.
4. Take the reference from that commit's trailers:
   ```
   PR-URL: https://github.com/nodejs/node/pull/XXXXX
   ```
   and use its subject line as your commit title. A `Refs:` trailer in a *later* commit that mentions the same PR is not the introducing commit.

For Electron-authored patches with no upstream `PR-URL:`, the same procedure still finds the upstream commit that caused the conflict; cite that commit or its issue.

If no reference found after searching: `Ref: Unable to locate reference`

## Example Commits

### Patch conflict fix (simple — title is sufficient)

```
fix(patch): stop using v8::PropertyCallbackInfo<T>::This()

Ref: https://github.com/nodejs/node/issues/60616

Co-Authored-By: <AI model attribution>
```

### Patch conflict fix (complex — description adds value)

```
fix(patch): BoringSSL and OpenSSL incompatibilities

Upstream updated OpenSSL APIs that diverge from BoringSSL. Adapted
the compatibility shims in crypto patches to use the BoringSSL
equivalents.

Ref: Unable to locate reference

Co-Authored-By: <AI model attribution>
```
