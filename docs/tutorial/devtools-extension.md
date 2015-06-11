# DevTools extension

To make debugging easier, Electron has basic support for
[Chrome DevTools Extension][devtools-extension].

For most devtools extensions, you can simply download the source code and use
the `BrowserWindow.addDevToolsExtension` API to load them, the loaded extensions
will be remembered so you don't need to call the API every time when creating
a window.

For example to use the [React DevTools Extension](https://github.com/facebook/react-devtools), first you need to download its
source code:

```bash
$ cd /some-directory
$ git clone --recursive https://github.com/facebook/react-devtools.git
```

Then you can load the extension in Electron by opening devtools in any window,
and then running the following code in the devtools console:

```javascript
require('remote').require('browser-window').addDevToolsExtension('/some-directory/react-devtools');
```

To unload the extension, you can call `BrowserWindow.removeDevToolsExtension`
API with its name and it will not load the next time you open the devtools:

```javascript
require('remote').require('browser-window').removeDevToolsExtension('React Developer Tools');
```

## Format of devtools extension

Ideally all devtools extension written for Chrome browser can be loaded by
Electron, but they have to be in a plain directory, for those packaged `crx`
extensions, there is no way for Electron to load them unless you find a way to
extract them into a directory.

## Background pages

Currently Electron doesn't support the background pages feature in chrome extensions,
so for some devtools extensions that rely on this feature, they may not work in Electron.

## `chrome.*` APIs

Some chrome extensions use `chrome.*` APIs for some features, there has been some
effort to implement those APIs in Electron, however not all are implemented.

Given that not all `chrome.*` APIs are implemented if the devtools extension is using APIs other than `chrome.devtools.*`, the extension is very likely not to work. You can report failing extensions in the issue tracker so that we can add support for those APIs.

[devtools-extension]: https://developer.chrome.com/extensions/devtools
