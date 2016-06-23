# Debugging the Main Process

The browser window DevTools can only debug the renderer process scripts (i.e.
the web pages). In order to provide a way to debug the scripts from the main
process, Electron has provided the `--debug` and `--debug-brk` switches.

## Command Line Switches

Use the following command line switches to debug Electron's main process:

### `--debug=[port]`

When this switch is used Electron will listen for V8 debugger protocol
messages on `port`. The default `port` is `5858`.

### `--debug-brk=[port]`

Like `--debug` but pauses the script on the first line.

## Use node-inspector for Debugging

**Note:** Electron doesn't currently work very well
with node-inspector, and the main process will crash if you inspect the
`process` object under node-inspector's console.

### 1. Make sure you have [node-gyp required tools][node-gyp-required-tools] installed

### 2. Install [node-inspector][node-inspector]

```bash
$ npm install node-inspector
```

### 3. Install [node-pre-gyp][node-pre-gyp]

```bash
$ npm install node-pre-gyp
```

### 4. Recompile the `node-inspector` `v8` modules for electron (change the target to your electron version number)

```bash
$ node_modules/.bin/node-pre-gyp --target=0.36.11 --runtime=electron --fallback-to-build --directory node_modules/v8-debug/ --dist-url=https://atom.io/download/atom-shell reinstall
$ node_modules/.bin/node-pre-gyp --target=0.36.11 --runtime=electron --fallback-to-build --directory node_modules/v8-profiler/ --dist-url=https://atom.io/download/atom-shell reinstall
```

See also [How to install native modules][how-to-install-native-modules].

### 5. Enable debug mode for Electron

You can either start Electron with a debug flag like:

```bash
$ electron --debug=5858 your/app
```

or, to pause your script on the first line:

```bash
$ electron --debug-brk=5858 your/app
```

### 6. Start the [node-inspector][node-inspector] server using electron

```bash
$ ELECTRON_RUN_AS_NODE=true path/to/electron.exe node_modules/node-inspector/bin/inspector.js
```

### 7. Load the debugger UI

Open http://127.0.0.1:8080/debug?ws=127.0.0.1:8080&port=5858 in the Chrome
browser. You may have to click pause if starting with debug-brk to see the
entry line.

[node-inspector]: https://github.com/node-inspector/node-inspector
[node-pre-gyp]: https://github.com/mapbox/node-pre-gyp
[node-gyp-required-tools]: https://github.com/nodejs/node-gyp#installation
[how-to-install-native-modules]: using-native-node-modules.md#how-to-install-native-modules
