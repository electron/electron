# Electron Fuses

> Package time feature toggles

## What are fuses?

From a security perspective, it makes sense to disable certain unused Electron features
that are powerful but may make your app's security posture weaker. For example, any app that doesn't
use the `ELECTRON_RUN_AS_NODE` environment variable would want to disable the feature to prevent a
subset of "living off the land" attacks.

We also don't want Electron consumers forking to achieve this goal, as building from source and
maintaining a fork is a massive technical challenge and costs a lot of time and money.

Fuses are the solution to this problem. At a high level, they are "magic bits" in the Electron binary
that can be flipped when packaging your Electron app to enable or disable certain features/restrictions.

Because they are flipped at package time before you code sign your app, the OS becomes responsible
for ensuring those bits aren't flipped back via OS-level code signing validation
(e.g. [Gatekeeper](https://support.apple.com/en-ca/guide/security/sec5599b66df/web) on macOS or
[AppLocker](https://learn.microsoft.com/en-us/windows/security/application-security/application-control/app-control-for-business/applocker/applocker-overview)
on Windows).

## Current fuses

### `runAsNode`

**Default:** Enabled

**@electron/fuses:** `FuseV1Options.RunAsNode`

The `runAsNode` fuse toggles whether the [`ELECTRON_RUN_AS_NODE`](../api/environment-variables.md)
environment variable is respected or not. With this fuse disabled, [`child_process.fork`](https://nodejs.org/api/child_process.html#child_processforkmodulepath-args-options) in the main process will not function
as expected, as it depends on this environment variable to function. Instead, we recommend that you
use [Utility Processes](../api/utility-process.md), which work for many use cases where you need a
standalone Node.js process (e.g. a SQLite server process).

### `cookieEncryption`

**Default:** Disabled

**@electron/fuses:** `FuseV1Options.EnableCookieEncryption`

The `cookieEncryption` fuse toggles whether the cookie store on disk is encrypted using OS level
cryptography keys. By default, the SQLite database that Chromium uses to store cookies stores the
values in plaintext. If you wish to ensure your app's cookies are encrypted in the same way Chrome
does, then you should enable this fuse. Please note it is a one-way transition—if you enable this
fuse, existing unencrypted cookies will be encrypted-on-write, but subsequently disabling the fuse
later will make your cookie store corrupt and useless. Most apps can safely enable this fuse.

### `nodeOptions`

**Default:** Enabled

**@electron/fuses:** `FuseV1Options.EnableNodeOptionsEnvironmentVariable`

The `nodeOptions` fuse toggles whether the [`NODE_OPTIONS`](https://nodejs.org/api/cli.html#node_optionsoptions)
and [`NODE_EXTRA_CA_CERTS`](https://github.com/nodejs/node/blob/main/doc/api/cli.md#node_extra_ca_certsfile)
environment variables are respected. The `NODE_OPTIONS` environment variable can be used to pass all
kinds of custom options to the Node.js runtime and isn't typically used by apps in production.
Most apps can safely disable this fuse.

### `nodeCliInspect`

**Default:** Enabled

**@electron/fuses:** `FuseV1Options.EnableNodeCliInspectArguments`

The `nodeCliInspect` fuse toggles whether the `--inspect`, `--inspect-brk`, etc. flags are respected
or not. When disabled, it also ensures that `SIGUSR1` signal does not initialize the main process
inspector. Most apps can safely disable this fuse.

### `embeddedAsarIntegrityValidation`

**Default:** Disabled

**@electron/fuses:** `FuseV1Options.EnableEmbeddedAsarIntegrityValidation`

The `embeddedAsarIntegrityValidation` fuse toggles a feature on macOS and Windows that validates the
content of the `app.asar` file when it is loaded. This feature is designed to have a minimal
performance impact but may marginally slow down file reads from inside the `app.asar` archive.
Most apps can safely enable this fuse.

For more information on how to use ASAR integrity validation, please read the [Asar Integrity](asar-integrity.md) documentation.

### `onlyLoadAppFromAsar`

**Default:** Disabled

**@electron/fuses:** `FuseV1Options.OnlyLoadAppFromAsar`

The `onlyLoadAppFromAsar` fuse changes the search system that Electron uses to locate your app code.
By default, Electron will search for this code in the following order:

1. `app.asar`
1. `app`
1. `default_app.asar`

When this fuse is enabled, Electron will _only_ search for `app.asar`. When combined with the [`embeddedAsarIntegrityValidation`](#embeddedasarintegrityvalidation) fuse, this fuse ensures that
it is impossible to load non-validated code.

### `loadBrowserProcessSpecificV8Snapshot`

**Default:** Disabled

**@electron/fuses:** `FuseV1Options.LoadBrowserProcessSpecificV8Snapshot`

V8 snapshots can be useful to improve app startup performance. V8 lets you take snapshots of
initialized heaps and then load them back in to avoid the cost of initializing the heap.

The `loadBrowserProcessSpecificV8Snapshot` fuse changes which V8 snapshot file is used for the browser
process. By default, Electron's processes will all use the same V8 snapshot file. When this fuse is
enabled, the main process uses the file called `browser_v8_context_snapshot.bin` for its V8 snapshot.
Other processes will use the V8 snapshot file that they normally do.

Using separate snapshots for renderer processes and the main process can improve security, especially
to make sure that the renderer doesn't use a snapshot with `nodeIntegration` enabled.
See [electron/electron#35170](https://github.com/electron/electron/issues/35170) for details.

### `grantFileProtocolExtraPrivileges`

**Default:** Enabled

**@electron/fuses:** `FuseV1Options.GrantFileProtocolExtraPrivileges`

The `grantFileProtocolExtraPrivileges` fuse changes whether pages loaded from the `file://` protocol
are given privileges beyond what they would receive in a traditional web browser. This behavior was
core to Electron apps in original versions of Electron, but is no longer required as apps should be
[serving local files from custom protocols](./security.md#18-avoid-usage-of-the-file-protocol-and-prefer-usage-of-custom-protocols) now instead.

If you aren't serving pages from `file://`, you should disable this fuse.

The extra privileges granted to the `file://` protocol by this fuse are incompletely documented below:

* `file://` protocol pages can use `fetch` to load other assets over `file://`
* `file://` protocol pages can use service workers
* `file://` protocol pages have universal access granted to child frames also running on `file://`
  protocols regardless of sandbox settings

## How do I flip fuses?

### The easy way

[`@electron/fuses`](https://npmjs.com/package/@electron/fuses) is a JavaScript utility designed to make flipping these fuses easy. Check out the README of that module for more details on usage and potential error cases.

```js @ts-nocheck
const { flipFuses, FuseVersion, FuseV1Options } = require('@electron/fuses')

flipFuses(
  // Path to electron
  require('electron'),
  // Fuses to flip
  {
    version: FuseVersion.V1,
    [FuseV1Options.RunAsNode]: false
  }
)
```

You can validate the fuses that have been flipped or check the fuse status of an arbitrary Electron
app using the `@electron/fuses` CLI.

```bash
npx @electron/fuses read --app /Applications/Foo.app
```

>[!NOTE]
> If you are using Electron Forge to distribute your application, you can flip fuses using
> [`@electron-forge/plugin-fuses`](https://www.electronforge.io/config/plugins/fuses),
> which comes pre-installed with all templates.

### The hard way

> [!IMPORTANT]
> Glossary:
>
> * **Fuse Wire**: A sequence of bytes in the Electron binary used to control the fuses
> * **Sentinel**: A static known sequence of bytes you can use to locate the fuse wire
> * **Fuse Schema**: The format/allowed values for the fuse wire

Manually flipping fuses requires editing the Electron binary and modifying the fuse wire to be the
sequence of bytes that represent the state of the fuses you want.

Somewhere in the Electron binary, there will be a sequence of bytes that look like this:

```text
| ...binary | sentinel_bytes | fuse_version | fuse_wire_length | fuse_wire | ...binary |
```

* `sentinel_bytes` is always this exact string: `dL7pKGdnNz796PbbjQWNKmHXBZaB9tsX`
* `fuse_version` is a single byte whose unsigned integer value represents the version of the fuse schema
* `fuse_wire_length` is a single byte whose unsigned integer value represents the number of fuses in the following fuse wire
* `fuse_wire` is a sequence of N bytes, each byte represents a single fuse and its state.
  * "0" (0x30) indicates the fuse is disabled
  * "1" (0x31) indicates the fuse is enabled
  * "r" (0x72) indicates the fuse has been removed and changing the byte to either 1 or 0 will have no effect.

To flip a fuse, you find its position in the fuse wire and change it to "0" or "1" depending on the state you'd like.

You can view the current schema [here](https://github.com/electron/electron/blob/main/build/fuses/fuses.json5).
