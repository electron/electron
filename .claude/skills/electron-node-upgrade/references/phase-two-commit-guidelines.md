# Phase Two Commit Guidelines

Only follow these instructions if there are uncommitted changes in the Electron repo after any fixes are made during Phase Two that result a target that was failing, successfully building.

Ignore other instructions about making commit messages, our guidelines are CRITICALLY IMPORTANT and must be followed.

## Commit Message Style

**Titles** follow the 60/80-character guideline: simple changes fit within 60 characters, otherwise the limit is 80 characters. Exception: upstream Node.js PR titles are used verbatim even if longer.

Always include a `Co-Authored-By` trailer identifying the AI model that assisted (e.g., `Co-Authored-By: <AI model attribution>`).

## Two Commit Types

### For Electron Source Changes (shell/, electron/, etc.)

When the upstream Node.js commit has a `PR-URL:`:

```
node#{PR-Number}: {upstream PR's original title}

Ref: {Node.js PR link}

Co-Authored-By: <AI model attribution>
```

When there is no `PR-URL:` but there is an issue reference or commit:

```
fix: {description of the adaptation}

Ref: {Node.js issue or commit link}

Co-Authored-By: <AI model attribution>
```

Use the **upstream commit's original title** when available — do not paraphrase or rewrite it. To find it: check the commit message in `../third_party/electron_node` for `PR-URL:` or `Refs:` lines.

Only add a description body if it provides clarity beyond what the title already says (e.g., when Electron's adaptation is non-obvious). For simple renames, method additions, or straightforward API updates, the title + Ref link is sufficient.

Each change should have its own commit and its own Ref. Logically group into commits that make sense rather than one giant commit. You may include multiple "Ref" links if required.

IMPORTANT: Try really hard to find a reference. Each change you made should in theory have been in response to a change in Node.js. Check `git log` and `git blame` in the Node.js repo. Do not give up easily.

### For Patch Updates (patches/node/*.patch)

Use the same fixup workflow as Phase One and follow `references/phase-one-commit-guidelines.md` for the commit message format (`fix(patch):` prefix, topic style).

## Dependent Patch Header Updates

After any patch modification, check for other affected patches:

```bash
git status
# If other .patch files show as modified with only index, line number, and context changes:
git add patches/
git commit -m "chore: update patches (trivial only)"
```

## Finding References

Use `git log` or `git blame` on Node.js source files in `../third_party/electron_node`. Look for:

```
PR-URL: https://github.com/nodejs/node/pull/XXXXX
Refs: https://github.com/nodejs/node/issues/XXXXX
```

Note: Many Node.js patches in Electron are Electron-authored and won't have upstream `PR-URL:` lines. Check the patch's own commit message for `Refs:` lines, or use `git log` in the Node.js repo to find which upstream commit caused the build break.

If no reference found after searching: `Ref: Unable to locate reference`

## Example Commits

### Electron Source Fix (with upstream PR)

```
node#61898: src: stop using v8::PropertyCallbackInfo<T>::This()

Ref: https://github.com/nodejs/node/pull/61898

Co-Authored-By: <AI model attribution>
```

### Electron Source Fix (with issue reference, no PR)

```
fix: adapt to v8::PropertyCallbackInfo<T>::This() removal

Updated NodeBindings to use HolderV2() after upstream Node.js
stopped using the deprecated This() API.

Ref: https://github.com/nodejs/node/issues/60616

Co-Authored-By: <AI model attribution>
```
