- Get the version of crashpad that we're going to use.
  - `libcc/src/third_party/crashpad/README.chromium` will have a line `Revision:` with a checksum.
  - clone [Electron's Crashpad fork](https://github.com/electron/crashpad) and create a new branch
  - `git clone https://chromium.googlesource.com/crashpad/crashpad`
  - `git checkout 01110c0a3b`
  - `git remote add electron https://github.com/electron/crashpad`
  - `git checkout -b electron-crashpad-vA.B.C.D`
     - `A.B.C.D` is the Chromium version found in `libcc/VERSION`
       and will be something like `62.0.3202.94`

- Make a checklist of the Electron patches we need to apply
  e.g. `git log --oneline`
  or view http://github.com/electron/crashpad/commits/previous-branch-name
- Foreach patch:
  - (in new branch) `git cherry-pick checksum`
  - resolve any conflicts
  - make sure it builds
  - add && commit
- Push your work:
  `git push electron electorn-crashpad-v62.0.3202.94`

- Update Electron to build the new crashpad:
  - `cd vendor/crashpad`
  - `git fetch`
  - `git checkout electron-crashpad-v62.0.3202.94`
- Regenerate Ninja files
  - from Electron root's root, run `script/update.py`
  - `script/build.py -c D --target=crashpad_client`
  - `script/build.py -c D --target=crashpad_handler`
  - If both of these work, it's probably good.
- Push changes to submodule reference
  - (from electron root) `git add vendor/crashpad`
  - `git push origin upgrade-to-chromium-62`
  


