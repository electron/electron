# Debugging Preload Scripts

Preload scripts run in the renderer process before the web page's own scripts
are loaded, which places them in a middle ground between the two workflows
covered so far: they are not part of the page, yet (when unsandboxed) they are
not part of the main process either. This page describes the debugging options
that are currently observed to work for preload scripts, and how they differ
between sandboxed and unsandboxed renderers.

## Preload scripts in the renderer DevTools

The DevTools opened for a [`BrowserWindow`](../api/browser-window.md) debug the
JavaScript executed in that window's renderer process, including its preload
script. There are a few things worth knowing:

* When [`contextIsolation`](../tutorial/context-isolation.md) is enabled
  (the default), the preload script runs in an isolated context, separate from
  the context the page runs in. The DevTools console exposes a JavaScript
  context dropdown that allows you to switch between the page's context and the
  preload script's isolated context.
* Sources viewed in the DevTools belong to the page's context. To inspect the
  values your preload script exposes, switch the console's context as described
  above, or log values from the preload script itself (see
  [Logging as a fallback](#logging-as-a-fallback)).

## Sandboxed preload scripts

Since Electron 20, renderers are sandboxed by default. A sandboxed preload
script runs without a Node.js environment: it only gets a restricted `require`
function, a reduced [`process`](../api/process.md) object, and a `global` alias.
For details, see the [Process Sandboxing](../tutorial/sandbox.md#preload-scripts)
documentation.

Node.js-based debuggers (such as the one in VS Code or tools using
`--inspect`) attach to Node.js instances. A sandboxed preload script does not
run inside one, so breakpoints set in sandboxed preload code will not bind in
these debuggers. If you observe greyed-out (unbound) breakpoints in a sandboxed
preload script, this is the reason.

If you need debugger support in your preload script, consider whether making
its renderer unsandboxed is an acceptable tradeoff for your application
(see [Unsandboxed preload scripts](#unsandboxed-preload-scripts)).

## Unsandboxed preload scripts

Setting `sandbox: false` in the renderer's `webPreferences` runs the preload
script with a Node.js environment available:

```js title='main.js'
const path = require('node:path')
const { BrowserWindow } = require('electron')

const win = new BrowserWindow({
  webPreferences: {
    preload: path.join(__dirname, 'preload.js'),
    sandbox: false
  }
})
```

In this configuration, breakpoints set in preload code have been observed to
bind in external debuggers that attach to the Node.js inspector, such as VS
Code when [debugging with a Node launch configuration](./debugging-vscode.md),
as well as in the renderer's DevTools.

Note that disabling the sandbox comes with security implications, especially if
any untrusted code or content is loaded in the affected renderer. See
[Disabling the sandbox for a single process](../tutorial/sandbox.md#disabling-the-sandbox-for-a-single-process)
for details and mitigation guidance.

## Logging as a fallback

In all configurations, `console.log` calls in a preload script appear in the
DevTools console of the renderer that loaded it. When the app is launched from
a terminal, this output is also printed to the terminal.

Chromium's logging behavior can be adjusted with the `ELECTRON_ENABLE_LOGGING`
environment variable or the `--enable-logging` command line switch. For more
information, see the
[environment variables documentation](../api/environment-variables.md#electron_enable_logging)
and the
[command line switches documentation](../api/command-line-switches.md#--enable-loggingfile).
