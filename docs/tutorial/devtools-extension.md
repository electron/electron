# DevTools extension

To make debugging more easy, Electron has added basic support for
[Chrome DevTools Extension][devtools-extension].

For most devtools extensions, you can simply download their source codes and use
the `BrowserWindow.addDevToolsExtension` API to load them, the loaded extensions
will be remembered so you don't need to call the API every time when creating
a window.

For example to use the React DevTools Extension, first you need to download its
source code:

```bash
$ cd /some-directory
$ git clone --recursive https://github.com/facebook/react-devtools.git
```

Then you can load it in Electron by opening the devtools in arbitray window,
and run this code in the console of devtools:

```javascript
require('remote').require('browser-window').addDevToolsExtension('/some-directory/react-devtools')
```

To unload the extension, you can call `BrowserWindow.removeDevToolsExtension`
API with its name and it will disappear the next time you open the devtools:

```javascript
require('remote').require('browser-window').removeDevToolsExtension('React Developer Tools')
```

## Format of devtools extension

Ideally all devtools extension written for Chrome browser can be loaded by
Electron, but they have to be in a plain directory, for those packaged `crx`
extensions, there is no way in Electron to load them unless you find a way to
extract them into a directory.

## Background pages

Currently Electron doesn't support the background pages of chrome extensions,
so for some devtools extensions that rely on this feature, they may not work
well in Electron

## `chrome.*` APIs

Some chrome extensions use `chrome.*` APIs for some features, there is some
effort to implement those APIs in Electron to make them work, but we have
only implemented few for now.

So if the devtools extension is using APIs other than `chrome.devtools.*`, it is
very likely to fail, but you can report those extensions in the issues tracker
so we can add support for those APIs.

[devtools-extension]: https://developer.chrome.com/extensions/devtools
