# Source Code Directory Structure

The source code of Electron is separated into a few parts, mostly
following Chromium on the separation conventions.

You may need to become familiar with
[Chromium's multi-process architecture](https://www.chromium.org/developers/design-documents/multi-process-architecture/)
to understand the source code better.

## Project structure

Electron is a complex project containing multiple upstream dependencies, which are tracked in source
control via the [`DEPS`](https://github.com/electron/electron/blob/main/DEPS) file. When
[initializing a local Electron checkout](./build-instructions-gn.md), Electron's source code is just one
of many nested folders within the project root.

The project contains a single `src` folder that corresponds to a specific git checkout of
[Chromium's `src` folder](https://source.chromium.org/chromium/chromium/src). In addition, Electron's
repository code is contained in `src/electron` (with its own nested git repository), and other
Electron-specific third-party dependencies (e.g. [nan](https://github.com/nodejs/nan) or
[node](https://github.com/nodejs/node)) are located in `src/third_party` (along with all other
Chromium third-party dependencies, such as WebRTC or ANGLE).

For all code outside of `src/electron`, Electron-specific code changes are maintained via git patches.
See the [Patches](./patches.md) development guide for more information.

```plaintext
Project Root
└── src
    ├── electron
    ├── third_party
    │   ├── nan
    │   ├── electron_node
    │   └── ...other third party deps
    └── ...other folders
```

## Structure of Electron source code

```plaintext
Electron
├── build/ - Build configuration files needed to build with GN.
├── buildflags/ - Determines the set of features that can be conditionally built.
├── chromium_src/ - Source code copied from Chromium that isn't part of the content layer.
├── default_app/ - A default app run when Electron is started without
|                  providing a consumer app.
├── docs/ - Electron's documentation.
|   ├── api/ - Documentation for Electron's externally-facing modules and APIs.
|   ├── development/ - Documentation to aid in developing for and with Electron.
|   ├── fiddles/ - A set of code snippets one can run in Electron Fiddle.
|   ├── images/ - Images used in documentation.
|   └── tutorial/ - Tutorial documents for various aspects of Electron.
├── lib/ - JavaScript/TypeScript source code.
|   ├── browser/ - Main process initialization code.
|   |   ├── api/ - API implementation for main process modules.
|   ├── common/ - Relating to logic needed by both main and renderer processes.
|   |   └── api/ - API implementation for modules that can be used in
|   |              both the main and renderer processes
|   ├── isolated_renderer/ - Handles creation of isolated renderer processes when
|   |                        contextIsolation is enabled.
|   ├── node/ - Initialization code for Node.js in the main process.
│   ├── preload_realm/ - Initialization code for sandboxed renderer preload scripts.
│   │   └── api/ - API implementation for preload scripts.
|   ├── renderer/ - Renderer process initialization code.
|   |   ├── api/ - API implementation for renderer process modules.
|   |   └── web-view/ - Logic that handles the use of webviews in the
|   |                   renderer process.
|   ├── sandboxed_renderer/ - Logic that handles creation of sandboxed renderer
|   |   |                     processes.
|   |   └── api/ - API implementation for sandboxed renderer processes.
│   ├── utility/ - Utility process initialization code.
│   │   └── api/ - API implementation for utility process modules.
|   └── worker/ - Logic that handles proper functionality of Node.js
|                 environments in Web Workers.
├── patches/ - Patches applied on top of Electron's core dependencies
|   |          in order to handle differences between our use cases and
|   |          default functionality.
|   ├── boringssl/ - Patches applied to Google's fork of OpenSSL, BoringSSL.
|   ├── chromium/ - Patches applied to Chromium.
|   ├── node/ - Patches applied on top of Node.js.
|   └── v8/ - Patches applied on top of Google's V8 engine.
├── shell/ - C++ source code.
|   ├── app/ - System entry code.
|   ├── browser/ - The frontend including the main window, UI, and all of the
|   |   |          main process things. This talks to the renderer to manage web
|   |   |          pages.
|   |   ├── ui/ - Implementation of UI stuff for different platforms.
|   |   |   ├── cocoa/ - Cocoa specific source code.
|   |   |   ├── win/ - Windows GUI specific source code.
|   |   |   └── x/ - X11 specific source code.
|   |   ├── api/ - The implementation of the main process APIs.
|   |   ├── net/ - Network related code.
|   |   ├── mac/ - Mac specific Objective-C source code.
|   |   └── resources/ - Icons, platform-dependent files, etc.
|   ├── renderer/ - Code that runs in renderer process.
|   |   └── api/ - The implementation of renderer process APIs.
|   ├── common/ - Code that used by both the main and renderer processes,
|   |   |         including some helper functions and code to integrate node's
|   |   |         message loop into Chromium's message loop.
|   |   └── api/ - The implementation of common APIs, and foundations of
|   |              Electron's built-in modules.
│   ├── services/node/ - Provides a Node.js runtime to utility processes. 
│   └── utility - Code that runs in the utility process.
├── spec/ - Components of Electron's test suite run in the main process.
├── typings/ - Internal TypeScript types that aren't exported in electron.d.ts.
└── BUILD.gn - Building rules of Electron.
```

## Structure of other Electron directories

* **.github** - GitHub-specific config files including issues templates, CI with GitHub Actions and CODEOWNERS.
* **dist** - Temporary directory created by `script/create-dist.py` script
  when creating a distribution.
* **node_modules** - Third party node modules used for building.
* **npm** - Logic for installation of Electron via npm.
* **out** - Temporary output directory for `siso`.
* **script** - Scripts used for development purpose like building, packaging,
  testing, etc.

```plaintext
script/ - The set of all scripts Electron runs for a variety of purposes.
├── codesign/ - Fakes codesigning for Electron apps; used for testing.
├── lib/ - Miscellaneous python utility scripts.
└── release/ - Scripts run during Electron's release process.
    ├── notes/ - Generates release notes for new Electron versions.
    └── uploaders/ - Uploads various release-related files during release.
```

* **typings** - TypeScript typings for Electron's internal code.
