# Debugging the Main Process

The DevTools in an Electron browser window can only debug JavaScript that's
executed in that window (i.e. the web pages). To debug JavaScript that's
executed in the main process you will need to use an external debugger and
launch Electron with the `--inspect` or `--inspect-brk` switch.

## Command Line Switches

Use one of the following command line switches to enable debugging of the main
process:

### `--inspect=[port]`

Electron will listen for V8 inspector protocol messages on the specified `port`,
an external debugger will need to connect on this port. The default `port` is
`5858`.

```shell
electron --inspect=5858 your/app
```

### `--inspect-brk=[port]`

Like `--inspector` but pauses execution on the first line of JavaScript.

## External Debuggers

You will need to use a debugger that supports the V8 inspector protocol.

- Connect Chrome by visiting `chrome://inspect` and selecting to inspect the
  launched Electron app present there.
- [Debugging the Main Process in VSCode](debugging-main-process-vscode.md)
