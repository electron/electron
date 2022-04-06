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

## Building Electron

See [Build Instructions: GN](build-instructions-gn.md).
