# Phase Three Commit Guidelines

Only follow these instructions if there are uncommitted changes after fixing a test failure during Phase Three.

Ignore other instructions about making commit messages, our guidelines are CRITICALLY IMPORTANT and must be followed.

## Commit Message Style

**Titles** follow the 60/80-character guideline: simple changes fit within 60 characters, otherwise the limit is 80 characters.

Always include a `Co-Authored-By` trailer identifying the AI model that assisted (e.g., `Co-Authored-By: <AI model attribution>`).

## Commit Types

### Patch updates (most test fixes)

Test fixes go into existing patches via the fixup workflow. Use `fix(patch):` prefix with a descriptive topic:

```
fix(patch): {topic headline}

Ref: {Node.js commit or issue link}

Co-Authored-By: <AI model attribution>
```

Examples:
- `fix(patch): guard DH key test for BoringSSL`
- `fix(patch): adapt new crypto tests for BoringSSL`
- `fix(patch): correct thenable snapshot for Chromium V8`
- `fix(patch): skip AES-KW tests with BoringSSL`

Group related test fixes into a single commit when they address the same root cause (e.g., multiple crypto tests all needing BoringSSL guards for the same missing cipher). Don't create one commit per test file if they share the same fix pattern.

### Snapshot regeneration

When a snapshot test fails because Chromium's V8 produces different output, regenerate it:

```bash
NODE_REGENERATE_SNAPSHOTS=1 node script/node-spec-runner.js test/test-runner/test-foo.mjs
```

Then commit the updated snapshot patch with a title describing what changed:

```
fix(patch): correct {name} snapshot for Chromium V8

Ref: {V8 CL or issue link if known}

Co-Authored-By: <AI model attribution>
```

### Trivial patch updates

After any patch modification, check for dependent patches that only have index/hunk header changes:

```bash
git status
# If other .patch files show as modified with only trivial changes:
git add patches/
git commit -m "chore: update patches (trivial only)"
```

## Finding References

For BoringSSL-related test fixes, the reference is typically the upstream Node.js PR that added the new test:

```bash
cd ../third_party/electron_node
git log --oneline -5 -- test/parallel/test-crypto-foo.js
git log -1 <commit> --format="%B" | grep "PR-URL"
```

For V8 behavioral differences, reference the Chromium CL:

```
Ref: https://chromium-review.googlesource.com/c/v8/v8/+/NNNNNNN
```

If no reference found after searching: `Ref: Unable to locate reference`
