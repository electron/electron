# Build Instructions (macOS)

Follow the guidelines below for building **Electron itself** on macOS, for the purposes of creating custom Electron binaries. For bundling and distributing your app code with the prebuilt Electron binaries, see the [application distribution][application-distribution] guide.

[application-distribution]: ../tutorial/application-distribution.md

## Prerequisites

* macOS >= 11.6.0
* [Xcode](https://developer.apple.com/technologies/tools/). The exact version
  needed depends on what branch you are building, but the latest version of
  Xcode is generally a good bet for building `main`.
* [node.js](https://nodejs.org) (external)
* Python >= 3.7

### Arm64-specific prerequisites

* Rosetta 2
  * this can be installed by using the softwareupdate command line tool.
  * `$ softwareupdate --install-rosetta`

## Building Electron

See [Build Instructions: GN](build-instructions-gn.md).

## Troubleshooting

### Xcode is complaining about incompatible architecture (MacOS arm64-specific)

If both Xcode and Xcode command line tools are installed (`$ xcode -select --install`, or directly download the correct version [here](https://developer.apple.com/download/all/?q=command%20line%20tools)), but the stack trace says otherwise like so:

```sh
xcrun: error: unable to load libxcrun
(dlopen(/Users/<user>/.electron_build_tools/third_party/Xcode/Xcode.app/Contents/Developer/usr/lib/libxcrun.dylib (http://xcode.app/Contents/Developer/usr/lib/libxcrun.dylib), 0x0005):
 tried: '/Users/<user>/.electron_build_tools/third_party/Xcode/Xcode.app/Contents/Developer/usr/lib/libxcrun.dylib (http://xcode.app/Contents/Developer/usr/lib/libxcrun.dylib)'
 (mach-o file, but is an incompatible architecture (have (x86_64), need (arm64e))), '/Users/<user>/.electron_build_tools/third_party/Xcode/Xcode-11.1.0.app/Contents/Developer/usr/lib/libxcrun.dylib (http://xcode-11.1.0.app/Contents/Developer/usr/lib/libxcrun.dylib)' (mach-o file, but is an incompatible architecture (have (x86_64), need (arm64e)))).`
```

If you are on arm64 architecture, the build script may be pointing to the wrong Xcode version (11.x.y doesn't support arm64). Navigate to `/Users/<user>/.electron_build_tools/third_party/Xcode/` and rename `Xcode-13.3.0.app` to `Xcode.app` to ensure the right Xcode version is used.

### I'm running into a vpython error resulting in a failed goroutine

```sh
Updating depot_tools...
[E2022-08-08T15:10:18.034202-07:00 18261 0 annotate.go:273] original error: permission denied

goroutine 1:
#0 go.chromium.org/luci/vpython/venv/venv.go:316 - venv.(*Env).ensure()
  reason: failed to ensure VirtualEnv

#1 go.chromium.org/luci/vpython/venv/venv.go:160 - venv.With()
  reason: failed to create empty probe environment

#2 go.chromium.org/luci/vpython/run.go:60 - vpython.Run()
```

This error can be solved by uninstalling build-tools, running `xcode-select -r`, reinstalling build-tools, and finally clearing your `gcache` with `rm -rf ~/.git_cache`
