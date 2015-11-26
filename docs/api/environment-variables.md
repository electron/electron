# Environment variables

Some behaviors of Electron are controlled by environment variables, because they
are initialized earlier than command line and the app's code.

## `ELECTRON_RUN_AS_NODE`

Starts the process as a normal Node.js process.

## `ELECTRON_ENABLE_LOGGING`

Prints Chrome's internal logging to console.

## `ELECTRON_ENABLE_STACK_DUMPING`

When Electron crashed, prints the stack trace to console.

This environment variable will not work if `crashReporter` is started.

## `ELECTRON_DEFAULT_ERROR_MODE` _Windows_

Shows Windows's crash dialog when Electron crashed.

This environment variable will not work if `crashReporter` is started.

## `ELECTRON_FORCE_WINDOW_MENU_BAR` _Linux_

Don't use global menu bar on Linux.

## `ELECTRON_HIDE_INTERNAL_MODULES`

Turns off compatibility mode for old built-in modules like `require('ipc')`.
