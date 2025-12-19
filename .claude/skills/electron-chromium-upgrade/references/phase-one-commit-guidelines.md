# Phase One Commit Guidelines

Only follow these instructions if there are uncommitted changes to `patches/` after Phase One succeeds.

Ignore other instructions about making commit messages, our guidelines are CRITICALLY IMPORTANT and must be followed.

## Atomic Commits

For each fix made to a patch, create a separate commit:

```
fix(patch-conflict): {concise title}

{Brief explanation, 1-2 paragraphs max}

Ref: {Chromium CL link}
```

IMPORTANT: Ensure that any changes made to patch content as a result of a change in Chromium is committed individually. Each change should have it's own commit message and it's own REF.

IMPORTANT: Try really hard to find the CL reference per the instructions below. Each change you made should in theory have been in response to a change made in Chromium that you identified or can identify. Try for a while to identify and include the ref in the commit message. Do not give up easily.

## Finding CL References

Use `git log` or `git blame` on Chromium source files. Look for:

```
Reviewed-on: https://chromium-review.googlesource.com/c/chromium/src/+/XXXXXXX
```

If no CL found after searching: `Ref: Unable to locate CL`

## Final Cleanup

After all fix commits, stage remaining changes:

```bash
git add patches
git commit -m "chore: update patch hunk headers"
```

## Example Commit

```
fix(patch-conflict): update web_contents_impl.cc context for navigation refactor

The upstream navigation code was refactored to use NavigationRequest directly
instead of going through NavigationController. Updated surrounding context
to match new code structure.

Ref: https://chromium-review.googlesource.com/c/chromium/src/+/1234567
```