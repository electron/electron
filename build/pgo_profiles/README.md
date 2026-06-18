# Electron PGO profiles

Electron release builds are optimized with Electron-collected PGO profiles
instead of Chrome's published ones. PGO profiles match functions by symbol
name and control-flow hash, so Chrome's profiles cannot cover code that
differs in Electron - all of Node.js, patched Chromium files, V8 built with
Node's flags, and the Electron shell - and uncovered functions are laid out
as cold.

Profiles are generated and uploaded by Electron's PGO generation pipeline and
served from `https://dev-cdn-experimental.electronjs.org/pgo/`.

## How consumption works

1. **State files** - `<target>.pgo.txt` in this directory names the profile
   each target uses (e.g. `electron-linux-x64-<timestamp>-<sha>.profdata`).
   Updating a profile means updating the state file; profile binaries are
   never checked in.
2. **Download** - gclient hooks (see `DEPS`) run
   `script/pgo/download-profiles.py`, which downloads each state file's
   profile into this directory under its versioned name (the V8 builtins
   profile uses the fixed name `electron-v8-builtins.profile`, since GN args
   files reference it statically).
3. **Build wiring** - Chromium's standard `chrome_pgo_phase = 2` machinery
   applies the profile. A small patch
   (`patches/chromium/build_resolve_pgo_profiles_from_electron_state_files.patch`)
   teaches its profile resolution to read Electron's state files - including
   per-arch Linux profiles, which upstream does not have - so every compiler
   and linker flag upstream maintains for PGO (`-fprofile-use`, warning
   suppressions, extended-TSP block layout, and anything they add in the
   future) applies to Electron's profiles unchanged.

`chrome_pgo_phase` already defaults to 2 for official builds, so no
per-platform configuration is needed - any release build on any platform/arch
picks up its profile automatically:

```sh
e init my-release --root=$PWD --import release
e build
```

The V8 builtins profile is wired separately (it is consumed by mksnapshot,
not clang): `release.gn` points `v8_builtins_profiling_log_file` at the
downloaded Electron builtins profile.

## Fallbacks

- Build against Chrome's published profiles: set the `pgo_data_path` GN arg
  to a Chrome profile (explicitly set paths take precedence over Electron's
  state-file resolution) and set `checkout_pgo_profiles=True` in gclient
  custom vars so Chrome's profile is downloaded.
- Build without PGO entirely: `chrome_pgo_phase = 0`.

## Platform notes

| Target | C++ profile | V8 builtins profile |
|---|---|---|
| linux-x64, linux-arm64 | yes | yes (64-bit profile) |
| macos-x64, macos-arm64 | yes | yes (64-bit profile) |
| win-x64, win-arm64 | yes | yes (64-bit profile) |
| linux-arm, win-x86 | yes | no - Electron only generates a 64-bit builtins profile; the mismatch is skipped (mksnapshot warns rather than aborts) |
