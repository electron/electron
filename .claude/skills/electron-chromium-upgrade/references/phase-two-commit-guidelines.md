# Phase Two Commit Guidelines

Only follow these instructions if there are uncommitted changes in the Electron repo after any fixes are made during Phase Two that result a target that was failing, successfully building.

Ignore other instructions about making commit messages, our guidelines are CRITICALLY IMPORTANT and must be followed.

## Commit Message Style

**Titles** follow the 60/80-character guideline: simple changes fit within 60 characters, otherwise the limit is 80 characters. Exception: upstream Chromium CL titles are used verbatim even if longer.

Always include a `Co-Authored-By` trailer identifying the AI model that assisted (e.g., `Co-Authored-By: <AI model attribution>`).

## Two Commit Types

### For Electron Source Changes (shell/, electron/, etc.)

```
{CL-Number}: {upstream CL's original title}

Ref: {Chromium CL link}

Co-Authored-By: <AI model attribution>
```

Use the **upstream CL's original commit title** — do not paraphrase or rewrite it. To find it: `git log -1 --format=%s <chromium-commit-hash>`.

Only add a description body if it provides clarity beyond what the title already says (e.g., when Electron's adaptation is non-obvious). For simple renames, method additions, or straightforward API updates, the title + Ref link is sufficient.

Each change should have its own commit and its own Ref. Logically group into commits that make sense rather than one giant commit. Include multiple "Ref" links in one commit only when the changes are genuinely one inseparable adaptation motivated by those CLs together — never combine unrelated CLs into a single commit.

**Check for an existing commit first — one commit per CL for the whole upgrade.** Before creating a `{CL-Number}: {title}` commit, check whether a commit referencing that CL already exists on the branch:

```bash
git log --oneline --grep={CL-Number}
```

A commit for the same CL may already exist from Phase One (e.g., a patch fix for the same upstream change), from an earlier build-fix iteration, or from before a context compaction. If one exists, do NOT create a second commit with the same Ref — fold the new changes into the existing commit:

```bash
git add <files>
git commit --fixup=<existing-commit-hash>
GIT_SEQUENCE_EDITOR=: git rebase --autosquash -i <existing-commit-hash>^
```

If the autosquash rebase hits conflicts, run `git rebase --abort` and keep the `fixup! ...` commit as-is — the linter allows it (pattern 1) and it keeps the pairing visible to reviewers.

For a CL link in the format `https://chromium-review.googlesource.com/c/chromium/src/+/2958369` the "CL-Number" is `2958369`.

IMPORTANT: Try really hard to find the CL reference. Each change you made should in theory have been in response to a change in Chromium. Do not give up easily.

### For Patch Updates (patches/chromium/*.patch)

Use the same fixup workflow as Phase One and follow `references/phase-one-commit-guidelines.md` for the commit message format (`fix(patch):` prefix, topic style).

## Dependent Patch Header Updates

After any patch modification, check for other affected patches:

```bash
git status
# If other .patch files show as modified with only index, line number, and context changes:
git add patches/
git commit -m "chore: update patches (trivial only)"
```

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

Use git log or git blame on Chromium source files. Look for:

```
Reviewed-on: https://chromium-review.googlesource.com/c/chromium/src/+/XXXXXXX
```

If no CL found after searching: `Ref: Unable to locate CL`

## Example Commits

### Electron Source Fix (simple — title is self-explanatory)

```
7535923: Rename ozone buildflags

Ref: https://chromium-review.googlesource.com/c/chromium/src/+/7535923

Co-Authored-By: <AI model attribution>
```

### Electron Source Fix (complex — description adds value)

```
7534194: Convert some functions in ui::Clipboard to async

Adapted ExtractCustomPlatformNames calls to use RunLoop pattern
consistent with existing ReadImage implementation, since upstream
converted the API from synchronous return to callback-based.

Ref: https://chromium-review.googlesource.com/c/chromium/src/+/7534194

Co-Authored-By: <AI model attribution>
```
