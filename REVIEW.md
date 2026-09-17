# Review instructions

Instructions for automated review of pull requests to electron/electron. The
single most important rule: **only make claims you can support from files you
can actually read** — and what you can read depends on your checkout.

## First, determine what you can see

This repository is only the Electron layer. For a real build it is checked
out as `src/electron/` inside a much larger tree that `gclient` (`e sync`)
assembles from the pins in `DEPS`: Chromium at `src/`, Node.js at
`src/third_party/electron_node/`, V8, BoringSSL, …. Before reviewing, check:

- **Full checkout:** the repository's parent directory is a Chromium `src`
  tree: `../content/`, `../chrome/`, `../v8/` and
  `../third_party/electron_node/` all exist relative to the repo root.
- **Standalone clone:** those paths do not exist; you have electron/electron
  and nothing else. If in doubt, or only some exist, assume this.

Follow only whichever of the next two sections matches; everything from
"`patches/`" onward applies to both. Either way, generated headers (`*.mojom.h`
and friends, `electron/buildflags/buildflags.h`, other `electron/…` paths made
from templates here) exist only in build output (`../out/…`), never in the
source tree; if no build output is present they won't be either, so never
report them as missing.

## If you have a standalone clone

None of the upstream code is present. `#include "shell/..."` paths are files
in this repository; everything else (`base/`, `content/`, `chrome/`,
`third_party/blink/`, `v8/`, `gin/`, `mojo/`, Node's headers, …) is upstream
code you cannot see — including the `//chrome` sources `chromium_src/BUILD.gn`
compiles. Only `//electron/…` GN targets (and relative `:name` labels in this
repo's BUILD files) are defined here. So:

- Do not state what a Chromium, Node.js or V8 function does, what its
  signature or threading/ownership contract is, whether an override matches a
  virtual method, or that a call site here misuses an upstream API, unless the
  evidence is in this repo's files or in the PR diff itself — likewise for
  Node.js internals (`internal/…` modules, the `fs`/`module` code ASAR wraps).
- When a concern genuinely depends on upstream behaviour, ask the author ("I
  can't see `content::WebContentsObserver` from this checkout — can you confirm
  this runs on the UI thread?") at Nit severity at most, never as a defect.
- In `patches/`, do **not** claim a hunk will fail to apply, conflicts with
  upstream, targets the wrong line or is no longer needed — you cannot see the
  upstream file; CI applies every patch to the pinned revisions. Ask instead.

## If you have a full checkout

The previous section's restrictions do not apply to you, as long as the tree
is synced to this PR's `DEPS` pins (check when the PR changes `DEPS`; if
unsure, treat the rolled dependency as code you cannot read). Use the tree:

- Verify directly in the synced tree (`../`, `../v8/`,
  `../third_party/electron_node/`, …) the upstream signatures, the virtual
  methods `shell/` overrides, threading/ownership contracts and the Node.js
  internals `lib/` wraps; report real mismatches as findings.
- For `patches/` changes, open the upstream file each hunk targets and check
  the hunk against it. A synced tree normally already has the existing
  `patches/` applied: a new patch should apply to what you see, but for a patch
  the PR modifies you are looking at upstream with its old version applied —
  judge the new hunks against that. Then a hunk that cannot apply, or a patch
  upstream no longer needs, is a real finding here.
- Cite upstream evidence by path relative to the Chromium `src` root (e.g.
  `content/browser/…:123`) so it is clear which tree a claim rests on. You
  still never propose editing upstream files — such changes go via `patches/`.

## `patches/`

Each subdirectory of `patches/` (`chromium/`, `node/`, `v8/`, …) holds
git-format patches applied, in the order of its `.patches` file, on top of the
matching upstream checkout (mapping in `patches/config.json`). They are
exported by tooling (`e patches` / `script/git-export-patches`), not written
by hand. Review a patch change as a diff of a diff:

- A new `.patch` needs a subject and a body that meaningfully explains why
  Electron needs it and, if it is meant to be temporary, an upstream link or
  how to tell when it can be dropped (see `docs/development/patches.md`; Nit
  if empty or boilerplate). It must be added to that directory's `.patches`
  list; a deleted patch must be removed from it.
- Check whether the `+`/`-` lines inside the patch plausibly do what the
  description says, and whether Electron-side code in `shell/` or `lib/` that
  relies on the patch changes consistently in the same PR.
- Churn in context lines, `index` hashes and line offsets of otherwise
  untouched patches is normal re-export noise; ignore it. Upstream behaviour
  changes belong in `patches/`; never suggest editing files under `../`.

## What a change here usually needs (checkable in this repo)

- `shell/` (C++) and `lib/` (TypeScript bundled into the binary) are split by
  process type (`browser/` = main process, `renderer/`, `sandboxed_renderer/`,
  `utility/`, `common/`, …). Check that new code sits in the directory for the
  process it runs in, and be careful about what becomes reachable from a
  sandboxed or context-isolated renderer. New C++ sources in `shell/` are
  normally listed in `filenames.gni`; if a PR adds one and no `.gn`/`.gni`
  file in the diff references it, mention it once.
- In `lib/`, `@electron/internal/…` imports resolve to `lib/…`, and
  `process._linkedBinding('electron_…')` objects are C++ under `shell/`, typed
  by hand in `typings/` (`internal-ambient.d.ts`, `internal-electron.d.ts`) —
  check the two against each other.
- Public API lives in `docs/api/*.md`; `electron.d.ts` is generated from them
  at build time and not checked in. A PR that adds or changes a public method,
  event, property or option should update `docs/api/` (types, platform tags,
  and the YAML "API history" block when an API is added, changed or
  deprecated) and `docs/breaking-changes.md` for deprecations and breakage.
- Behaviour changes and bug fixes usually come with a `spec/` test; if there is
  none, ask whether one is feasible rather than asserting it is required (some
  platform- or GPU-specific behaviour cannot be tested in CI).
- Treat IPC handlers in `lib/browser/` and `shell/browser/api/`, anything
  touching `contextBridge`, `webPreferences` defaults (`contextIsolation`,
  `sandbox`, `nodeIntegration`), navigation/permission/protocol handlers,
  fuses and ASAR integrity as security-sensitive: validate arguments that come
  from a renderer, check which process and world the code runs in, and cite
  the exact lines. No speculative security findings about code you can't read.
- PRs based on a release line (`NN-x-y`) are normally backports from `main`,
  often opened by the backport bot: review them for faithful application to
  the older branch (missing hunks, code that differs on that branch), not to
  re-argue the original design.

## Post no findings on

- Formatting, include order and lint (CI runs clang-format, ESLint, docs lint,
  GN format and patch-file lint); `filenames.auto.gni` (generated and
  CI-checked even if the PR touches it), lockfiles, generated/vendored files.
- PR-description conventions (release-notes `Notes:` line, "Backport of #…"):
  bots already enforce them.
- In `DEPS` roll PRs ("chore: bump chromium to …" / "… node to …"), the version
  bump itself and what changed upstream between the two versions; their
  `shell/`, `lib/`, `patches/` and build changes are reviewed normally.

## Evidence bar and tone

- Every Important finding needs a `path:line` citation to a file you actually
  read — in this repository, or in the synced tree if you have one — and a
  concrete failure mode. "This might be wrong depending on how Chromium
  handles X" is not a finding; it is at most a question. For anything you
  cannot read, prefer "I can't see X from this checkout — can you confirm Y?";
  if you cannot name what you would need to see to be sure, drop the comment.
- Keep reviews short: at most five Nits, the rest as a count in the summary.
  On re-review after a push, report Important findings and whether earlier
  ones were addressed; no new Nits. Lead the summary with which kind of
  checkout you reviewed from and whether you found anything you could verify.
