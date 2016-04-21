# Environment variables

Control application configuration and behavior without changing code.

Some behaviors of Electron are controlled by environment variables, because they
are initialized earlier than command line and the app's code.

Examples on POSIX shells:

```bash
$ export ELECTRON_ENABLE_LOGGING=true
$ electron
```

on Windows console:

```powershell
> set ELECTRON_ENABLE_LOGGING=true
> electron
```

## `ELECTRON_RUN_AS_NODE`

Starts the process as a normal Node.js process.

## `ELECTRON_ENABLE_LOGGING`

Prints Chrome's internal logging to console.

## `ELECTRON_LOG_ASAR_READS`

When Electron reads from an ASAR file, log the read offset and file path to
the system `tmpdir`. The resulting file can be provided to the ASAR module
to optimize file ordering.

## `ELECTRON_ENABLE_STACK_DUMPING`

When Electron crashed, prints the stack trace to console.

This environment variable will not work if `crashReporter` is started.

## `ELECTRON_DEFAULT_ERROR_MODE` _Windows_

Shows Windows's crash dialog when Electron crashed.

This environment variable will not work if `crashReporter` is started.

## `ELECTRON_NO_ATTACH_CONSOLE` _Windows_

Don't attach to current console session.

## `ELECTRON_FORCE_WINDOW_MENU_BAR` _Linux_

Don't use global menu bar on Linux.

## `ELECTRON_HIDE_INTERNAL_MODULES`

Turns off compatibility mode for old built-in modules like `require('ipc')`.
