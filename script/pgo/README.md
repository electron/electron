# Electron PGO profile generation

This directory contains the infrastructure for generating Electron-specific
PGO (profile-guided optimization) profiles.

## Why Electron needs its own profiles

Electron release builds use `chrome_pgo_phase = 2`, which applies a PGO
profile during compilation. Today that profile is the one Chromium publishes
for Chrome (downloaded by `tools/update_pgo_profiles.py`). That profile is
collected on Chrome's binary - every function that differs in Electron
(patched Chromium code, all of Node.js, the Electron shell, V8 built with
Node's flags) silently loses profile guidance.

Measured on Speedometer 3.1 (Linux x64), replacing Chrome's profile with an
Electron-collected profile is worth roughly **+9%**.

## How profiles are generated

The process mirrors Chromium's own PGO recipe
(`tools/pgo/generate_profile.py`):

1. Build an **instrumented** Electron: `build/args/pgo-instrument.gn`
   (`chrome_pgo_phase = 1`).
2. Run `script/pgo/collect-profile.js`, which:
   - serves Speedometer 3, JetStream 2, and MotionMark from a local server
     (pinned revisions, no external network during collection);
   - launches the instrumented build with `LLVM_PROFILE_FILE` set and drives
     every workload via `script/pgo/benchmark-app`;
   - relies on a clean `app.quit()` so every Electron process writes its
     counters (killed processes write nothing);
   - merges the resulting `.profraw` files with `llvm-profdata` into a single
     `.profdata`.
3. Upload the merged profiles. In CI this is done by the `upload-profiles`
   job in the `pgo-generation` workflow: it authenticates to Azure with OIDC
   through the `pgo-upload` GitHub environment and uploads to the `pgo`
   container of the build-tools storage account. Uploaded profiles are served
   at `https://dev-cdn.electronjs.org/pgo/<profile-name>`.

## Running locally

```sh
# 1. Build the instrumented Electron (example with build-tools)
e init pgo-instrument --root=$PWD --import pgo-instrument
e build

# 2. Collect (Linux needs a display, e.g. under xvfb-run)
xvfb-run -a -s "-screen 0 1280x1024x24" \
  node electron/script/pgo/collect-profile.js \
    --electron out/Release/electron \
    --output out/Release/electron.profdata
```

The collection takes roughly 30-60 minutes (instrumented builds run at about
half speed).

## Using a generated profile

Point release builds at the profile with:

```
pgo_data_path = "//path/to/electron.profdata"
```

## Platform notes

- Profiles are platform-specific (and arch-specific on macOS/Windows where it
  matters). The CI workflow generates one profile per platform/arch it can
  run benchmarks on.
- Cross-compiled targets that cannot run their own binaries (e.g. Windows
  arm64 built on x64 runners) reuse the host-arch profile for their platform,
  matching what Chromium does for Linux.
