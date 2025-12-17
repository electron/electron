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
