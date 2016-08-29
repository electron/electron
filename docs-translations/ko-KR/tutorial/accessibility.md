> 이 문서는 아직 Electron 기여자가 번역하지 않았습니다.
>
> Electron에 기여하고 싶다면 [기여 가이드](https://github.com/electron/electron/blob/master/CONTRIBUTING-ko.md)를
> 참고하세요.
>
> 문서의 번역이 완료되면 이 틀을 삭제해주세요.

# Accessibility

Making accessible applications is important and we're happy to introduce new functionality to [Devtron](https://electron.atom.io/devtron) and [Spectron](https://electron.atom.io/spectron) that gives developers the opportunity to make their apps better for everyone.

---

Accessibility concerns in Electron applications are similar to those of websites because they're both ultimately HTML. With Electron apps, however, you can't use the online resources for accessibility audits because your app doesn't have a URL to point the auditor to.

These new features bring those auditing tools to your Electron app. You can choose to add audits to your tests with Spectron or use them within DevTools with Devtron. Read on for a summary of the tools or checkout our [accessibility documentation](http://electron.atom.io/docs/tutorials/accessibility) for more information.

### Spectron

In the testing framework Spectron, you can now audit each window and `<webview>` tag in your application. For example:

```javascript
app.client.auditAccessibility().then(function (audit) {
  if (audit.failed) {
    console.error(audit.message)
  }
})
```

You can read more about this feature in [Spectron's documentation](https://github.com/electron/spectron#accessibility-testing).

### Devtron

In Devtron there is a new accessibility tab which will allow you to audit a page in your app, sort and filter the results.

![devtron screenshot](https://cloud.githubusercontent.com/assets/1305617/17156618/9f9bcd72-533f-11e6-880d-389115f40a2a.png)

Both of these tools are using the [Accessibility Developer Tools](https://github.com/GoogleChrome/accessibility-developer-tools) library built by Google for Chrome. You can learn more about the accessibility audit rules this library uses on that [repository's wiki](https://github.com/GoogleChrome/accessibility-developer-tools/wiki/Audit-Rules).

If you know of other great accessibility tools for Electron, add them to the [accessibility documentation](http://electron.atom.io/docs/tutorials/accessibility) with a pull request.
