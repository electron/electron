# crashReporter

> Submit crash reports to a remote server.

Process: [Main](../tutorial/quick-start.md#main-process), [Renderer](../tutorial/quick-start.md#renderer-process)

The following is an example of automatically submitting a crash report to a
remote server:

```javascript
const {crashReporter} = require('electron')

crashReporter.start({
  productName: 'YourName',
  companyName: 'YourCompany',
  submitURL: 'https://your-domain.com/url-to-submit',
  autoSubmit: true
})
```

For setting up a server to accept and process crash reports, you can use
following projects:

* [socorro](https://github.com/mozilla/socorro)
* [mini-breakpad-server](https://github.com/electron/mini-breakpad-server)

Crash reports are saved locally in an application-specific temp directory folder.
For a `productName` of `YourName`, crash reports will be stored in a folder
named `YourName Crashes` inside the temp directory. You can customize this temp
directory location for your app by calling the `app.setPath('temp', '/my/custom/temp')`
API before starting the crash reporter.

## Methods

The `crashReporter` module has the following methods:

### `crashReporter.start(options)`

* `options` Object
  * `companyName` String (optional)
  * `submitURL` String - URL that crash reports will be sent to as POST.
  * `productName` String (optional) - Defaults to `app.getName()`.
  * `autoSubmit` Boolean (optional) - Send the crash report without user interaction.
    Default is `true`.
  * `ignoreSystemCrashHandler` Boolean (optional) - Default is `false`.
  * `extra` Object (optional) - An object you can define that will be sent along with the
    report. Only string properties are sent correctly, Nested objects are not
    supported.

You are required to call this method before using any other `crashReporter` APIs
and in each process (main/renderer) from which you want to collect crash reports.
You can pass different options to `crashReporter.start` when calling from different processes.

**Note:** On Windows and Linux, Electron uses `breakpad` for crash collection and reporting.
Crashes can be collected from the main and renderer process, but not from the child processes 
created via the `child_process` module.

**Note:** On macOS, Electron uses a new `crashpad` client for crash collection and reporting.
Crashes can be collected from the main, renderer and any of the child processes created via the `child_process` module.
If you want to enable crash reporting, initializing `crashpad` from the main process using `crashReporter.start` is required 
regardless of which process you want to collect crashes from. Once initialized this way, the crashpad handler collects
crashes from all processes. You still have to call `crashReporter.start` from the renderer process, otherwise crashes from
renderer processes will get reported without `companyName`, `productName` or any of the `extra` information.

### `crashReporter.getLastCrashReport()`

Returns [`CrashReport`](structures/crash-report.md):

Returns the date and ID of the last crash report. If no crash reports have been
sent or the crash reporter has not been started, `null` is returned.

### `crashReporter.getUploadedReports()`

Returns [`CrashReport[]`](structures/crash-report.md):

Returns all uploaded crash reports. Each report contains the date and uploaded
ID.

## Crash Report Payload

The crash reporter will send the following data to the `submitURL` as
a `multipart/form-data` `POST`:

* `ver` String - The version of Electron.
* `platform` String - e.g. 'win32'.
* `process_type` String - e.g. 'renderer'.
* `guid` String - e.g. '5e1286fc-da97-479e-918b-6bfb0c3d1c72'
* `_version` String - The version in `package.json`.
* `_productName` String - The product name in the `crashReporter` `options`
  object.
* `prod` String - Name of the underlying product. In this case Electron.
* `_companyName` String - The company name in the `crashReporter` `options`
  object.
* `upload_file_minidump` File - The crash report in the format of `minidump`.
* All level one properties of the `extra` object in the `crashReporter`
  `options` object.
