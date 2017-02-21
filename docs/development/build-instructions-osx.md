# Build Instructions (macOS)

Follow the guidelines below for building Electron on macOS.

## Prerequisites

* macOS >= 10.11.6
* [Xcode](https://developer.apple.com/technologies/tools/) >= 8.2.1
* [node.js](http://nodejs.org) (external)

If you are using the Python downloaded by Homebrew, you also need to install
the following Python modules:

* [pyobjc](https://pythonhosted.org/pyobjc/install.html)

## macOS SDK

If you're simply developing Electron and don't plan to redistribute your
custom Electron build, you may skip this section.

For certain features (e.g. pinch-zoom) to work properly, you must target the
macOS 10.10 SDK.

Official Electron builds are built with [Xcode 8.2.1](http://adcdownload.apple.com/Developer_Tools/Xcode_8.2.1/Xcode_8.2.1.xip), which does not contain
the 10.10 SDK by default. To obtain it, first download and mount the
[Xcode 6.4](http://developer.apple.com/devcenter/download.action?path=/Developer_Tools/Xcode_6.4/Xcode_6.4.dmg)
DMG.

Then, assuming that the Xcode 6.4 DMG has been mounted at `/Volumes/Xcode` and
that your Xcode 8.2.1 install is at `/Applications/Xcode.app`, run:

```bash
cp -r /Volumes/Xcode/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/
```

You will also need to enable Xcode to build against the 10.10 SDK:

- Open `/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Info.plist`
- Set the `MinimumSDKVersion` to `10.10`
- Save the file

## Getting the Code

```bash
$ git clone https://github.com/electron/electron
```

## Bootstrapping

The bootstrap script will download all necessary build dependencies and create
the build project files. Notice that we're using [ninja](https://ninja-build.org/)
to build Electron so there is no Xcode project generated.

```bash
$ cd electron
$ ./script/bootstrap.py -v
```

## Building

Build both `Release` and `Debug` targets:

```bash
$ ./script/build.py
```

You can also only build the `Debug` target:

```bash
$ ./script/build.py -c D
```

After building is done, you can find `Electron.app` under `out/D`.

## 32bit Support

Electron can only be built for a 64bit target on macOS and there is no plan to
support 32bit macOS in the future.

## Cleaning

To clean the build files:

```bash
$ npm run clean
```

## Tests

See [Build System Overview: Tests](build-system-overview.md#tests)
