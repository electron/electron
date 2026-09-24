# crashReporter

> Submit crash reports to a remote server.

Process: [Main](../glossary.md#main-process), [Renderer](../glossary.md#renderer-process)

> [!IMPORTANT]
> If you want to call this API from a renderer process with context isolation enabled,
> place the API call in your preload script and
> [expose](../tutorial/context-isolation.md#after-context-isolation-enabled) it using the
> [`contextBridge`](context-bridge.md) API.

The following is an example of setting up Electron to automatically submit
crash reports to a remote server:

```js
const { crashReporter } = require('electron')

crashReporter.start({ submitURL: 'https://your-domain.com/url-to-submit' })
```

For a guide to collecting, receiving and symbolicating crash reports, including
how to run your own crash server or use a hosted service, see the
[Crash Reporting](../tutorial/crash-reporting.md) tutorial.

Electron uses [Crashpad](https://chromium.googlesource.com/crashpad/crashpad/+/refs/heads/main/README.md)
to monitor and report crashes. Crashpad uses the same
[upload protocol](https://chromium.googlesource.com/crashpad/crashpad/+/HEAD/doc/overview_design.md#Upload-to-collection-server)
as Breakpad, so servers that accept Breakpad minidumps can receive Electron's
crash reports.

Crash reports are stored in a directory underneath the app's user data
directory, called `Crashpad`. You can get this directory with
`app.getPath('crashDumps')`, and override it by calling
`app.setPath('crashDumps', '/path/to/crashes')` before starting the crash
reporter.

The `crashReporter` module is disabled in Mac App Store builds. Its methods can
be called, but they do nothing: no crash reports are collected or uploaded,
`getUploadedReports()` returns an empty array and `getUploadToServer()` returns
`false`.

On Windows, some crashes never reach an in-process crash handler, most notably
`__fastfail` terminations (`STATUS_STACK_BUFFER_OVERRUN`, raised by
security-check failures, Control Flow Guard violations and the C runtime's
`abort()`). To capture these, Electron ships `electron_wer.dll`, a Windows Error
Reporting (WER) runtime exception helper that Windows (10 20H1 and later) loads
out-of-process after such a crash and that asks the crashpad handler to write a
minidump. When `crashReporter.start()` is called, Electron lists the DLL under
`HKEY_CURRENT_USER\Software\Microsoft\Windows\Windows Error Reporting\RuntimeExceptionHelperModules`,
which Windows requires before it will load a helper, and registers it for every
Electron process. The helper is looked up as `<executable name>_wer.dll` next
to the executable, so if you rename `electron.exe` to `myapp.exe` when
packaging, rename `electron_wer.dll` to `myapp_wer.dll` as well. Installers
that write to `HKEY_LOCAL_MACHINE` may list it there instead. To opt out, do
not ship the DLL.

## Methods

The `crashReporter` module has the following methods:

### `crashReporter.start(options)`

<!--
```YAML history
changes:
  - pr-url: https://github.com/electron/electron/pull/23062
    description: "Added `rateLimit` and `compress` options."
  - pr-url: https://github.com/electron/electron/pull/23265
    description: "Deprecated calling this method in the renderer process."
    breaking-changes-header: deprecated-crashreporter-methods-in-the-renderer-process
  - pr-url: https://github.com/electron/electron/pull/25288
    description: "Default value of `compress` option changed from `false` to `true`."
    breaking-changes-header: default-changed-crashreporterstart-compress-true-
  - pr-url: https://github.com/electron/electron/pull/28105
    description: "The `submitURL` parameter is now optional when `uploadToServer` is `false`."
```
-->

* `options` Object
  * `submitURL` string (optional) - URL that crash reports will be sent to as
    POST. Required unless `uploadToServer` is `false`.
  * `productName` string (optional) - Defaults to `app.name`.
  * `companyName` string (optional) _Deprecated_ - Deprecated alias for
    `{ globalExtra: { _companyName: ... } }`.
  * `uploadToServer` boolean (optional) - Whether crash reports should be sent
    to the server. If false, crash reports will be collected and stored in the
    crashes directory, but not uploaded. Default is `true`.
  * `ignoreSystemCrashHandler` boolean (optional) _macOS_ _Linux_ - If true,
    crashes generated in the main process will not be forwarded to the system
    crash handler. Default is `false`. This option has no effect on Windows.
  * `rateLimit` boolean (optional) - If true, limit the number of crashes
    uploaded to 1/hour. Crash reports over the limit are not uploaded, but are
    still stored on disk. Default is `false`.
  * `compress` boolean (optional) - If true, crash reports will be compressed
    and uploaded with `Content-Encoding: gzip`. Default is `true`. Setting this
    to `false` while `uploadToServer` is `true` is deprecated and logs a
    deprecation warning.
  * `extra` Record\<string, string\> (optional) - Extra string key/value
    annotations that will be sent along with crash reports that are generated
    in the main process. Only string values are supported. Crashes generated in
    child processes will not include these extra parameters. To add extra
    parameters to crash reports generated from child processes, call
    [`addExtraParameter`](#crashreporteraddextraparameterkey-value) from the
    child process.
  * `globalExtra` Record\<string, string\> (optional) - Extra string key/value
    annotations that will be sent along with any crash reports generated in any
    process. These annotations are passed to the crash handler when it starts
    and cannot be changed once the crash reporter has been started. If a key is
    present in both the global extra parameters and the process-specific extra
    parameters, then the global one will take precedence. By default,
    `_productName` and `_version` (the app version) are included, and `prod`
    and `ver` (the Electron version) are always set by Electron. Global extra
    parameters are not returned by
    [`getParameters()`](#crashreportergetparameters).

This method must be called before using any other `crashReporter` APIs. Once
initialized this way, the crashpad handler collects crashes from all
subsequently created processes. The crash reporter cannot be disabled once
started.

This method should be called as early as possible in app startup, preferably
before `app.on('ready')`. If the crash reporter is not initialized at the time
a renderer process is created, then that renderer process will not be monitored
by the crash reporter.

> [!NOTE]
> You can test out the crash reporter by generating a crash using
> `process.crash()`.

> [!NOTE]
> If you need to send additional/updated `extra` parameters after your
> first call `start` you can call `addExtraParameter`.

> [!NOTE]
> Parameters passed in `extra`, `globalExtra` or set with
> `addExtraParameter` have limits on the length of the keys and values. Key
> names must be at most 39 bytes long, and values must be no longer than 20320
> bytes. Keys with names longer than the maximum are ignored, and a warning is
> emitted. Values longer than the maximum length are truncated.

> [!NOTE]
> This method is only available in the main process.

### `crashReporter.getLastCrashReport()`

<!--
```YAML history
changes:
  - pr-url: https://github.com/electron/electron/pull/23265
    description: "Deprecated calling this method in the renderer process."
    breaking-changes-header: deprecated-crashreporter-methods-in-the-renderer-process
```
-->

Returns [`CrashReport | null`](structures/crash-report.md) - The date and ID of the
crash report with the most recent upload time, from the list returned by
[`getUploadedReports()`](#crashreportergetuploadedreports). If there are no crash
reports at all, `null` is returned.

If no report has been uploaded yet but some are stored on disk, a report that has
not been uploaded may be returned. Check that its `id` is not empty before
treating it as uploaded.

> [!NOTE]
> This method is only available in the main process.

### `crashReporter.getUploadedReports()`

<!--
```YAML history
changes:
  - pr-url: https://github.com/electron/electron/pull/23265
    description: "Deprecated calling this method in the renderer process."
    breaking-changes-header: deprecated-crashreporter-methods-in-the-renderer-process
```
-->

Returns [`CrashReport[]`](structures/crash-report.md):

Returns the crash reports that Crashpad knows about. Each report contains the
date it was uploaded and the ID that the crash server returned for it.

Despite the method's name, reports that have not been uploaded (for example
because `uploadToServer` is `false`, the upload failed, or the report was rate
limited) are included too. For those reports, `id` is an empty string and `date`
is the Unix epoch (`new Date(0)`). To list only uploaded reports, filter out
reports with an empty `id`.

> [!NOTE]
> This method is only available in the main process.

### `crashReporter.getUploadToServer()`

<!--
```YAML history
changes:
  - pr-url: https://github.com/electron/electron/pull/23265
    description: "Deprecated calling this method in the renderer process."
    breaking-changes-header: deprecated-crashreporter-methods-in-the-renderer-process
```
-->

Returns `boolean` - Whether reports should be submitted to the server. Set through
the `start` method or `setUploadToServer`.

> [!NOTE]
> This method is only available in the main process.

### `crashReporter.setUploadToServer(uploadToServer)`

<!--
```YAML history
changes:
  - pr-url: https://github.com/electron/electron/pull/23265
    description: "Deprecated calling this method in the renderer process."
    breaking-changes-header: deprecated-crashreporter-methods-in-the-renderer-process
```
-->

* `uploadToServer` boolean - Whether reports should be submitted to the server.

This would normally be controlled by user preferences. This has no effect if
called before `start` is called.

> [!NOTE]
> This method is only available in the main process.

### `crashReporter.addExtraParameter(key, value)`

* `key` string - Parameter key, must be no longer than 39 bytes.
* `value` string - Parameter value, must be no longer than 20320 bytes.

Set an extra parameter to be sent with the crash report. The values specified
here will be sent in addition to any values set via the `extra` option when
`start` was called. Calling this again with the same key replaces the value.
The value is read when a crash happens, so you can update it as your app's
state changes.

Parameters added in this fashion (or via the `extra` parameter to
`crashReporter.start`) are specific to the calling process. Adding extra
parameters in the main process will not cause those parameters to be sent along
with crashes from renderer or other child processes. Similarly, adding extra
parameters in a renderer process will not result in those parameters being sent
with crashes that occur in other renderer processes or in the main process.
Processes created with [`utilityProcess`](utility-process.md) have no API for
setting extra parameters, so only `globalExtra` values are sent with their
crashes.

> [!NOTE]
> Parameters have limits on the length of the keys and values. Key
> names must be no longer than 39 bytes, and values must be no longer than 20320
> bytes. Keys with names longer than the maximum are ignored, and a warning is
> emitted. Values longer than the maximum length are truncated.

### `crashReporter.removeExtraParameter(key)`

* `key` string - Parameter key, must be no longer than 39 bytes.

Remove an extra parameter from the current set of parameters. Future crashes
will not include this parameter.

### `crashReporter.getParameters()`

Returns `Record<string, string>` - The current 'extra' parameters of the crash
reporter in the calling process, as set with the `extra` option and
`addExtraParameter`. Parameters set with the `globalExtra` option are not
included.

## In Node child processes

Since `require('electron')` is not available in Node child processes (processes
run with `ELECTRON_RUN_AS_NODE`, such as those created with
`child_process.fork()`), the following APIs are available on the `process`
object in Node child processes.

If the crash reporter is started in the main process, Node child processes are
monitored automatically. There is no way to start the crash reporter from a
Node child process.

#### `process.crashReporter.getParameters()`

See [`crashReporter.getParameters()`](#crashreportergetparameters).

#### `process.crashReporter.addExtraParameter(key, value)`

See [`crashReporter.addExtraParameter(key, value)`](#crashreporteraddextraparameterkey-value).

#### `process.crashReporter.removeExtraParameter(key)`

See [`crashReporter.removeExtraParameter(key)`](#crashreporterremoveextraparameterkey).

## Crash Report Payload

The crash reporter will send the following data to the `submitURL` as
a `multipart/form-data` `POST`. Unless `compress` is `false`, the request body
is gzip-compressed and sent with `Content-Encoding: gzip`.

* `ver` string - The version of Electron.
* `platform` string - e.g. 'win32'.
* `ptype` string - The type of process that crashed, e.g. 'browser' (the main
  process), 'renderer', 'gpu-process', 'utility' or 'node'.
* `process_type` string - Same as `ptype`. Kept for backwards compatibility.
* `guid` string - e.g. '5e1286fc-da97-479e-918b-6bfb0c3d1c72'. A random ID
  for this installation that stays the same between runs.
* `_version` string - The version in `package.json`.
* `_productName` string - The product name in the `crashReporter` `options`
  object.
* `prod` string - Name of the underlying product. In this case Electron.
* `_companyName` string - The company name in the `crashReporter` `options`
  object. Only sent if the deprecated `companyName` option is set.
* `upload_file_minidump` File - The crash report in the format of `minidump`.
* All level one properties of the `globalExtra` object in the `crashReporter`
  `options` object.
* All extra parameters of the process that crashed, set with the `extra`
  option (main process only) or `addExtraParameter`.

Each parameter is sent as a single form field, with values truncated to 20320
bytes. Other annotations recorded by Chromium, such as `pid` and `plat`, may
also be included.

The body of the server's response is stored as the crash report's ID, and is
returned in the `id` field by
[`getUploadedReports()`](#crashreportergetuploadedreports).
