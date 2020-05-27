# DevTools Extension

Electron supports the [Chrome DevTools Extension][devtools-extension], which can
be used to extend the ability of devtools for debugging popular web frameworks.

## How to load a DevTools Extension

This document outlines the process for manually loading an extension.
You may also try
[electron-devtools-installer](https://github.com/GPMDP/electron-devtools-installer),
a third-party tool that downloads extensions directly from the Chrome WebStore.

To load an extension in Electron, you need to download it in Chrome browser,
locate its filesystem path, and then load it by calling the
`BrowserWindow.addDevToolsExtension(extension)` API.

Using the [React Developer Tools][react-devtools] as example:

1. Install it in Chrome browser.
1. Navigate to `chrome://extensions`, and find its extension ID, which is a hash
   string like `fmkadmapgofadopljbjfkapdkoienihi`.
1. Find out filesystem location used by Chrome for storing extensions:
   * on Windows it is `%LOCALAPPDATA%\Google\Chrome\User Data\Default\Extensions`;
   * on Linux it could be:
     * `~/.config/google-chrome/Default/Extensions/`
     * `~/.config/google-chrome-beta/Default/Extensions/`
     * `~/.config/google-chrome-canary/Default/Extensions/`
     * `~/.config/chromium/Default/Extensions/`
   * on macOS it is `~/Library/Application Support/Google/Chrome/Default/Extensions`.
1. Pass the location of the extension to `BrowserWindow.addDevToolsExtension`
   API, for the React Developer Tools, it is something like:
   ```javascript
   const path = require('path')
   const os = require('os')

   BrowserWindow.addDevToolsExtension(
      path.join(os.homedir(), '/Library/Application Support/Google/Chrome/Default/Extensions/fmkadmapgofadopljbjfkapdkoienihi/4.3.0_0')
   )
   ```

**Note:** The `BrowserWindow.addDevToolsExtension` API cannot be called before the
ready event of the app module is emitted.

The extension will be remembered so you only need to call this API once per
extension. If you try to add an extension that has already been loaded, this method
will not return and instead log a warning to the console.

### How to remove a DevTools Extension

You can pass the name of the extension to the `BrowserWindow.removeDevToolsExtension`
API to remove it. The name of the extension is returned by
`BrowserWindow.addDevToolsExtension` and you can get the names of all installed
DevTools Extensions using the `BrowserWindow.getDevToolsExtensions` API.

## Supported DevTools Extensions

Electron only supports a limited set of `chrome.*` APIs, so some extensions
using unsupported `chrome.*` APIs for chrome extension features may not work.
Following Devtools Extensions are tested and guaranteed to work in Electron:

* [Ember Inspector](https://chrome.google.com/webstore/detail/ember-inspector/bmdblncegkenkacieihfhpjfppoconhi)
* [React Developer Tools](https://chrome.google.com/webstore/detail/react-developer-tools/fmkadmapgofadopljbjfkapdkoienihi)
* [Backbone Debugger](https://chrome.google.com/webstore/detail/backbone-debugger/bhljhndlimiafopmmhjlgfpnnchjjbhd)
* [jQuery Debugger](https://chrome.google.com/webstore/detail/jquery-debugger/dbhhnnnpaeobfddmlalhnehgclcmjimi)
* [AngularJS Batarang](https://chrome.google.com/webstore/detail/angularjs-batarang/ighdmehidhipcmcojjgiloacoafjmpfk)
* [Vue.js devtools](https://chrome.google.com/webstore/detail/vuejs-devtools/nhdogjmejiglipccpnnnanhbledajbpd)
* [Cerebral Debugger](https://cerebraljs.com/docs/introduction/devtools.html)
* [Redux DevTools Extension](https://chrome.google.com/webstore/detail/redux-devtools/lmhkpmbekcpmknklioeibfkpmmfibljd)
* [MobX Developer Tools](https://chrome.google.com/webstore/detail/mobx-developer-tools/pfgnfdagidkfgccljigdamigbcnndkod)

### What should I do if a DevTools Extension is not working?

First please make sure the extension is still being maintained, some extensions
can not even work for recent versions of Chrome browser, and we are not able to
do anything for them.

Then file a bug at Electron's issues list, and describe which part of the
extension is not working as expected.

[devtools-extension]: https://developer.chrome.com/extensions/devtools
[react-devtools]: https://chrome.google.com/webstore/detail/react-developer-tools/fmkadmapgofadopljbjfkapdkoienihi
