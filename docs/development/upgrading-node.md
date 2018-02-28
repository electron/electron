# Upgrading Node

## Discussion

One upgrade issue is building all of Electron with a single copy
of V8 to ensure compatibility. This is important because
upstream Node and [libchromiumcontent](upgrading-chromium.md)
both use their own versions of V8.

Upgrading Node is much easier than upgrading libchromiumcontent,
so fewer conflicts arise if one upgrades libchromiumcontent first,
then chooses the upstream Node release whose V8 is closest to it.

Electron has its own [Node fork](https://github.com/electron/node)
with modifications for the V8 build details mentioned above
and for exposing API needed by Electron. Once an upstream Node
release is chosen, it's placed in a branch in Electron's Node fork
and any Electron Node patches are applied there.

Another factor is that the Node project patches its version of V8.
As mentioned above, Electron builds everything with a single copy
of V8, so Node's V8 patches must be ported to that copy.

Once all of Electron's dependencies are building and using the same
copy of V8, the next step is to fix any Electron code issues caused
by the Node upgrade.

[FIXME] something about a Node debugger in Atom that we (e.g. deepak)
use and need to confirm doesn't break with the Node upgrade?

So in short, the primary steps are:

1. Update Electron's Node fork to the desired version
2. Backport Node's V8 patches to our copy of V8
3. Update Electron to use new version of Node
  * Update submodules
  * Update Node.js build configuration

## Updating Electron's Node [fork](https://github.com/electron/node)

1. Ensure that `master` on `electron/node` has updated release tags from `nodejs/node`
2. Create a branch in https://github.com/electron/node: `electron-node-vX.X.X` where the base that you're branching from is the tag for the desired update
  - `vX.X.X` Must use a version of node compatible with our current version of chromium
3. Re-apply our commits from the previous version of node we were using (`vY.Y.Y`) to `v.X.X.X`
  - Check release tag and select the range of commits we need to re-apply
  - Cherry-pick commit range:
    1. Checkout both `vY.Y.Y` & `v.X.X.X`
    2. `git cherry-pick FIRST_COMMIT_HASH..LAST_COMMIT_HASH`
  - Resolve merge conflicts in each file encountered, then:
    1. `git add <conflict-file>`
    2. `git cherry-pick --continue`
    3. Repeat until finished


## Updating [V8](https://github.com/electron/node/src/V8) Patches

We need to generate a patch file from each patch applied to V8.

1. Get a copy of Electron's libcc fork
  - `$ git clone https://github.com/electron/libchromiumcontent`
2. Run `script/update` to get the latest libcc
  - This will be time-consuming
3. Remove our copies of the old Node v8 patches
  - (In libchromiumcontent repo) Read `patches/v8/README.md` to see which patchfiles
    were created during the last update
  - Remove those files from `patches/v8/`:
    - `git rm` the patchfiles
    - edit `patches/v8/README.md`
    - commit these removals
4. Inspect Node [repo](https://github.com/electron/node) to see what patches upstream Node
  used with their v8 after bumping its version
  - `git log --oneline deps/V8`
5. Create a checklist of the patches. This is useful for tracking your work and for
  having a quick reference of commit hashes to use in the `git diff-tree` step below.
6. Read `patches/v8/README.md` to see which patchfiles came from the previous version of V8 and therefore need to be removed.
  - Delete each patchfile referenced in `patches/v8/README.md`
7. For each patch, do:
  - (In node repo) `git diff-tree --patch HASH > ~/path_to_libchromiumcontent/patches/v8/xxx-patch_name.patch`
    - `xxx` is an incremented three-digit number (to force patch order)
    - `patch_name` should loosely match the node commit messages,
      e.g. `030-cherry_pick_cc55747,patch` if the Node commit message was "cherry-pick cc55747"
  - (remainder of steps in libchromium repo)
     Manually edit the `.patch` file to match upstream V8's directory:
    - If a diff section has no instances of `deps/V8`, remove it altogether.
      - We don’t want those patches because we’re only patching V8.
    - Replace instances of `a/deps/v8/filename.ext` with `a/filename.ext`
      - This is needed because upstream Node keeps its V8 files in a subdirectory
  - Ensure that local status is clean: `git status` to make sure there are no unstaged changes.
  - Confirm that the patch applies cleanly with
     `script/patch.py -r src/V8 -p patches/v8/xxx-patch_name.patch.patch`
  - Create a new copy of the patch:
     - `cd src/v8 && git diff > ../../test.patch && cd ../..`
     - This is needed because the first patch has Node commit checksums that we don't want
  - Confirm that checksums are the only difference between the two patches:
     - `diff -u test.patch patches/v8/xxx-patch_name.patch`
  - Replace the old patch with the new:
     - `mv test.patch patches/v8/xxx-patch_name.patch`
  - Add the patched code to the index _without_ committing:
     - `cd src/v8 && git add . && cd ../..`
     - We don't want to commit the changes (they're kept in the patchfiles)
        but need them locally so that they don't show up in subsequent diffs
        while we iterate through more patches
  - Add the patch file to the index:
     - `git add a patches/v8/`
  - (Optionally) commit each patch file to ensure you can back up if you mess up a step:
     - `git commit patches/v8/`
8. Update `patches/v8/README.md` with references to all new patches that have been added so that the next person will know which need to be removed.
9. Update Electron's submodule references:
  ```sh
  $ cd electron/vendor/node
  electron/vendor/node$ git fetch
  electron/vendor/node$ git checkout electron-node-vA.B.C
  electron/vendor/node$ cd ../libchromiumcontent
  electron/vendor/libchromiumcontent$ git fetch
  electron/vendor/libchromiumcontent$ git checkout upgrade-to-chromium-X
  electron/vendor/libchromiumcontent$ cd ../..
  electron$ git add vendor
  electron$ git commit -m "update submodule referefences for node and libc"
  electron$ git pso upgrade-to-chromium-62
  electron$ script/bootstrap.py -d
  electron$ script/build.py -c -D
  ```

## Notes

- libcc and V8 are treated as a single unit
- Node maintains its own fork of V8
  - They backport a small amount of things as needed
  - Documentation in node about how [they work with V8](https://nodejs.org/api/v8.html)
- We update code such that we only use one copy of V8 across all of electron
  - E.g electron, libcc, and node
- We don’t track upstream closely due to logistics:
   - Upstream uses multiple repos and so merging into a single repo
   would result in lost history. So we only update when we’re planning
   a node version bump in electron.
- libcc is large and time-consuming to update, so we typically
  choose the node version based on which of its releases has a version
  of V8 that’s closest to the version in libcc that we’re using.
  - We sometimes have to wait for the next periodic Node release
   because it will sync more closely with the version of V8 in the new libcc
 - Electron keeps all its patches in libcc because it’s simpler than
   maintaining different repos for patches for each upstream project.
   - Crashpad, node, libcc, etc. patches are all kept in the same place
 - Building node:
   - There’s a chance we need to change our build configuration
   to match the build flags that node wants in `node/common.gypi`
