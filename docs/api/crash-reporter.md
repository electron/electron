# crashReporter

> Submit crash reports to a remote server.

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

## Methods

The `crash-reporter` module has the following methods:

### `crashReporter.start(options)`

* `options` Object
  * `companyName` String
  * `submitURL` String - URL that crash reports will be sent to as POST.
  * `productName` String (optional) - Defaults to `app.getName()`.
  * `autoSubmit` Boolean - Send the crash report without user interaction.
    Default is `true`.
  * `ignoreSystemCrashHandler` Boolean - Default is `false`.
  * `extra` Object - An object you can define that will be sent along with the
    report. Only string properties are sent correctly. Nested objects are not
    supported.

You are required to call this method before using other `crashReporter`
APIs.

**Note:** On macOS, Electron uses a new `crashpad` client, which is different
from `breakpad` on Windows and Linux. To enable the crash collection feature,
you are required to call the `crashReporter.start` API to initialize `crashpad`
in the main process and in each renderer process from which you wish to collect
crash reports.

### `crashReporter.getLastCrashReport()`

Returns the date and ID of the last crash report. If no crash reports have been
sent or the crash reporter has not been started, `null` is returned.

### `crashReporter.getUploadedReports()`

Returns all uploaded crash reports. Each report contains the date and uploaded
ID.

### `crashReporter.setExtraParameters(extra)`

* `extra` Object - An object you can define that will be sent along with the
  report. Only string properties are sent correctly. Nested objects are not
  supported.

Updates the extra value that will be sent along with the report. This allows
you to add extra data while the crashReporter is already started. It could be
possible that you did not have access to certain data while you wanted to start
the crashReporter.

## crash-reporter Payload

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
* All level one properties of the `extra` object in the `crashReporter`.
  `options` object
