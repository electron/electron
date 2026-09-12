# Testing Widevine CDM

In Electron you can use the Widevine CDM library in builds compiled with
Widevine support.

Widevine Content Decryption Modules (CDMs) are how streaming services protect
content using HTML5 video in web browsers without relying on an NPAPI plugin
like Flash or Silverlight. Electron can register a Widevine CDM when the
Electron binary is built with Widevine support and the application provides
compatible CDM files.

> [!NOTE]
> Widevine support in Electron is experimental.

## Note on VMP

As of [`Electron v1.8.0 (Chrome v59)`](https://electronjs.org/releases#1.8.1),
the below steps may only be some of the necessary steps to enable Widevine; any
app on or after that version intending to use the Widevine CDM may need to be
signed using a license obtained from [Widevine](https://www.widevine.com/)
itself.

Per [Widevine](https://www.widevine.com/):

> Chrome 59 (and later) includes support for Verified Media Path (VMP). VMP
> provides a method to verify the authenticity of a device platform. For browser
> deployments, this will provide an additional signal to determine if a
> browser-based implementation is reliable and secure.
>
> The proxy integration guide has been updated with information about VMP and
> how to issue licenses.
>
> Widevine recommends our browser-based integrations (vendors and browser-based
> applications) add support for VMP.

Electron does not download, update, license, or sign Widevine CDM files for
applications. Application developers are responsible for obtaining any required
CDM files, licenses, and VMP signing support from Widevine or an authorized
integration provider.

## Using the library

When developing an unpackaged application, pass the path to the directory that
contains `manifest.json` with the `--widevine-cdm-path` command line switch.
Electron reads the CDM version from `manifest.json` when possible. If the
manifest does not contain the version, pass the library's version with
`--widevine-cdm-version`.

On Linux, these switches must be present on Electron's process command line
for an unpackaged application. The zygote loads library CDMs before Electron
runs the app's main script, so calling `app.commandLine.appendSwitch` from that
script is too late for sandboxed renderers:

```sh
electron \
  --widevine-cdm-path=/path/to/WidevineCdm \
  --widevine-cdm-version=4.10.2891.0 \
  /path/to/app
```

The version switch is optional when `manifest.json` contains a valid version.

On Windows and macOS, an unpackaged application can append the switches from
its main script before the `ready` event is emitted.

Example code for Windows and macOS:

```js
const { app, BrowserWindow } = require('electron')

app.commandLine.appendSwitch('widevine-cdm-path', '/path/to/WidevineCdm')

// Optional when manifest.json contains a valid version.
app.commandLine.appendSwitch('widevine-cdm-version', '4.10.2891.0')

let win = null
app.whenReady().then(() => {
  win = new BrowserWindow()
  win.show()
})
```

Packaged applications ignore `--widevine-cdm-path` to prevent loading library
code from an arbitrary command-line path. On all platforms, packaged
applications must bundle the `WidevineCdm` directory beside the Electron
executable.

## Verifying Widevine CDM support

To verify whether Widevine is registered, you can use the
[Encrypted Media Extensions API][eme]:

```js
const config = [{
  initDataTypes: ['cenc'],
  videoCapabilities: [{
    contentType: 'video/webm; codecs="vp8"'
  }],
  audioCapabilities: [{
    contentType: 'audio/webm; codecs="opus"'
  }]
}]

navigator.requestMediaKeySystemAccess('com.widevine.alpha', config)
  .then(() => {
    console.log('Widevine is available')
  })
  .catch((error) => {
    console.error('Widevine is unavailable:', error.name)
  })
```

This check only verifies key system availability. Successful protected playback
also depends on the CDM package, media codec support, the license server, and
any VMP or application-signing requirements imposed by the content provider.

[eme]: https://developer.mozilla.org/en-US/docs/Web/API/Encrypted_Media_Extensions_API
