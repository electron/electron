# Source Code Directory Structure

The source code of Electron is separated into a few parts, mostly
following Chromium on the separation conventions.

You may need to become familiar with [Chromium's multi-process
architecture](http://dev.chromium.org/developers/design-documents/multi-process-architecture)
to understand the source code better.

## Structure of Source Code

```
Electron
├── atom/ - C++ source code. (should be called Electron now)
|   ├── app/ - System entry code. Defines the entry point from the main function to Chromium; There are two entry points into Electron: `node` mode and `app` mode. `ELECTRON_RUN_AS_NODE`
|   ├── browser/ - The frontend including the main window, UI, and all of the
|   |   main process things. This talks to the renderer to manage web pages. Contains code that runs in the _main_ process. Chromium calls the "main" process the "browser" process.
|   |   ├── ui/ - Implementation of UI stuff for different platforms.
|   |   |   ├── cocoa/ - Cocoa specific source code.
|   |   |   ├── views/ - Contains our code for using UI library developed by Chromium (e.g. menus)
|   |   |   ├── win/ - Windows GUI specific source code.
|   |   |   └── x/ - X11 specific source code.
|   |   ├── api/ - The implementation of the main process APIs.
|   |   ├── lib/ - (Should not be here! Should go in parent folder.)
|   |   ├── mac/ - Mac-specific Objective-C source code. Can be UI-related or not.
|   |   ├── net/ - Network-related code. Implements custom protocols.
|   |   ├── osr/ - Code for off-screen rendering
|   |   └── resources/ - Icons, platform-dependent files, etc.
|   └── node/ - Code that is built as part of node that is not worth adding a patch to node. common.gypi changes the node build config to include this directory
|   ├── renderer/ - Code that runs in renderer process.
|   |   └── api/ - The implementation of renderer process APIs.
|   └── common/ - Code that used by both the main and renderer processes,
|       including some utility functions and code to integrate node's message
|       loop into Chromium's message loop.
|       └── api/ - The implementation of common APIs, and foundations of
|           Electron's built-in modules.
|       └── asar/ - Implementation of ASAR code. Provides APIs in js to use ASAR.
|       └── crash_reporter/ - Uniform interface for crashpad and breakpad.
|       └── native_mate_converters/ - Convert C++ types to Javascript types. Defines converters for each type. Can implement ToV8 and/or FromV8
|   |   └── resources/ - Icons, platform-dependent files, etc.
|   └── utility/ - Utility process is used for running dangerous and async tasks. (Unsure of purpose; created by a contributor; maybe created for printing?)
├── chromium_src/ - Source code that is copied from Chromium. (Modified to work with electron; built separately; vendored files that are extracted from chrome and modified; surface area of chrome that is not in libchromiumcontent; e.g. certain parts of DevTools, spell check, prerendering are not included in libchromiumcontent; things can break in here when chrome is upgraded)
├── default_app/ - The default page to show when Electron is started without
|   providing an app.
├── docs/ - Documentation in English.
├── docs-translations/ - Community-translated documentation in different languages.
├── lib/ - JavaScript source code. (structured like c++ files in `atom` directory) By default, these directories are not available to users. The files in the `exports` are. Includes custom APIs written to support DevTools extensions in Electron contexts, since that isn't included in the content module.
|   ├── browser/ - Javascript main process initialization code.
|   |   └── api/ - Javascript API implementation.
|   ├── common/ - JavaScript used by both the main and renderer processes
|   |   └── api/ - Javascript API implementation.
|   └── renderer/ - Javascript renderer process initialization code.
|       └── api/ - Javascript API implementation.
|       └── extensions/ - Implementation of the Chrome APIs which are used by Dev Tool extensions
|       └── web-view/ - JavaScript code of the web-view tag
|   ├── sandboxed_renderer/ - By default, code in Electron is not run in the sandbox. Contains files to initialize a fake node environment. Some features like preload script are implmented by Javascript. For normal renderer, we use APIs like fs. For sandbox we use alternative methods like IPC, and insert some APIs directly into DOM.
├── spec/ - Automated tests.
├── electron.gyp - Building rules of Electron. (configures how we build the c++ files and copy and package js files; which files to copy; which to generate; builds the asar file out of the javascript)
└── common.gypi - Compiler specific settings and building rules for other
├── script/ - Contains scripts run by developers
├── tools/ - Not run manually by developers; Contains script run automatically by gyp for building tasks
├── vendor/ - Third-party libraries; Each has its own gyp file; Most are dependencies of Chromium
|   components like `node` and `breakpad`.
|   ├──boto - Python tool for interacting with AWS
|   ├──breakpad - Crash reporting for Windows and Linux
|   ├──brightray - Thin wrapper around Chromium Content API
|   ├──crashpad - Crash reporting for Mac
|   ├──depot_tools - Set of tools for developing Chromium. Easy way to get tools like `ninja`
|   ├──native_mate - C++ library that makes it easier to use V8 APIs.
|   ├──node - Our fork of Node
|   ├──requests - Python library for making HTTP requests.
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
[/vendor][vendor] directory. Occasionally you might see a message like this
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
	su = submodule update --init --recursive
```

[vendor]: https://github.com/electron/electron/tree/master/vendor
