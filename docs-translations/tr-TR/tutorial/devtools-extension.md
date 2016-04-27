# DevTools Extension

To make debugging easier, Electron has basic support for the
[Chrome DevTools Extension][devtools-extension].

For most DevTools extensions you can simply download the source code and use
the `BrowserWindow.addDevToolsExtension` API to load them. The loaded extensions
will be remembered so you don't need to call the API every time when creating
a window.

** NOTE: React DevTools does not work, follow the issue here https://github.com/electron/electron/issues/915 **

For example, to use the [React DevTools Extension](https://github.com/facebook/react-devtools)
, first you need to download its source code:

```bash
$ cd /some-directory
$ git clone --recursive https://github.com/facebook/react-devtools.git
```

Follow the instructions in [`react-devtools/shells/chrome/Readme.md`](https://github.com/facebook/react-devtools/blob/master/shells/chrome/Readme.md) to build the extension.

Then you can load the extension in Electron by opening DevTools in any window,
and running the following code in the DevTools console:

```javascript
const BrowserWindow = require('electron').remote.BrowserWindow;
BrowserWindow.addDevToolsExtension('/some-directory/react-devtools/shells/chrome');
```

To unload the extension, you can call the `BrowserWindow.removeDevToolsExtension`
API with its name and it will not load the next time you open the DevTools:

```javascript
BrowserWindow.removeDevToolsExtension('React Developer Tools');
```

## Format of DevTools Extension

Ideally all DevTools extensions written for the Chrome browser can be loaded by
Electron, but they have to be in a plain directory. For those packaged with
`crx` extensions, there is no way for Electron to load them unless you find a
way to extract them into a directory.

## Background Pages

Currently Electron doesn't support the background pages feature in Chrome
extensions, so some DevTools extensions that rely on this feature may
not work in Electron.

## `chrome.*` APIs

Some Chrome extensions may use `chrome.*` APIs for features and while there has
been some effort to implement those APIs in Electron, not all have been
implemented.

Given that not all `chrome.*` APIs are implemented if the DevTools extension is
using APIs other than `chrome.devtools.*`, the extension is very likely not to
work. You can report failing extensions in the issue tracker so that we can add
support for those APIs.

[devtools-extension]: https://developer.chrome.com/extensions/devtools
