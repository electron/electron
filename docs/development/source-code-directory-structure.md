# Source Code Directory Structure

The source code of Electron is separated into a few parts, mostly
following Chromium on the separation conventions.

You may need to become familiar with [Chromium's multi-process
architecture](http://dev.chromium.org/developers/design-documents/multi-process-architecture)
to understand the source code better.

## Structure of Source Code

```
Electron
├── atom - C++ source code.
|   ├── app - System entry code.
|   ├── browser - The frontend including the main window, UI, and all of the
|   |   main process things. This talks to the renderer to manage web pages.
|   |   ├── ui - Implementation of UI stuff for different platforms.
|   |   |   ├── cocoa - Cocoa specific source code.
|   |   |   ├── gtk - GTK+ specific source code.
|   |   |   └── win - Windows GUI specific source code.
|   |   ├── api - The implementation of the main process APIs.
|   |   ├── net - Network related code.
|   |   ├── mac - Mac specific Objective-C source code.
|   |   └── resources - Icons, platform-dependent files, etc.
|   ├── renderer - Code that runs in renderer process.
|   |   └── api - The implementation of renderer process APIs.
|   └── common - Code that used by both the main and renderer processes,
|       including some utility functions and code to integrate node's message
|       loop into Chromium's message loop.
|       └── api - The implementation of common APIs, and foundations of
|           Electron's built-in modules.
├── chromium_src - Source code that copied from Chromium.
├── default_app - The default page to show when Electron is started without
|   providing an app.
├── docs - Documentations.
├── lib  - JavaScript source code.
|   ├── browser - Javascript main process initialization code.
|   |   └── api - Javascript API implementation.
|   ├── common - JavaScript used by both the main and renderer processes
|   |   └── api - Javascript API implementation.
|   └── renderer - Javascript renderer process initialization code.
|       └── api - Javascript API implementation.
├── spec - Automatic tests.
├── atom.gyp - Building rules of Electron.
└── common.gypi - Compiler specific settings and building rules for other
    components like `node` and `breakpad`.
```

## Structure of Other Directories

* **script** - Scripts used for development purpose like building, packaging,
  testing, etc.
* **tools** - Helper scripts used by gyp files, unlike `script`, scripts put
  here should never be invoked by users directly.
* **vendor** - Source code of third party dependencies, we didn't use
  `third_party` as name because it would confuse it with the same directory in
  Chromium's source code tree.
* **node_modules** - Third party node modules used for building.
* **out** - Temporary output directory of `ninja`.
* **dist** - Temporary directory created by `script/create-dist.py` script
  when creating a distribution.
* **external_binaries** - Downloaded binaries of third-party frameworks which
  do not support building with `gyp`.

## Keeping Git Submodules Up to Date

The Electron repository has a few vendored dependencies, found in the
[/vendor](/vendor) directory. Occasionally you might see a message like this
when running `git status`:

```sh
$ git status

	modified:   vendor/brightray (new commits)
	modified:   vendor/node (new commits)
```

To update these vendored dependencies, run the following command:

```sh
git submodule update --init --recursive
```

If you find yourself running this command often, you can create an alias for it
in your `~/.gitconfig` file:

```
[alias]
	su = git submodule update --init --recursive
```
