# desktopCapturer

> Access information about media sources that can be used to capture audio and
> video from the desktop using the [`navigator.mediaDevices.getUserMedia`][] API.

Process: [Main](../glossary.md#main-process)

The following example shows how to capture video from a desktop window whose
title is `Electron`:

```js
// main.js
const { app, BrowserWindow, desktopCapturer, session } = require('electron')

app.whenReady().then(() => {
  const mainWindow = new BrowserWindow()

  session.defaultSession.setDisplayMediaRequestHandler(
    (request, callback) => {
      desktopCapturer.getSources({ types: ['screen'] }).then(
        (sources) => {
          // Grant access to the first screen found.
          callback({ video: sources[0], audio: 'loopback' })
        },
        () => {
          // Deny the request if no sources could be retrieved, for example
          // when the user cancels the PipeWire picker on Linux.
          callback(null)
        }
      )
      // If true, use the system picker if available.
      // Note: this is currently experimental. If the system picker
      // is available, it will be used and the media request handler
      // will not be invoked.
    },
    { useSystemPicker: true }
  )

  mainWindow.loadFile('index.html')
})
```

```js
// renderer.js
const startButton = document.getElementById('startButton')
const stopButton = document.getElementById('stopButton')
const video = document.querySelector('video')

startButton.addEventListener('click', () => {
  navigator.mediaDevices
    .getDisplayMedia({
      audio: true,
      video: {
        width: 320,
        height: 240,
        frameRate: 30
      }
    })
    .then((stream) => {
      video.srcObject = stream
      video.onloadedmetadata = (e) => video.play()
    })
    .catch((e) => console.log(e))
})

stopButton.addEventListener('click', () => {
  video.pause()
})
```

```html
<!-- index.html -->
<html>
<meta http-equiv="content-security-policy" content="script-src 'self' 'unsafe-inline'" />
  <body>
    <button id="startButton" class="button">Start</button>
    <button id="stopButton" class="button">Stop</button>
    <video width="320" height="240" autoplay></video>
    <script src="renderer.js"></script>
  </body>
</html>
```

See [`navigator.mediaDevices.getDisplayMedia`](https://developer.mozilla.org/en-US/docs/Web/API/MediaDevices/getDisplayMedia) for more information.

> [!NOTE]
> `navigator.mediaDevices.getDisplayMedia` does not permit the use of `deviceId` for
> selection of a source - see [specification](https://w3c.github.io/mediacapture-screen-share/#constraints).

## Methods

The `desktopCapturer` module has the following methods:

### `desktopCapturer.getSources(options)`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/2963
changes:
  - pr-url: https://github.com/electron/electron/pull/16427
    description: "This method now returns a Promise instead of using a callback function."
    breaking-changes-header: api-changed-callback-based-versions-of-promisified-apis
  - pr-url: https://github.com/electron/electron/pull/54550
    description: "Added the `persistMode` and `restoreToken` options."
```
-->

* `options` Object
  * `types` string[] - An array of strings that lists the types of desktop sources
    to be captured, available types can be `screen` and `window`.
  * `thumbnailSize` [Size](structures/size.md) (optional) - The size that the media source thumbnail
    should be scaled to. Default is `150` x `150`. Set width or height to 0 when you do not need
    the thumbnails. This will save the processing time required for capturing the content of each
    window and screen.
  * `fetchWindowIcons` boolean (optional) - Set to true to enable fetching window icons. The default
    value is false. When false the appIcon property of the sources return null. Same if a source has
    the type screen.
  * `persistMode` string (optional) _Linux_ _Experimental_ - Can be `transient` or `persistent`.
    How long the source the user picks in the system picker can be reopened without showing the
    picker again. `transient`, the default, lasts at most until the app exits. `persistent` lasts
    across launches until the user revokes it; store the returned source's `restoreToken` to use
    it on a later launch. See [Restoring a source on Wayland](#restoring-a-source-on-wayland).
  * `restoreToken` string (optional) _Linux_ _Experimental_ - A `restoreToken` from an earlier
    call. If the system still honors it, the promise resolves with that source and no picker is
    shown. A token that is malformed, revoked or names a source that no longer exists is ignored
    and the picker is shown as if no token was passed. Pass `persistMode: 'persistent'` with a
    persistent token every time: restoring it as `transient` ends the persistent grant.

Returns `Promise<DesktopCapturerSource[]>` - Resolves with an array of [`DesktopCapturerSource`](structures/desktop-capturer-source.md) objects, each `DesktopCapturerSource` represents a screen or an individual window that can be captured.

> [!NOTE]
<!-- markdownlint-disable-next-line MD032 -->
> * Capturing audio requires `NSAudioCaptureUsageDescription` Info.plist key on macOS 14.2 Sonoma and higher - [read more](#macos-versions-142-or-higher).
> * Capturing the screen contents requires user consent on macOS 10.15 Catalina or higher, which can detected by [`systemPreferences.getMediaAccessStatus`][].

### `desktopCapturer.getRestoreToken(sourceId)` _Linux_ _Experimental_

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/54550
```
-->

* `sourceId` string - The `id` of a [`DesktopCapturerSource`](structures/desktop-capturer-source.md).

Returns `string` - The current restore token for the source, or an empty string if it has none.

The system may replace the token each time the source is opened, which happens once in
`desktopCapturer.getSources` and again when a stream is started from the source, and then only
the newest token works on the next launch. Read it with this method once the stream is running,
or before the app quits, and store that value rather than the one `getSources` returned. See
[Restoring a source on Wayland](#restoring-a-source-on-wayland).

[`navigator.mediaDevices.getUserMedia`]: https://developer.mozilla.org/en/docs/Web/API/MediaDevices/getUserMedia
[`systemPreferences.getMediaAccessStatus`]: system-preferences.md#systempreferencesgetmediaaccessstatusmediatype-windows-macos

## Caveats

### Linux

`desktopCapturer.getSources(options)` only returns a single source on Linux when using Pipewire.

PipeWire supports a single capture for both screens and windows. If you request the window and screen type, the selected source will be returned as a window capture.

#### Restoring a source on Wayland

On Wayland the source is chosen in the system's own picker, which `desktopCapturer.getSources`
shows on every call. To let the user pick once and capture that choice again on later launches,
pass `persistMode: 'persistent'`, store the source's restore token, and pass it back as
`restoreToken` next time. Electron does not store tokens itself. A token names a screen or
window the user already agreed to share, so keep it somewhere private to the app, for example
encrypted with [`safeStorage`](safe-storage.md). If the user has revoked the grant, the screen or
window no longer exists, or the stored value is not a valid token, it is ignored and the picker is
shown again.

How a token is matched to a source is up to the desktop environment. GNOME and KDE remember a
window by its app and title, and restore the open window of that app whose title is closest, so a
token can resolve to a different window of the same app. If no title is close enough the picker is
shown.

With a valid token nothing asks the user before capture starts. Show the user what is being
shared, and let them choose something else by calling `desktopCapturer.getSources` again without
`restoreToken`.

This needs `xdg-desktop-portal` with version 4 or later of the ScreenCast interface. On X11 and
on other platforms the options are ignored and `restoreToken` is always empty.

```js
const { app, desktopCapturer, safeStorage, session } = require('electron')

const fs = require('node:fs/promises')
const path = require('node:path')

const tokenFile = path.join(app.getPath('userData'), 'screencast-token')

async function readToken() {
  try {
    const { result } = await safeStorage.decryptStringAsync(await fs.readFile(tokenFile))
    return result
  } catch {
    return undefined
  }
}

async function writeToken(token) {
  if (token) await fs.writeFile(tokenFile, await safeStorage.encryptStringAsync(token))
}

app.whenReady().then(() => {
  let sourceId

  session.defaultSession.setDisplayMediaRequestHandler(async (request, callback) => {
    let sources
    try {
      sources = await desktopCapturer.getSources({
        types: ['screen', 'window'],
        persistMode: 'persistent',
        restoreToken: await readToken()
      })
    } catch {
      // The user closed the picker without choosing anything.
      callback(null)
      return
    }
    const [source] = sources
    sourceId = source.id
    await writeToken(source.restoreToken)
    callback({ video: source })
  })

  app.on('will-quit', (event) => {
    if (!sourceId) return
    event.preventDefault()
    writeToken(desktopCapturer.getRestoreToken(sourceId)).finally(() => {
      sourceId = undefined
      app.quit()
    })
  })
})
```

### macOS versions 14.2 or higher

`NSAudioCaptureUsageDescription` Info.plist key must be added in order for audio to be captured by
`desktopCapturer`. If instead you are running Electron from another program like a terminal or IDE
then that parent program must contain the Info.plist key.

This is in order to facilitate use of Apple's [CoreAudio Tap API](https://developer.apple.com/documentation/CoreAudio/capturing-system-audio-with-core-audio-taps#Configure-the-sample-code-project) by Chromium,
which is gated behind the "System Audio Recording" privacy permission. macOS attributes that
permission to the responsible process, so when running unpackaged from a terminal or IDE it is the
terminal or IDE that must be granted access.

> [!WARNING]
> If the permission is missing or has been denied, `desktopCapturer` still produces an audio
> track, but it is created in the `ended` state and never delivers samples. No warning or error is
> surfaced to JavaScript.

Since Electron 39, Chromium [uses the CoreAudio Tap API by default](https://source.chromium.org/chromium/chromium/src/+/ad17e8f8b93d5f34891b06085d373a668918255e)
for system audio capture on macOS 14.2 and later. There is no automatic fallback to the older
ScreenCaptureKit-based "Screen & System Audio Recording" path if tap creation fails, and as of
Electron 45 the `MacCatapLoopbackAudioForScreenShare` feature flag that previously allowed opting
back into it has been [removed upstream](https://chromium-review.googlesource.com/c/chromium/src/+/8275391)
and no longer has any effect.

### macOS versions 12.7.6 or lower

`navigator.mediaDevices.getUserMedia` does not work on macOS versions 12.7.6 and prior for audio
capture due to a fundamental limitation whereby apps that want to access the system's audio require
a [signed kernel extension](https://developer.apple.com/library/archive/documentation/Security/Conceptual/System_Integrity_Protection_Guide/KernelExtensions/KernelExtensions.html).
Chromium, and by extension Electron, does not provide this. Only in macOS 13 and onwards does Apple
provide APIs to capture desktop audio without the need for a signed kernel extension.

It is possible to circumvent this limitation by capturing system audio with another macOS app like
[BlackHole](https://existential.audio/blackhole/) or [Soundflower](https://rogueamoeba.com/freebies/soundflower/)
and passing it through a virtual audio input device. This virtual device can then be queried
with `navigator.mediaDevices.getUserMedia`.
