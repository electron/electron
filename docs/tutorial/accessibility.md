# Accessibility

Making accessible applications is important and we're happy to provide
functionality to [Devtron][devtron] and [Spectron][spectron] that gives
developers the opportunity to make their apps better for everyone.

---

Accessibility concerns in Electron applications are similar to those of
websites because they're both ultimately HTML. With Electron apps, however,
you can't use the online resources for accessibility audits because your app
doesn't have a URL to point the auditor to.

These features bring those auditing tools to your Electron app. You can
choose to add audits to your tests with Spectron or use them within DevTools
with Devtron. Read on for a summary of the tools.

## Spectron

In the testing framework Spectron, you can now audit each window and `<webview>`
tag in your application. For example:

```javascript
app.client.auditAccessibility().then(function (audit) {
  if (audit.failed) {
    console.error(audit.message)
  }
})
```

You can read more about this feature in [Spectron's documentation][spectron-a11y].

## Devtron

In Devtron, there is an accessibility tab which will allow you to audit a
page in your app, sort and filter the results.

![devtron screenshot][devtron-screenshot]

Both of these tools are using the [Accessibility Developer Tools][a11y-devtools]
library built by Google for Chrome. You can learn more about the accessibility
audit rules this library uses on that [repository's wiki][a11y-devtools-wiki].

If you know of other great accessibility tools for Electron, add them to the
accessibility documentation with a pull request.

## Manually enabling accessibility features

Electron applications will automatically enable accessibility features in the
presence of assistive technology (e.g. [JAWS](https://www.freedomscientific.com/products/software/jaws/)
on Windows or [VoiceOver](https://help.apple.com/voiceover/mac/10.15/) on macOS).
See Chrome's [accessibility documentation][a11y-docs] for more details.

You can also manually toggle these features either within your Electron application
or by setting flags in third-party native software.

### Using Electron's API

By using the [`app.setAccessibilitySupportEnabled(enabled)`][setAccessibilitySupportEnabled]
API, you can manually expose Chrome's accessibility tree to users in the application preferences.
Note that the user's system assistive utilities have priority over this setting and
will override it.

### Within third-party software

#### macOS

On macOS, third-party assistive technology can toggle accessibility features inside
Electron applications by setting the `AXManualAccessibility` attribute
programmatically:

```objc
CFStringRef kAXManualAccessibility = CFSTR("AXManualAccessibility");

+ (void)enableAccessibility:(BOOL)enable inElectronApplication:(NSRunningApplication *)app
{
    AXUIElementRef appRef = AXUIElementCreateApplication(app.processIdentifier);
    if (appRef == nil)
        return;

    CFBooleanRef value = enable ? kCFBooleanTrue : kCFBooleanFalse;
    AXUIElementSetAttributeValue(appRef, kAXManualAccessibility, value);
    CFRelease(appRef);
}
```

[devtron]: https://electronjs.org/devtron
[devtron-screenshot]: https://cloud.githubusercontent.com/assets/1305617/17156618/9f9bcd72-533f-11e6-880d-389115f40a2a.png
[spectron]: https://electronjs.org/spectron
[spectron-a11y]: https://github.com/electron/spectron#accessibility-testing
[a11y-docs]: https://www.chromium.org/developers/design-documents/accessibility#TOC-How-Chrome-detects-the-presence-of-Assistive-Technology
[a11y-devtools]: https://github.com/GoogleChrome/accessibility-developer-tools
[a11y-devtools-wiki]: https://github.com/GoogleChrome/accessibility-developer-tools/wiki/Audit-Rules
[setAccessibilitySupportEnabled]: ../api/app.md#appsetaccessibilitysupportenabledenabled-macos-windows
