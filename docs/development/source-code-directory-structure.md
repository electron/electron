# Source code directory structure

## Overview

The source code of atom-shell is separated into a few parts, and we are mostly
following Chromium on the separation conventions.

You may need to become familiar with [Chromium's multi-process
architecture](http://dev.chromium.org/developers/design-documents/multi-process-architecture)
to understand the source code better.

## Structure of source code

* **app** - Contains system entry code, this is the most basic level of the
  program.
* **browser** - The frontend including the main window, UI, and all browser
  side things. This talks to the renderer to manage web pages.
  * **ui** - Implementation of UI stuff for different platforms.
  * **atom** - Initializes the javascript environment of browser.
  * **default_app** - The default page to show when atom-shell is started
    without providing an app.
  * **api** - The implementation of browser side APIs.
     * **lib** - Javascript part of the API implementation.
* **renderer** - Code that runs in renderer.
  * **api** - The implementation of renderer side APIs.
     * **lib** - Javascript part of the API implementation.
* **common** - Code that used by both browser and renderer, including some
  utility functions and code to integrate node's message loop into Chromium's message loop.
  * **api** - The implementation of common APIs, and foundations of
    atom-shell's built-in modules.
     * **lib** - Javascript part of the API implementation.
* **spec** - Automatic tests.
* **script** - Scripts for building atom-shell.

## Structure of other directories

* **vendor** - Build dependencies.
* **tools** - Helper scripts to build atom-shell.
* **node_modules** - Third party node modules used for building or running
  specs.
* **out** - Output directory for `ninja`.
* **dist** - Temporary directory created by `script/create-dist.py` script
  when creating an distribution.
* **node** - Downloaded node binary, it's built from
  https://github.com/atom/node/tree/chromium-v8.
* **frameworks** - Downloaded third-party binaries of frameworks (only on
  Mac).
