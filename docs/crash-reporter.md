## Synopsis

An example of automatically submitting crash reporters to remote server:

```javascript
crashReporter = require('crash-reporter');
crashReporter.setCompanyName('YourCompany');
crashReporter.setSubmissionUrl('https://your-domain.com/url-to-submit');
crashReporter.setAutoSubmit(true);
```

## crashReporter.setCompanyName(company)

* `company` String

## crashReporter.setSubmissionUrl(url)

* `url` String

## crashReporter.setAutoSubmit(is)

* `is` Boolean
