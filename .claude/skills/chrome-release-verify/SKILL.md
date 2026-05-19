---
name: chrome-release-verify
description: End-to-end Chrome security backport for an Electron release branch. Given a Chrome Releases blog URL and a branch (e.g. 41-x-y), determines which CVE fixes are missing from the *actual synced source*, writes the cherry-pick patches locally, validates them with `e sync --3` + `lint --patches`, then pushes a single PR. Use when asked to backport a Chrome security release to N-x-y, "is CVE-X already in N-x-y?", or to produce/validate the cherry-pick set for a release branch.
---

# Chrome Release → Validated Backport PR

Input: `$ARGUMENTS` = `<release-branch> <chrome-releases-blog-url>` (e.g. `41-x-y https://chromereleases.googleblog.com/2026/04/stable-channel-update-for-desktop_15.html`). Ask if either is missing.

The flow is **local-first**: nothing is pushed until every patch applies via `e sync --3` and passes `lint --patches`.

## 1. Map CVE → bug → fix CL

Run `/chrome-release-cls <blog-url>` (or its inline procedure) to produce `/tmp/cve_bugs.txt` (`CVE|bug|severity|desc`) and a per-bug canonical fix CL. For each CL also note `repo` (path under `src/`: `.`, `v8`, `third_party/{skia,angle,pdfium,dawn}`, `third_party/libaom/source/libaom`) and `gerrit-host`.

**Prefer the target-milestone merge CL** if one exists (e.g. on `41-x-y` ≈ M146, prefer the `[M146]` cherry-pick over the main CL) — it's already rebased and far less likely to conflict. Find it via `git log --all --grep` on the Change-Id, or Gerrit `?q=bug:<n>`. If Chrome did *not* merge a fix to the target milestone, that's a strong signal the vulnerable code doesn't exist there — flag it for skip rather than forcing a port.

## 2. Prepare a synced worktree

Reuse `bp-<NN>` from `e show configs` if present, else `e worktree add bp-<NN> ~/src/electron-bp-<NN> --source <current> --no-sync`.

```bash
cd <root>/src/electron
git fetch origin <branch>
git checkout -B security-backport/<branch>/<short-date> origin/<branch>
e use bp-<NN>
e sync 2>&1 | tee /tmp/bp_sync.log
```

If sync fails with `NotADirectoryError: '<root>/src/.git/objects/info/alternates'`, remove `GIT_CACHE_PATH` from the bp config's `env` and retry.

## 3. Verify IN-TREE vs NEEDS-BACKPORT

For each bug, three checks against the **synced** repo:

1. `git -C "$repo" log HEAD --since='1 year ago' -E --grep="\b${bug}\b" --format='%h %s'`
2. Fetch Change-Id from Gerrit, then `git log HEAD --grep="^Change-Id: ${cid}$"`
3. `grep -rlE "(\b${bug}\b|${cid})" <root>/src/electron/patches/`

Any hit ⇒ IN-TREE. All empty ⇒ NEEDS-BACKPORT.

For each NEEDS-BACKPORT CL, also fetch its file list (`/changes/<proj>~<cl>/revisions/current/files`) and **skip** if every file is under `chrome/browser/`, `chrome/android/`, `ios/`, or `components/**/android/` — Electron doesn't compile those.

Report the table now (`CVE | Sev | Bug | Component | Verdict | CL`) and the proposed backport set; get user sign-off before continuing.

## 4. Write patches locally (no push yet)

For each backport CL, fetch the raw patch and write it into `patches/<dir>/`:

```bash
curl -s "https://${host}.googlesource.com/changes/${proj//\//%2F}~${cl}/revisions/current/patch" \
  | base64 -d > "patches/${dir}/cherry-pick-${short}.patch"
echo "cherry-pick-${short}.patch" >> "patches/${dir}/.patches"
```

For repos with no Gerrit host `e cherry-pick` supports (e.g. **libaom** on aomedia), instead `git cherry-pick` the upstream commits onto the synced sub-repo HEAD and `git format-patch` the result.

For any newly-created `patches/<dir>/`, append to `patches/config.json` **preserving the compact one-line-per-entry style**:

```json
  { "patch_dir": "src/electron/patches/<dir>", "repo": "src/third_party/<dir-or-nested-path>" }
```

## 5. Validate with `e sync --3`

```bash
e sync --3 2>&1 | tee /tmp/bp_sync3.log
```

On `Patch failed at NNNN <subject>`:

- `cd` into the failing repo, inspect `git diff` for conflict markers.
- **Test-only files** (e.g. `web_tests/VirtualTestSuites`, `*_unittest.cc` context drift): take ours (`git checkout --ours -- <file>`) if the security-relevant hunks merged cleanly.
- **Substantive code conflicts**: check whether a target-milestone merge CL exists and swap to it. If none exists upstream and the surrounding code is structurally different, **drop the patch** (delete the file, remove from `.patches` and `config.json`) and note it for a separate manual-port PR — do not improvise security-fix semantics.
- After resolving: `git add <files> && git -c commit.gpgsign=false am --continue`, then `e patches <repo>` to export the resolved patch, then re-run `e sync --3`. Repeat until clean.

## 6. Export → lint → re-apply loop

```bash
e patches all
node script/lint.js --patches   # must exit 0
```

If lint reports findings (typically trailing whitespace on `+` content lines), fixing them **changes the bytes the patch writes**, which invalidates the `index <old>..<new>` blob hashes that `e patches` baked in. Hand-editing a `.patch` and pushing it as-is will pass lint locally but fail CI's Apply Patches re-export check with a one-line `index` hash diff.

So whenever lint (or you) modifies any `.patch` file after export, round-trip once more:

```bash
# fix the lint findings in patches/**/*.patch, then:
e sync            # re-apply the edited patches (no --3 needed; they applied cleanly last time)
e patches all     # re-export so index blob hashes match the edited content
node script/lint.js --patches      # must now exit 0
git diff --quiet -- patches/ || { echo "patches changed again — repeat the loop"; }
```

Repeat until `lint --patches` exits 0 **and** `git diff -- patches/` is empty after the final `e patches all`. Only then is the patch set CI-stable.

## 7. Commit, push, PR

```bash
git add patches/
git commit -m "chore: cherry-pick <N> changes from <dirs>"
git push origin HEAD
gh pr create --repo electron/electron --base <branch> --head <this-branch> \
  --title "chore: cherry-pick <N> changes from <dirs>" \
  --label "<branch>" --label backport-check-skip --label semver/patch --label "security 🔒" \
  --body-file /tmp/pr_body.md
```

PR body format:

```markdown
Backports the following changes:

* [`<shortCommit>`](<gerrit-CL-url>) from <patchDir> — <subject> ([<bug>](https://crbug.com/<bug>), CVE-YYYY-NNNN)
* ...

Notes: Security: backported fixes for CVE-YYYY-NNNN, CVE-YYYY-NNNN, ....
```

Short commit links to the **Gerrit CL**; bug links to `crbug.com`; CVE comes from the blog mapping (the patch's own `Bug:` footer may differ); `Notes:` is the last line. Mention any dropped patches (with reason) above the `Notes:` line.

Restore `e use <previous>` when done.
