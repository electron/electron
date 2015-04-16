# Debugging the main process

The devtools of browser window can only debug the renderer process scripts.
(I.e. the web pages.) In order to provide a way to debug the scripts of
the main process, Electron has provided the `--debug` and `--debug-brk`
switches.

## Command line switches

### `--debug=[port]`

When this switch is used Electron would listen for V8 debugger protocol
messages on `port`, the `port` is `5858` by default.

### `--debug-brk=[port]`

Like `--debug` but pauses the script on the first line.

## Use node-inspector for debugging

__Note:__ Electron uses node v0.11.13, which currently doesn't work very well
with node-inspector, and the main process would crash if you inspect the
`process` object under node-inspector's console.

### 1. Start the [node-inspector][node-inspector] server

```bash
$ node-inspector
```

### 2. Enable debug mode for Electron

You can either start Electron with a debug flag like:

```bash
$ electron --debug=5858 your/app
```

or, to pause your script on the first line:

```bash
$ electron --debug-brk=5858 your/app
```

### 3. Load the debugger UI

Open http://127.0.0.1:8080/debug?port=5858 in the Chrome browser.

[node-inspector]: https://github.com/node-inspector/node-inspector
