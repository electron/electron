# Analyzing Patch Failures

## Investigation Steps

1. **Read the patch file** at `patches/node/{patch_name}.patch`

2. **Examine current state** of the file in the Node.js repo at mentioned line numbers

3. **Check recent upstream changes:**
   ```bash
   cd ../third_party/electron_node
   git log --oneline -10 -- {file}
   ```

4. **Find Node.js PR** in commit messages:
   ```
   PR-URL: https://github.com/nodejs/node/pull/{PR_NUMBER}
   ```

## Critical: Resolve by Intent, Not by Mechanical Merge

When resolving a patch conflict, do NOT blindly preserve the patch's old code. Instead:

1. **Understand the upstream commit's full scope** — not just the conflicting hunk.
   Run `git show <commit> --stat` and read diffs for all affected files.
   Upstream may have removed structs, members, or methods that the patch
   references in other hunks or files.

2. **Re-read the patch commit message** to understand its *intent* — what
   behavior does it need to preserve or add?

3. **Implement the intent against the new upstream code.** If the patch's
   purpose is "add BoringSSL compatibility", add only the compatibility
   layer — don't also restore old code that upstream separately removed.

### Lesson: Upstream Removals Break Patch References

- **Trigger:** Patch conflict involves an upstream refactor (not just context drift)
- **Strategy:** After identifying the upstream commit, check its full diff for
  removed types, members, and methods. If the patch's old code references
  something removed, the resolution must use the new upstream mechanism.

### Lesson: Separate Patch Purpose from Patch Implementation

- **Trigger:** Conflict between "upstream simplified code" vs "patch has older code"
- **Strategy:** Identify the *minimal* change the patch needs. If the patch
  wraps code in a conditional, only add the conditional — don't restore old
  code that was inside the conditional but was separately cleaned up upstream.

### Lesson: Finish the Adaptation at Conflict Time

- **Trigger:** A patch conflict involves an upstream API removal or replacement
- **Strategy:** When resolving the conflict, fully adapt the patch to use the
  new API in the same commit. Don't remove the old code and leave behind stale
  references that will "be fixed in Phase Two." Each patch fix commit should be
  a complete resolution.

## Common Failure Patterns

| Pattern | Cause | Solution |
|---------|-------|----------|
| Context lines don't match | Surrounding code changed | Update context in patch |
| File not found | File renamed/moved | Update patch target path |
| Function not found | Refactored upstream | Find new function name |
| OpenSSL → BoringSSL mismatch | Crypto API change | Update to BoringSSL-compatible API |
| GYP/GN build change | Build system refactor | Adapt build patch to new structure |
| Deleted code | Feature removed | Verify patch still needed |
| V8 API bridge patch conflicts | Node.js caught up to Chromium's V8 | Patch may be deletable — verify the API is now in Node.js' V8 natively |

## Using Git Blame

To find the commit that changed specific lines:

```bash
cd ../third_party/electron_node
git blame -L {start},{end} -- {file}
git log -1 {commit_sha}  # Look for PR-URL: line
```

## Verifying Patch Necessity

Before deleting a patch, verify:
1. The patched functionality was intentionally removed upstream
2. Electron doesn't need the patch for other reasons
3. No other code depends on the patched behavior

**V8 bridge patches:** Electron uses Chromium's V8, which is often ahead of the V8 bundled in Node.js. Many patches exist to bridge this version gap — adapting Node.js code to work with newer V8 APIs that Chromium's V8 exposes. During major Node.js upgrades, Node.js' V8 catches up to Chromium's, and these bridge patches often become unnecessary. Check whether the API the patch shims is now available natively in the new Node.js version's V8.

When in doubt, keep the patch and adapt it.

## Phase Two: Build-Time Patch Issues

Sometimes patches that applied successfully in Phase One cause build errors in Phase Two. This can happen when:

1. **Incomplete types**: A patch disables a header include, but new upstream code uses the type
2. **Missing members**: A patch modifies a class, but upstream added new code referencing the original

### Finding Which Patch Affects a File

```bash
grep -l "filename.cc" patches/node/*.patch
```

### Matching Existing Patch Patterns

When fixing build errors in patched files, examine the existing patch to understand its style:
- Does it use `#if 0` / `#endif` guards?
- Does it use `#if BUILDFLAG(...)` conditionals?
- Does it use `#ifndef` / `#ifdef` guards for BoringSSL vs OpenSSL?
- What's the pattern for disabled functionality?

Apply fixes consistent with the existing patch style.
