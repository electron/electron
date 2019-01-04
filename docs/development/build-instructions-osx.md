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
$ python ./script/tls.py
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

Official Electron builds are built with [Xcode 8.3.3](http://adcdownload.apple.com/Developer_Tools/Xcode_8.3.3/Xcode_8.3.3.xip), and the MacOS 10.12 SDK.  Building with a newer SDK works too, but the releases currently use the 10.12 SDK.

## Getting the Code

```sh
$ git clone https://github.com/electron/electron
```

## Bootstrapping

The bootstrap script will download all necessary build dependencies and create
the build project files. Notice that we're using [ninja](https://ninja-build.org/)
to build Electron so there is no Xcode project generated.

```sh
$ cd electron
$ ./script/bootstrap.py -v
```

If you are using editor supports [JSON compilation database](http://clang.llvm.org/docs/JSONCompilationDatabase.html) based
language server, you can generate it:

```sh
$ ./script/build.py --compdb
```

## Building

Build both `Release` and `Debug` targets:

```sh
$ ./script/build.py
```

You can also only build the `Debug` target:

```sh
$ ./script/build.py -c D
```

After building is done, you can find `Electron.app` under `out/D`.

## 32bit Support

Electron can only be built for a 64bit target on macOS and there is no plan to
support 32bit macOS in the future.

## Cleaning

To clean the build files:

```sh
$ npm run clean
```

To clean only `out` and `dist` directories:

```sh
$ npm run clean-build
```

**Note:** Both clean commands require running `bootstrap` again before building.

## Tests

See [Build System Overview: Tests](build-system-overview.md#tests)
