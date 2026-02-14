# Phase Two Commit Guidelines

Only follow these instructions if there are uncommitted changes in the Electron repo after any fixes are made during Phase Two that result a target that was failing, successfully building.

Ignore other instructions about making commit messages, our guidelines are CRITICALLY IMPORTANT and must be followed.

## Two Commit Types

### For Electron Source Changes (shell/, electron/, etc.)

```
{CL-Number}: {upstream CL's original title}

Ref: {Chromium CL link}
```

Use the **upstream CL's original commit title** — do not paraphrase or rewrite it. To find it: `git log -1 --format=%s <chromium-commit-hash>`.

Only add a description body if it provides clarity beyond what the title already says (e.g., when Electron's adaptation is non-obvious). For simple renames, method additions, or straightforward API updates, the title + Ref link is sufficient.

IMPORTANT: Ensure that any change made to electron as a result of a change in Chromium is committed individually. Each change should have it's own commit message and it's own REF. Logically grouped into commits that make sense rather than one giant commit.

IMPORTANT: Try really hard to find the CL reference per the instructions below. Each change you made should in theory have been in response to a change made in Chromium that you identified or can identify. Try for a while to identify and include the ref in the commit message. Do not give up easily.

You may include multiple "Ref" links if required.

For a CL link in the format `https://chromium-review.googlesource.com/c/chromium/src/+/2958369` the "CL-Number" is `2958369`

### For Patch Updates (patches/chromium/*.patch)

Use the same fixup workflow as Phase One:
1. Fix in Chromium source tree
2. Fixup commit + rebase
3. Export with `e patches chromium`
4. Commit the patch file:

```
fix(patch-update): {concise description}

{Brief explanation}

Ref: {Chromium CL link}
```

## Dependent Patch Header Updates

After any patch modification, check for other affected patches:

```bash
git status
# If other .patch files show as modified with only index, line number, and context changes:
git add patches/
git commit -m "chore: update patches (trivial only)"
```

## Finding CL References

Use git log or git blame on Chromium source files. Look for:

Reviewed-on: https://chromium-review.googlesource.com/c/chromium/src/+/XXXXXXX

If no CL found after searching: Ref: Unable to locate CL

## Example Commits

### Electron Source Fix (simple — title is self-explanatory)

```
7535923: Rename ozone buildflags

Ref: https://chromium-review.googlesource.com/c/chromium/src/+/7535923
```

### Electron Source Fix (complex — description adds value)

```
7534194: Convert some functions in ui::Clipboard to async

Adapted ExtractCustomPlatformNames calls to use RunLoop pattern
consistent with existing ReadImage implementation, since upstream
converted the API from synchronous return to callback-based.

Ref: https://chromium-review.googlesource.com/c/chromium/src/+/7534194
```

### Patch Fix

```
fix(patch-update): update picture-in-picture for gesture handling refactor

Upstream added new gesture handling code that accesses live caption dialog.
The live caption functionality is disabled in Electron's patch, so wrapped
the new code in #if 0 guards to match existing pattern.

Ref: https://chromium-review.googlesource.com/c/chromium/src/+/7654321
```
