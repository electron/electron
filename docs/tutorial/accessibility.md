# Accessibility

Accessibility concerns in Electron applications are similar to those of
websites because they're both ultimately HTML.

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

[a11y-docs]: https://www.chromium.org/developers/design-documents/accessibility#TOC-How-Chrome-detects-the-presence-of-Assistive-Technology
[a11y-devtools]: https://github.com/GoogleChrome/accessibility-developer-tools
[a11y-devtools-wiki]: https://github.com/GoogleChrome/accessibility-developer-tools/wiki/Audit-Rules
[setAccessibilitySupportEnabled]: ../api/app.md#appsetaccessibilitysupportenabledenabled-macos-windows
