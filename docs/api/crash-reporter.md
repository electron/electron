# crash-reporter

An example of automatically submitting crash reporters to remote server:

```javascript
crashReporter = require('crash-reporter');
crashReporter.start({
  productName: 'YourName',
  companyName: 'YourCompany',
  submitUrl: 'https://your-domain.com/url-to-submit',
  autoSubmit: true
});
```

## crashReporter.start(options)

* `options` Object
  * `productName` String, default: Electron
  * `companyName` String, default: GitHub, Inc
  * `submitUrl` String, default: http://54.249.141.255:1127/post
    * URL that crash reports would be sent to as POST
  * `autoSubmit` Boolean, default: true
    * Send the crash report without user interaction
  * `ignoreSystemCrashHandler` Boolean, default: false
  * `extra` Object
    * An object you can define which content will be send along with the report.
    * Only string properties are send correctly.
    * Nested objects are not supported.

Developers are required to call the API before using other crashReporter APIs.


**Note:** On OS X, electron uses a new `crashpad` client, which is different
with the `breakpad` on Windows and Linux. To enable crash collection feature,
you are required to call `crashReporter.start` API to initiliaze `crashpad` in
main process, even you only collect crash report in renderer process.

## crashReporter.getLastCrashReport()

Returns the date and ID of last crash report, when there was no crash report
sent or the crash reporter is not started, `null` will be returned.

## crashReporter.getUploadedReports()

Returns all uploaded crash reports, each report contains date and uploaded ID.

# crash-reporter payload

The crash reporter will send the following data to the `submitUrl` as `POST`:

* `rept` String - e.g. 'electron-crash-service'
* `ver` String - The version of Electron
* `platform` String - e.g. 'win32'
* `process_type` String - e.g. 'renderer'
* `ptime` Number
* `_version` String - The version in `package.json`
* `_productName` String - The product name in the crashReporter `options` object
* `prod` String - Name of the underlying product. In this case Electron
* `_companyName` String - The company name in the crashReporter `options` object
* `upload_file_minidump` File - The crashreport as file
* All level one properties of the `extra` object in the crashReporter `options` object
