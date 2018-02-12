# Upgrading Crashpad

1. Get the version of crashpad that we're going to use.
  - `libcc/src/third_party/crashpad/README.chromium` will have a line `Revision:` with a checksum
    - We need to check out the corresponding branch.
  - Clone Google's crashpad (https://chromium.googlesource.com/crashpad/crashpad)
    - `git clone https://chromium.googlesource.com/crashpad/crashpad`
  - Check out the branch with the revision checksum:
      - `git checkout <revision checksum>`
  - Add electron's crashpad fork as a remote
    - `git remote add electron https://github.com/electron/crashpad`
  - Check out a new branch for the update
    - `git checkout -b electron-crashpad-vA.B.C.D`
    - `A.B.C.D` is the Chromium version found in `libcc/VERSION`
       and will be something like `62.0.3202.94`

2. Make a checklist of the Electron patches that need to be applied
  with `git log --oneline`
    - Or view https://github.com/electron/crashpad/commits/previous-branch-name

3. For each patch:
  - In `electron-crashpad-vA.B.C.D`, cherry-pick the patch's checksum
    - `git cherry-pick <checksum>`
  - Resolve any conflicts
  - Make sure it builds then add, commit, and push work to electron's crashpad fork
    - `git push electron electron-crashpad-vA.B.C.D`

4. Update Electron to build the new crashpad:
  - `cd vendor/crashpad`
  - `git fetch`
  - `git checkout electron-crashpad-v62.0.3202.94`
5. Regenerate Ninja files against both targets
  - From Electron root's root, run `script/update.py`
  - `script/build.py -c D --target=crashpad_client`
  - `script/build.py -c D --target=crashpad_handler`
  - Both should build with no errors
6. Push changes to submodule reference
  - (From electron root) `git add vendor/crashpad`
  - `git push origin upgrade-to-chromium-62`
