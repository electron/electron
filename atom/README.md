# `atom`

C++ source code.

Looks up Chromium headers in [/vendor/brightray/vendor/download/libchromiumcontent/src/](../vendor/brightray/vendor/download/libchromiumcontent/src/)
and
[/chromium_src](../chromium_src).

_This directory should be called `electron` now._

## `/atom/app`

System entry code. Defines the entry point from the main function to Chromium; There are two entry points into Electron: `node` mode and `app` mode. `ELECTRON_RUN_AS_NODE`
