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
  * `productName` String
  * `companyName` String
  * `submitUrl` String - URL that crash reports would be sent to
  * `autoSubmit` Boolean - Send the crash report without user interaction
  * `ignoreSystemCrashHandler` Boolean
