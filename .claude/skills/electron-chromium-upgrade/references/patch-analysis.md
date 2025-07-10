# Analyzing Patch Failures

## Investigation Steps

1. **Read the patch file** at `patches/{target}/{patch_name}.patch`

2. **Examine current state** of the file in Chromium at mentioned line numbers

3. **Check recent upstream changes:**
   ```bash
   cd ..  # or relevant target repo
   git log --oneline -10 -- {file}
   ```

4. **Find Chromium CL** in commit messages:
   ```
   Reviewed-on: https://chromium-review.googlesource.com/c/chromium/src/+/{CL_NUMBER}
   ```

## Critical: Resolve by Intent, Not by Mechanical Merge

When resolving a patch conflict, do NOT blindly preserve the patch's old code. Instead:

1. **Understand the upstream CL's full scope** — not just the conflicting hunk.
   Run `git show <commit> --stat` and read diffs for all affected files.
   Upstream may have removed structs, members, or methods that the patch
   references in other hunks or files.

2. **Re-read the patch commit message** to understand its *intent* — what
   behavior does it need to preserve or add?

3. **Implement the intent against the new upstream code.** If the patch's
   purpose is "add a feature flag guard", add only the guard — don't also
   restore old code inside the guard that upstream separately removed.

### Lesson: Upstream Removals Break Patch References

- **Trigger:** Patch conflict involves an upstream refactor (not just context drift)
- **Strategy:** After identifying the upstream CL, check its full diff for
  removed types, members, and methods. If the patch's old code references
  something removed, the resolution must use the new upstream mechanism.
- **Evidence:** An upstream CL removed a `HeadlessModeWindow` struct from a
  header, but the conflict was only in a `.mm` file. Mechanically keeping the
  patch's old line (`headless_mode_window_ = ...`) produced code referencing
  a nonexistent type — caught only on review, not at patch-apply time.

### Lesson: Separate Patch Purpose from Patch Implementation

- **Trigger:** Conflict between "upstream simplified code" vs "patch has older code"
- **Strategy:** Identify the *minimal* change the patch needs. If the patch
  wraps code in a conditional, only add the conditional — don't restore old
  code that was inside the conditional but was separately cleaned up upstream.
- **Evidence:** An occlusion patch needed only a feature flag check, but the
  old patch also contained a version check that upstream intentionally removed.
  Mechanically preserving the old patch code re-added the removed check.

### Lesson: Finish the Adaptation at Conflict Time

- **Trigger:** A patch conflict involves an upstream API removal or replacement
- **Strategy:** When resolving the conflict, fully adapt the patch to use the
  new API in the same commit. Don't remove the old code and leave behind stale
  references that will "be fixed in Phase Two." Each patch fix commit should be
  a complete resolution.
- **Evidence:** A safestorage patch conflicted because Chromium removed Keychain V1.
  The conflict was resolved by removing V1 hunks, but the remaining code still
  called V1 methods (`FindGenericPassword` with 3 args, `ItemDelete` with
  `SecKeychainItemRef`). These should have been adapted to V2 APIs in the same
  commit, not deferred.

## Common Failure Patterns

| Pattern | Cause | Solution |
|---------|-------|----------|
| Context lines don't match | Surrounding code changed | Update context in patch |
| File not found | File renamed/moved | Update patch target path |
| Function not found | Refactored upstream | Find new function name |
| `DCHECK` → `CHECK_IS_TEST` | Macro change | Update to new macro |
| Deleted code | Feature removed | Verify patch still needed |

## Using Git Blame

To find the CL that changed specific lines:

```bash
cd ..
git blame -L {start},{end} -- {file}
git log -1 {commit_sha}  # Look for Reviewed-on: line
```

## Verifying Patch Necessity

Before deleting a patch, verify:
1. The patched functionality was intentionally removed upstream
2. Electron doesn't need the patch for other reasons
3. No other code depends on the patched behavior

When in doubt, keep the patch and adapt it.

## Phase Two: Build-Time Patch Issues

Sometimes patches that applied successfully in Phase One cause build errors in Phase Two. This can happen when:

1. **Incomplete types**: A patch disables a header include, but new upstream code uses the type
2. **Missing members**: A patch modifies a class, but upstream added new code referencing the original

### Finding Which Patch Affects a File

```bash
grep -l "filename.cc" patches/chromium/*.patch
```

Matching Existing Patch Patterns

When fixing build errors in patched files, examine the existing patch to understand its style:
- Does it use #if 0 / #endif guards?
- Does it use #if BUILDFLAG(...) conditionals?
- What's the pattern for disabled functionality?

Apply fixes consistent with the existing patch style.
