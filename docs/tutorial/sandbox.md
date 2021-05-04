# Process Sandboxing

One key security feature in Chromium is that processes can be executed within a sandbox.
The sandbox limits the harm that untrusted code can cause by limiting access to most
system resources — sandboxed processes can only freely use CPU cycles and memory.
In order to perform operations requiring additional privilege, sandboxed processes
use dedicated communication channels to delegate tasks to more privileged processes.

In Chromium, sandboxing is mainly applied to renderer processes. In addition, certain
utility processes are also sandboxed, including the audio service, the GPU service,
and the storage service.

See Chromium's [Sandbox design document][sandbox] for more information.

## Sandboxing in Electron

Electron comes with a mixed sandbox environment, having Chromium's sandbox enabled but
disabling it by default for renderer processes (meaning that it still applies to the
relevant utility processes).

Unsandboxed renderers are not a problem for desktop applications that only display
trusted code, but it makes Electron less secure than Chromium for displaying untrusted
web content. For applications that require more security, sandboxed renderers are
recommended.

Sandboxed processes in Electron behave _mostly_ in the same way as Chromium's do, but
Electron has a few additional quirks to consider.

### Renderer processes

When renderer processes in Electron are sandboxed, they behave in the same way as a
regular Chrome renderer would. A sandboxed renderer won't have a Node.js
environment initialized.

<!-- TODO(erickzhao): when we have a solid guide for IPC, link it here -->
Therefore, when the sandbox is enabled, renderer processes can only make changes to the
system by delegating tasks to the main process via inter-process communication (IPC).

### Preload scripts

In order to allow renderer processes to communicate with the main process, preload
scripts attached to sandboxed renderers will still have a limited Node.js environment
enabled. Node's `require` module is exposed, but will only be able to import a subset
of Electron and Node's built-in modules:

* `electron` (only renderer process modules)
* [`events`](https://nodejs.org/api/events.html)
* [`timers`](https://nodejs.org/api/timers.html)
* [`url`](https://nodejs.org/api/url.html)

In addition, the preload script also polyfills certain Node.js primitives as globals:

* [`Buffer`](https://nodejs.org/api/Buffer.html)
* [`process`](../api/process.md)
* [`clearImmediate`](https://nodejs.org/api/timers.html#timers_clearimmediate_immediate)
* [`setImmediate`](https://nodejs.org/api/timers.html#timers_setimmediate_callback_args)

Note that because the Node.js environment is still present in the `preload`, it is still
possible to leak privileged APIs to untrusted code running in the renderer process unless
[`contextIsolation`][contextIsolation] is enabled.

### Configuring the sandbox

In Electron, renderer sandboxing can be enabled on a per-process basis with
the `sandbox: true` preference in the [`BrowserWindow`][browser-window] constructor.

```js
// main.js
app.whenReady().then(() => {
  const win = new BrowserWindow({
    webPreferences: {
      sandbox: true
    }
  })
  win.loadURL('https://google.com')
})
```

If you want to force sandboxing for all renderers, you can also use the
[`app.enableSandbox`][enable-sandbox] API. Note that this API has to be called before the
app's `ready` event.

```js
// main.js
app.enableSandbox()
app.whenReady().then(() => {
  // no need to pass `sandbox: true` since `app.enableSandbox()` was called.
  const win = new BrowserWindow()
  win.loadURL('https://google.com')
})
```

You can also disable Chromium's sandbox entirely with the [`--no-sandbox`][no-sandbox]
CLI flag, which will disable the sandbox for all processes (including utility processes).
Note that the `sandbox: true` option will still disable the renderer's Node.js
environment.

## A note on rendering untrusted content

Rendering untrusted content in Electron is still somewhat uncharted territory,
though some apps are finding success (e.g. [Beaker Browser][beaker]).
Our goal is to get as close to Chrome as we can in terms of the security of
sandboxed content, but ultimately we will always be behind due to a few fundamental
issues:

1. We do not have the dedicated resources or expertise that Chromium has to
   apply to the security of its product. We do our best to make use of what we
   have, to inherit everything we can from Chromium, and to respond quickly to
   security issues, but Electron cannot be as secure as Chromium without the
   resources that Chromium is able to dedicate.
2. Some security features in Chrome (such as Safe Browsing and Certificate
   Transparency) require a centralized authority and dedicated servers, both of
   which run counter to the goals of the Electron project. As such, we disable
   those features in Electron, at the cost of the associated security they
   would otherwise bring.
3. There is only one Chromium, whereas there are many thousands of apps built
   on Electron, all of which behave slightly differently. Accounting for those
   differences can yield a huge possibility space, and make it challenging to
   ensure the security of the platform in unusual use cases.
4. We can't push security updates to users directly, so we rely on app vendors
   to upgrade the version of Electron underlying their app in order for
   security updates to reach users.

While we make our best effort to backport Chromium security fixes to older
versions of Electron, we do not make a guarantee that every fix will be
backported. Your best chance at staying secure is to be on the latest stable
version of Electron.

[sandbox](https://chromium.googlesource.com/chromium/src/+/master/docs/design/sandbox.md)
[issue-23506](https://github.com/electron/electron/issues/23506)
[browser-window](../api/browser-window.md)
[enable-sandbox](../api/app.md#appenablesandbox)
[no-sandbox](../api/command-line-switches.md#--no-sandbox)
[context-isolation](./context-isolation.md)
[beaker](https://github.com/beakerbrowser/beaker)
