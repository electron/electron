# Crash Reporting

## Overview

When a native crash happens in your app (a segfault in Chromium, a fatal V8 error, an
out-of-memory renderer, or a bug in a native Node.js module), there is no JavaScript
exception to catch. Electron's [`crashReporter`](../api/crash-reporter.md) module
captures these crashes as minidumps that you can collect, upload and turn back into
readable stack traces.

This guide covers how crash reporting works in Electron, how to attach useful context to
reports, how to receive them, and how to symbolicate them.

## What happens when your app crashes

Electron uses [Crashpad](https://chromium.googlesource.com/crashpad/crashpad/+/refs/heads/main/README.md),
the same crash reporting system as Chromium. When you call `crashReporter.start()` in the
main process:

1. Electron starts a separate crash handler process.
2. Child processes created after `start()` is called (renderers, the GPU process, utility
   processes and Node.js child processes) are monitored automatically.
3. When a monitored process crashes, the handler writes a minidump: a snapshot of the
   crashed process's threads, stacks and loaded modules. This works even when the crashed
   process is too broken to run any code of its own.
4. If uploads are enabled, the handler sends the minidump to your `submitURL`, along with
   the annotations (key/value metadata) that were set when the process crashed.

Crash reports are stored under the directory returned by `app.getPath('crashDumps')`. To
store them somewhere else, call `app.setPath('crashDumps', path)` before calling
`crashReporter.start()`. The layout of files inside this directory is an implementation
detail and may change between versions of Electron, so don't depend on it.

## Setting up the crash reporter

Call `crashReporter.start()` in the main process as early as possible, ideally before
`app.whenReady()`. Renderer and child processes that start before the crash reporter
won't be monitored.

```js title='main.js'
const { app, crashReporter } = require('electron')

crashReporter.start({
  submitURL: 'https://crashes.example.com/submit',
  uploadToServer: true,
  globalExtra: {
    releaseChannel: 'beta'
  }
})

app.whenReady().then(() => {
  // create windows...
})
```

The options that matter most are:

* `submitURL` - Where crash reports are sent, as a `multipart/form-data` `POST`.
  Required unless `uploadToServer` is `false`.
* `uploadToServer` - Whether to upload reports. If you ask users for consent to send
  crash reports, start with `uploadToServer: false` and call
  `crashReporter.setUploadToServer(true)` when they opt in. Reports are still written
  to disk when uploads are disabled.
* `productName` - Sent as `_productName`. Defaults to `app.name`.
* `globalExtra` - Annotations that are sent with crashes from every process. See
  [Attaching context to crash reports](#attaching-context-to-crash-reports).
* `extra` - Annotations for crashes in the main process only.
* `rateLimit` - Limits uploads to one per hour. Reports over the limit are not uploaded
  but stay on disk.
* `compress` - Uploads are gzip-compressed (`Content-Encoding: gzip`) by default. Most
  crash servers expect this. Setting `compress: false` is deprecated and logs a
  warning.
* `ignoreSystemCrashHandler` - Stops main process crashes from also being passed to the
  operating system's crash handler. This has no effect on Windows.

Calling `start()` a second time does nothing, and the crash reporter can't be stopped
once it has started.

To test your setup, call `process.crash()` from the process you want to crash.

### Mac App Store builds

The crash reporter is disabled in [Mac App Store](./mac-app-store-submission-guide.md)
builds of Electron. All `crashReporter` methods can still be called, but they do
nothing: no minidumps are written and `getUploadedReports()` always returns an empty
array. Crashes in MAS builds are reported only through Apple's own crash reporting.

### Sandboxed renderers and preload scripts

`crashReporter` is one of the modules available to [sandboxed](./sandbox.md) preload
scripts. In renderer processes it only provides `addExtraParameter()`,
`removeExtraParameter()` and `getParameters()`. The crash reporter itself is started in
the main process.

With [context isolation](./context-isolation.md) enabled, web content can't reach
`crashReporter` directly. Call it from your preload script, or expose a narrow function
with the [`contextBridge`](../api/context-bridge.md), as shown in the next section.

## Attaching context to crash reports

A stack trace tells you where a crash happened. Annotations tell you what the app was
doing at the time: which window was open, which feature was in use, which account type
the user has. Annotation values are captured at the moment of the crash, so what you get
is whatever the value was when the process died.

### Where annotations apply

There are two kinds of annotation, and it's important to put each value in the right
place:

* **`globalExtra`** is set once, in `crashReporter.start()`, and is sent with crashes
  from every process. It can't be changed later. Use it for values that don't change
  while the app is running, such as a release channel or build ID.
* **`extra` and `addExtraParameter()`** are per-process. A value set in the main
  process is only sent with main process crashes. A value set in one renderer is only
  sent when that renderer crashes. Each process has to set its own values.

If the same key is set in both `globalExtra` and a process's own parameters, the
`globalExtra` value is used.

Where you can set per-process values:

| Process | How to set values |
| --- | --- |
| Main | `extra` in `crashReporter.start()`, or `crashReporter.addExtraParameter()` |
| Renderer | `crashReporter.addExtraParameter()` in the preload script |
| Node.js child process (`child_process.fork()`) | `process.crashReporter.addExtraParameter()` |
| Utility process (`utilityProcess.fork()`) | No API. Only `globalExtra` values are sent. |

`crashReporter.getParameters()` returns the current process's own parameters. It does not
include `globalExtra`.

### Limits

* Key names must be at most 39 bytes long. Longer keys are ignored, and Electron emits a
  process warning when you try to set one.
* Values are strings of at most 20320 bytes. Longer values are truncated.
* Limits are in bytes, not characters, so non-ASCII text uses up the limit faster.

### Keep annotations current

Because values are read at crash time, update them as your app's state changes. For
example, you can record how many windows are open and which feature the user is using
in the main process:

```js title='main.js'
const { app, crashReporter } = require('electron')

let windowCount = 0

app.on('browser-window-created', (event, win) => {
  windowCount++
  crashReporter.addExtraParameter('windowCount', String(windowCount))
  win.on('closed', () => {
    windowCount--
    crashReporter.addExtraParameter('windowCount', String(windowCount))
  })
})

async function exportProject() {
  crashReporter.addExtraParameter('feature', 'export')
  try {
    // ...run the export
  } finally {
    crashReporter.removeExtraParameter('feature')
  }
}
```

For renderer crashes, set values in the renderer. This preload script records the current
route of a single-page app and lets the page mark which feature is active. It only
accepts known keys, so the page can't fill reports with arbitrary data:

```js title='preload.js'
const { contextBridge, crashReporter } = require('electron')

const allowedKeys = new Set(['feature', 'route'])

contextBridge.exposeInMainWorld('crashContext', {
  set: (key, value) => {
    if (allowedKeys.has(key)) {
      crashReporter.addExtraParameter(key, String(value))
    }
  }
})

window.addEventListener('DOMContentLoaded', () => {
  crashReporter.addExtraParameter('route', location.pathname)
})
```

```js title='renderer.js' @ts-nocheck
window.crashContext.set('route', '/settings')
window.crashContext.set('feature', 'image-editor')
```

## Reacting to crashes at runtime

Minidumps are for diagnosing a crash later. To react when a process dies, for example to
log it or recover, listen for these events in the main process:

* [`render-process-gone`](../api/app.md#event-render-process-gone) on `app`, or on a
  single [`webContents`](../api/web-contents.md#event-render-process-gone), for renderer
  processes.
* [`child-process-gone`](../api/app.md#event-child-process-gone) on `app` for all other
  child processes, such as the GPU and utility processes.

Both events provide a `reason` (such as `crashed`, `oom`, `killed` or `clean-exit`) and
an `exitCode`.

```js title='main.js'
const { app, BrowserWindow } = require('electron')

app.on('child-process-gone', (event, details) => {
  console.error(`${details.type} process gone: ${details.reason} (exit code ${details.exitCode})`)
})

app.whenReady().then(() => {
  const win = new BrowserWindow()
  let recentCrashes = 0

  win.webContents.on('render-process-gone', (event, details) => {
    console.error(`Renderer gone: ${details.reason} (exit code ${details.exitCode})`)
    if (details.reason === 'clean-exit') return

    // Reload the page in a new renderer process, but don't retry forever.
    recentCrashes++
    if (recentCrashes <= 3) {
      win.reload()
      setTimeout(() => {
        recentCrashes--
      }, 60 * 1000)
    }
  })

  win.loadFile('index.html')
})
```

## Receiving crash reports

### On your own server

Crash reports are sent to `submitURL` as a `multipart/form-data` `POST`, gzip-compressed
unless you set `compress: false`. The form contains:

* `upload_file_minidump` - The minidump file.
* `process_type` - The type of process that crashed, such as `renderer`, or `browser`
  for the main process.
* `prod` - Always `Electron`.
* `ver` - The Electron version.
* `_productName` - The `productName` option, which defaults to `app.name`.
* `_version` - Your app's version, from `app.getVersion()`.
* `guid` - An ID for this installation.
* `platform` - `win32`, `darwin` or `linux`.
* Your `globalExtra` values and the crashed process's own parameters.

Crashpad and Chromium may add other fields. These aren't part of Electron's API and can
change without notice, so don't rely on them. See
[Crash Report Payload](../api/crash-reporter.md#crash-report-payload) for the full list
of documented fields.

Respond with a `200` status. The body of the response is stored as the report's ID, which
`crashReporter.getUploadedReports()` returns, so you can use it to link a user's report
to your server's record.

Crashpad uses the Breakpad upload protocol, so any server that accepts Breakpad or
Crashpad minidumps can receive Electron's reports.

### Collecting reports locally

If you set `uploadToServer: false`, crash reports are still written under
`app.getPath('crashDumps')`, but they are not sent anywhere. This is useful while
developing, or if you want to ask the user before sending anything: once they agree, call
`crashReporter.setUploadToServer(true)`. Only crashes that happen after that are
uploaded; reports written while uploads were disabled are not sent later.

Electron doesn't provide an API for reading minidump files from disk, and the layout of
the crash dumps directory can change between versions. If you need the minidumps
themselves, receive them with your own `submitURL` (which can be a server running on
the same machine) instead of reading files from the directory.

## Symbolicating crash reports

A minidump contains raw memory addresses, not function names. Electron's release builds
are stripped of debug information, so to turn those addresses into a readable stack
trace you need the symbol files for the exact Electron version, platform and
architecture that crashed. This is called symbolication.

### Getting Electron's symbols

Each [Electron release](https://github.com/electron/electron/releases) on GitHub ships
symbol archives for every platform it supports:

* [Breakpad](https://chromium.googlesource.com/breakpad/breakpad/) symbols for all
  platforms. These are what most minidump tools use.
* dSYMs for macOS, for use with Apple's tools such as `atos` and `lldb`.
* PDBs for Windows, for use with WinDbg and Visual Studio.
* Debug info for Linux, for use with `gdb`.

Electron also runs a symbol server at `https://symbols.electronjs.org`. Tools that
support symbol servers can download the symbols they need from it, so you don't have to
download the archives for each version. For debuggers on Windows, see
[Setting Up Symbol Server in Debugger](../development/debugging-with-symbol-server.md).

### Example: symbolicating a minidump

With [`minidump-stackwalk`](https://github.com/rust-minidump/rust-minidump/tree/main/minidump-stackwalk),
you can point at the symbol server directly:

```sh
cargo install minidump-stackwalk
minidump-stackwalk --symbols-url=https://symbols.electronjs.org /path/to/crash.dmp
```

Or download the Breakpad symbols archive for the matching release, extract it, and pass
the directory of symbols to the tool. This works with Breakpad's own
`minidump_stackwalk` as well:

```sh
minidump_stackwalk /path/to/crash.dmp /path/to/electron-symbols
```

From Node.js, you can use the [`minidump`](https://github.com/electron/node-minidump)
package, which bundles Breakpad's tools:

```js @ts-nocheck
const minidump = require('minidump')

minidump.addSymbolPath('/path/to/electron-symbols')
minidump.walkStack('/path/to/crash.dmp', (error, report) => {
  if (error) throw error
  console.log(report.toString())
})
```

To find out which Electron version a report came from, read the `ver` field that was
uploaded with it.

### Symbols for your own code

Electron's symbols only cover Electron's own binaries. If your app includes native Node.js
modules or other native libraries, frames in those will stay unsymbolicated unless you
also keep symbols for them. Generate Breakpad symbols for each binary you ship with
Breakpad's `dump_syms` tool (or `minidump.dumpSymbol()` from the `minidump` package),
and store them where your symbolication tool can find them. Keep them for every
version you release, since symbols only match the exact build they came from.

### macOS system crash reports

When an app crashes on macOS, the system may also write a crash report (an `.ips` file,
shown in the Console app) to `~/Library/Logs/DiagnosticReports`. For Electron's release
builds these reports are misleading: because the binaries are stripped, macOS labels each
frame with the nearest exported symbol plus a large offset, such as
`v8::internal::SetupIsolateDelegate::SetupHeap(v8::internal::Heap*) + 2919558`. These
function names are almost always wrong.

To get a correct stack from one of these reports, use
[`@electron/symbolicate-mac`](https://github.com/electron/symbolicate-mac). It reads text
crash reports, spindumps and `sample` output, works out the Electron version from the
report's "Binary Images" section, and downloads the right symbols:

```sh
npx @electron/symbolicate-mac /path/to/crash.txt
```

For an `.ips` file, open it in the Console app and save the full text report, including
the "Binary Images" section, first.

You can also symbolicate individual addresses yourself with `atos` and the dSYM from the
matching release. Take the load address of `Electron Framework` from the "Binary Images"
section and the frame's address from the stack:

```sh
atos -arch arm64 -o "Electron Framework.dSYM/Contents/Resources/DWARF/Electron Framework" \
  -l 0x109b49000 0x10ae32a06
```

## Using a hosted crash reporting service

Instead of running your own server, you can send crash reports to a hosted service. These
services receive Electron's minidumps, symbolicate them (usually with Electron's public
symbols) and group similar crashes. Many also offer SDKs that report JavaScript errors
alongside native crashes. Services with Electron support include:

* [Sentry](https://docs.sentry.io/platforms/javascript/guides/electron/)
* [BugSplat](https://www.bugsplat.com/docs/sdk/electron/)
* [Backtrace](https://github.com/backtrace-labs/backtrace-javascript/tree/main/packages/electron) (part of Sauce Labs)
* [Bugsnag](https://docs.bugsnag.com/platforms/electron/)

This list is provided for convenience. The Electron project doesn't endorse or support
any of these services; check each one's documentation for how it works with Electron.
