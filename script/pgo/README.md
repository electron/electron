# Electron PGO profile generation

This directory contains the infrastructure for generating Electron-specific
PGO (profile-guided optimization) profiles. Two kinds of profile are
generated:

1. **C++ binary profiles** (`.profdata`) - guide clang's optimization of all
   compiled C++ (Chromium, V8's C++, Node.js, the Electron shell).
2. **V8 builtins profiles** (`.profile`) - guide mksnapshot's basic-block
   ordering of the V8 builtins baked into the snapshot (property access ICs,
   `Array.prototype.*`, promise machinery, etc.).

## Why Electron needs its own profiles

Electron release builds use `chrome_pgo_phase = 2`, which applies a PGO
profile during compilation. Today that profile is the one Chromium publishes
for Chrome (downloaded by `tools/update_pgo_profiles.py`). That profile is
collected on Chrome's binary - every function that differs in Electron
(patched Chromium code, all of Node.js, the Electron shell, V8 built with
Node's flags) silently loses profile guidance.

Measured on Speedometer 3.1 (Linux x64), replacing Chrome's profile with an
Electron-collected profile is worth roughly **+9%**.

The same problem applies to V8 builtins: Chrome's published builtins profiles
reject Electron's promise/async builtins (`RunMicrotasks`,
`AsyncFunctionAwait`, `FulfillPromise`, `PromiseConstructor`, ...) because
Electron's Node.js integration changes their code generation
(`v8_promise_internal_field_count = 1`,
`v8_enable_javascript_promise_hooks = true`). An Electron-generated builtins
profile covers every builtin with zero rejections.

## How profiles are generated

The process mirrors Chromium's own PGO recipe
(`tools/pgo/generate_profile.py`), extended with Electron-specific workloads:

1. Build an **instrumented** Electron: `build/args/pgo-instrument.gn`
   (`chrome_pgo_phase = 1`).
2. Run `script/pgo/collect-profile.js`, which:
   - serves Speedometer 3, JetStream 2, and MotionMark from a local
     HTTPS/HTTP-2 server (pinned revisions, no external network during
     collection; an ephemeral CA is installed into the host trust store so
     the real certificate verification path is trained);
   - launches the instrumented build with `LLVM_PROFILE_FILE` set and drives
     every workload via `script/pgo/benchmark-app`:
     - **browser benchmarks** - Speedometer 3 (x3), JetStream 2, MotionMark
       (the same set Chromium's PGO bots run);
     - **main-process workload** - Node.js Buffer/crypto/fs/JSON loops.
       Browser benchmarks never execute Node-only native code, which would
       otherwise be cold-marked by the profile (measured at -63% on Buffer
       operations without this phase);
     - **IPC + contextBridge workload** - bridge marshaling and
       `ipcRenderer.invoke` round trips at multiple payload sizes;
     - **network workload** - renderer fetch/WebSocket loops plus Node-side
       HTTPS, training Chromium's TLS/H2/network-service code and Node's
       separate TLS/HTTP stack;
   - relies on a clean `app.quit()` so every Electron process writes its
     counters (killed processes write nothing);
   - merges the resulting `.profraw` files with `llvm-profdata` into a single
     `.profdata`.
3. Merge and upload. In CI, collection jobs produce raw `.profraw` artifacts
   (collection hosts can't always run the x64 LLVM tooling - e.g. Linux arm64
   runners); the `merge-profiles` job then merges each platform/arch's set
   into a `.profdata` using the in-tree llvm-profdata tooling (the merge job
   restores the Linux src cache and runs Chromium's standard
   `tools/clang/scripts/update.py --package=coverage_tools`), and the
   `upload-profiles` job authenticates to Azure with OIDC through the
   `pgo-upload` GitHub environment and uploads to the `pgo` container of the
   build-tools storage account. Uploaded profiles are served at
   `https://dev-cdn.electronjs.org/pgo/<profile-name>`.

## How V8 builtins profiles are generated

The process mirrors upstream V8's recipe (`v8/tools/builtins-pgo/generate.py`):

1. Build **d8** with Electron's exact V8 configuration plus builtins
   profiling: `build/args/pgo-builtins-instrument.gn`
   (`v8_enable_builtins_profiling = true`). Configuration matching is
   mandatory - builtin block-graph hashes depend on V8 codegen flags, and a
   profile generated with the wrong configuration is rejected by mksnapshot.
2. Run JetStream 2 under d8 with `--turbo-profiling-output=<log>` (the same
   workload upstream V8 uses for its profiles).
3. Convert the log to block hints with `v8/tools/builtins-pgo/get_hints.py`.

Builtin block graphs are arch-independent, so - mirroring upstream V8, where
arm64 release builds consume the x64 profile - one x64-generated profile
covers Electron's x64 and arm64 builds. 32-bit targets (win-x86, linux-arm)
keep using upstream's x86 profile until 32-bit generation is added.

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

For the V8 builtins profile:

```sh
# 1. Build instrumented d8
e init pgo-builtins --root=$PWD --import pgo-builtins-instrument
e build --target v8:d8

# 2. Run JetStream 2 under d8 and convert to block hints
git clone --depth 1 https://github.com/WebKit/JetStream.git /tmp/jetstream
cd /tmp/jetstream
<out>/d8 --no-sandbox-prohibit-insecure-mode \
  --turbo-profiling-output=/tmp/v8.builtins.log cli.js
python3 <src>/v8/tools/builtins-pgo/get_hints.py \
  /tmp/v8.builtins.log electron-v8-x64.profile
```

## Using generated profiles

Point release builds at the profiles with:

```gn
# C++ binary profile
pgo_data_path = "//path/to/electron-<platform>-<arch>-....profdata"

# V8 builtins profile (must be an in-tree path for remote-exec builds)
v8_builtins_profiling_log_file = "//v8/tools/builtins-pgo/profiles/electron-v8-x64.profile"
```

## Platform notes

Profiles are generated for every published Electron platform/arch - each
combo's instrumented build runs its own benchmarks:

| Profile | Collection host |
|---|---|
| linux-x64 | x64 ARC runner (build container) |
| linux-arm64 | `ubuntu-22.04-arm` (arm64v8 test container) |
| win-x64 | `windows-latest` |
| win-x86 | `windows-latest` (WOW64) |
| win-arm64 | `windows-11-arm` |
| macos-x64 | `macos-15-large` |
| macos-arm64 | `macos-15` |
