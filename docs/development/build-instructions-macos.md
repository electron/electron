# Build Instructions (macOS)

Follow the guidelines below for building Electron on macOS.

## Prerequisites

* macOS >= 10.11.6
* [Xcode](https://developer.apple.com/technologies/tools/) >= 8.2.1
* [node.js](https://nodejs.org) (external)
* Python 2.7 with support for TLS 1.2

## Python

Please also ensure that your system and Python version support at least TLS 1.2.
This depends on both your version of macOS and Python. For a quick test, run:

```sh
$ npm run check-tls
```

If the script returns that your configuration is using an outdated security
protocol, you can either update macOS to High Sierra or install a new version
of Python 2.7.x. To upgrade Python, use [Homebrew](https://brew.sh/):

```sh
$ brew install python@2 && brew link python@2 --force
```

If you are using Python as provided by Homebrew, you also need to install
the following Python modules:

* [pyobjc](https://pythonhosted.org/pyobjc/install.html)

## macOS SDK

If you're developing Electron and don't plan to redistribute your
custom Electron build, you may skip this section.

For certain features (e.g. pinch-zoom) to work properly, you must target the
macOS 10.10 SDK.

Official Electron builds are built with [Xcode 8.2.1](http://adcdownload.apple.com/Developer_Tools/Xcode_8.2.1/Xcode_8.2.1.xip), which does not contain
the 10.10 SDK by default. To obtain it, first download and mount the
[Xcode 6.4](http://developer.apple.com/devcenter/download.action?path=/Developer_Tools/Xcode_6.4/Xcode_6.4.dmg)
DMG.

Then, assuming that the Xcode 6.4 DMG has been mounted at `/Volumes/Xcode` and
that your Xcode 8.2.1 install is at `/Applications/Xcode.app`, run:

```sh
cp -r /Volumes/Xcode/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/
```

You will also need to enable Xcode to build against the 10.10 SDK:

- Open `/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Info.plist`
- Set the `MinimumSDKVersion` to `10.10`
- Save the file

## Building Electron

See [Build Instructions: GN](build-instructions-gn.md).
