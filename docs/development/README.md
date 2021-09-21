# Developing Electron

These guides are intended for people working on the Electron project itself.
For guides on Electron app development, see
[/docs/README.md](../README.md#guides-and-tutorials).

## Table of Contents

* [Code of Conduct](https://github.com/electron/electron/blob/master/CODE_OF_CONDUCT.md)
* [Contributing to Electron](https://github.com/electron/electron/blob/master/CONTRIBUTING.md)
* [Issues](issues.md)
* [Pull Requests](pull-requests.md)
* [Documentation Styleguide](coding-style.md#documentation)
* [Source Code Directory Structure](source-code-directory-structure.md)
* [Coding Style](coding-style.md)
* [Using clang-tidy on C++ Code](clang-tidy.md)
* [Build System Overview](build-system-overview.md)
* [Build Instructions (macOS)](build-instructions-macos.md)
* [Build Instructions (Windows)](build-instructions-windows.md)
* [Build Instructions (Linux)](build-instructions-linux.md)
* [Chromium Development](chromium-development.md)
* [V8 Development](v8-development.md)
* [Testing](testing.md)
* [Debugging](debugging.md)
* [Patches](patches.md)

## Getting Started

In order to contribute to Electron, the first thing you'll want to do is get the code.

[Electron's Build Tools](https://github.com/electron/build-tools) automate much of the setup for compiling Electron from source with different configurations and build targets.

If you would prefer to build Electron manually, see [Build Instructions](build-instructions-gn.md).

Once you've checked out and built the code, you may want to take a look around the source tree to get a better idea
of what each directory is responsible for. [Source Code Directory Structure](source-code-directory-structure.md) gives a good overview of the purpose of each directory.

## Opening Issues on Electron

For any issue, there are generally three ways an individual can
contribute:

1. By opening the issue for discussion
    * If you believe that you have found a new bug in Electron, you should report it by creating a new issue in
    the [`electron/electron` issue tracker](https://github.com/electron/electron/issues).
2. By helping to triage the issue
    * You can do this either by providing assistive details (a reproducible test case that demonstrates a bug) or by providing suggestions to address the issue.
3. By helping to resolve the issue
    * This can be done by demonstrating that the issue is not a bug or is fixed;
      but more often, by opening a pull request that changes the source in `electron/electron`
      in a concrete and reviewable manner.

See [issues](issues.md) for more information.

## Making a Pull Request to Electron

See [Pull Requests](pull-requests.md) for more information.

## Debugging

For information related to debugging Electron itself (vs an app _built with Electron_), see [Debugging](debugging.md).
