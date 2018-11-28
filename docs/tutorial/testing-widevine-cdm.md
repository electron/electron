# Testing Widevine CDM

In Electron you can use the Widevine CDM library shipped with Chrome browser.

Widevine Content Decryption Modules (CDMs) are how streaming services protect
content using HTML5 video to web browsers without relying on an NPAPI plugin
like Flash or Silverlight. Widevine support is an alternative solution for
streaming services that currently rely on Silverlight for playback of
DRM-protected video content. It will allow websites to show DRM-protected video
content in Firefox without the use of NPAPI plugins. The Widevine CDM runs in an
open-source CDM sandbox providing better user security than NPAPI plugins.

#### Note on VMP

As of [`Electron v1.8.0 (Chrome v59)`](https://electronjs.org/releases#1.8.1),
the below steps are may only be some of the necessary steps to enable Widevine;
any app on or after that version intending to use the Widevine CDM may need to
be signed using a license obtained from [Widevine](https://www.widevine.com/)
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

To enable video playback with this new restriction,
[castLabs](https://castlabs.com/open-source/downstream/) has created a
[fork](https://github.com/castlabs/electron-releases) that has implemented the
necessary changes to enable Widevine to be played in an Electron application if
one has obtained the necessary licenses from widevine.

## Getting the library

Open `chrome://components/` in Chrome browser, find `Widevine Content Decryption Module`
and make sure it is up to date, then you can find the library files from the
application directory.

### On Windows

The library file `widevinecdm.dll` will be under
`Program Files(x86)/Google/Chrome/Application/CHROME_VERSION/WidevineCdm/_platform_specific/win_(x86|x64)/`
directory.

### On MacOS

The library file `libwidevinecdm.dylib` will be under
`/Applications/Google Chrome.app/Contents/Versions/CHROME_VERSION/Google Chrome Framework.framework/Versions/A/Libraries/WidevineCdm/_platform_specific/mac_(x86|x64)/`
directory.

**Note:** Make sure that chrome version used by Electron is greater than or
equal to the `min_chrome_version` value of Chrome's widevine cdm component.
The value can be found in `manifest.json` under `WidevineCdm` directory.

## Using the library

After getting the library files, you should pass the path to the file
with `--widevine-cdm-path` command line switch, and the library's version
with `--widevine-cdm-version` switch. The command line switches have to be
passed before the `ready` event of `app` module gets emitted.

Example code:

```javascript
const { app, BrowserWindow } = require('electron')

// You have to pass the directory that contains widevine library here, it is
// * `libwidevinecdm.dylib` on macOS,
// * `widevinecdm.dll` on Windows.
app.commandLine.appendSwitch('widevine-cdm-path', '/path/to/widevine_library')
// The version of plugin can be got from `chrome://plugins` page in Chrome.
app.commandLine.appendSwitch('widevine-cdm-version', '1.4.8.866')

let win = null
app.on('ready', () => {
  win = new BrowserWindow()
  win.show()
})
```

## Verifying Widevine CDM support

To verify whether widevine works, you can use following ways:

* Open https://shaka-player-demo.appspot.com/ and load a manifest that uses
`Widevine`.
* Open http://www.dash-player.com/demo/drm-test-area/, check whether the page
says `bitdash uses Widevine in your browser`, then play the video.
