# desktopCapturer

> Access information about media sources that can be used to capture audio and
> video from the desktop using the [`navigator.mediaDevices.getDisplayMedia`][] API.

Process: [Main](../glossary.md#main-process)

> [!WARNING]
> The `desktopCapturer` module is deprecated. Use
> [`session.setDisplayMediaRequestHandler`](session.md#sessetdisplaymediarequesthandleropts)
> instead, which aligns with the web-standard
> [`navigator.mediaDevices.getDisplayMedia`](https://developer.mozilla.org/en-US/docs/Web/API/MediaDevices/getDisplayMedia)
> API. See the [migration guide](#migrating-from-getsources-to-setdisplaymediarequesthandler) below.

The following example shows how to capture video from a desktop window whose
title is `Electron`:

```js
// main.js
const { app, BrowserWindow, session } = require('electron')

app.whenReady().then(() => {
  const mainWindow = new BrowserWindow()

  session.defaultSession.setDisplayMediaRequestHandler((request, callback) => {
    // Allow the requesting tab to capture itself.
    callback({ video: request.frame })
  })

  mainWindow.loadFile('index.html')
})
```

```js
// renderer.js
const startButton = document.getElementById('startButton')
const stopButton = document.getElementById('stopButton')
const video = document.querySelector('video')

startButton.addEventListener('click', () => {
  navigator.mediaDevices.getDisplayMedia({
    audio: true,
    video: {
      width: 320,
      height: 240,
      frameRate: 30
    }
  }).then(stream => {
    video.srcObject = stream
    video.onloadedmetadata = (e) => video.play()
  }).catch(e => console.log(e))
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

### `desktopCapturer.getSources(options)` _Deprecated_

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/2963
changes:
  - pr-url: https://github.com/electron/electron/pull/16427
    description: "This method now returns a Promise instead of using a callback function."
    breaking-changes-header: api-changed-callback-based-versions-of-promisified-apis
deprecated:
  - pr-url: https://github.com/electron/electron/pull/51235
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

Returns `Promise<DesktopCapturerSource[]>` - Resolves with an array of [`DesktopCapturerSource`](structures/desktop-capturer-source.md) objects, each `DesktopCapturerSource` represents a screen or an individual window that can be captured.

**Deprecated:** Use [`session.setDisplayMediaRequestHandler`](session.md#sessetdisplaymediarequesthandlerhandler-opts) instead.

> [!NOTE]
<!-- markdownlint-disable-next-line MD032 -->
> * Capturing audio requires `NSAudioCaptureUsageDescription` Info.plist key on macOS 14.2 Sonoma and higher - [read more](#macos-versions-142-or-higher).
> * Capturing the screen contents requires user consent on macOS 10.15 Catalina or higher, which can detected by [`systemPreferences.getMediaAccessStatus`][].

[`navigator.mediaDevices.getDisplayMedia`]: https://developer.mozilla.org/en-US/docs/Web/API/MediaDevices/getDisplayMedia
[`navigator.mediaDevices.getUserMedia`]: https://developer.mozilla.org/en/docs/Web/API/MediaDevices/getUserMedia
[`systemPreferences.getMediaAccessStatus`]: system-preferences.md#systempreferencesgetmediaaccessstatusmediatype-windows-macos

## Caveats

### Linux

`desktopCapturer.getSources(options)` only returns a single source on Linux when using Pipewire.

PipeWire supports a single capture for both screens and windows. If you request the window and screen type, the selected source will be returned as a window capture.

### macOS versions 14.2 or higher

`NSAudioCaptureUsageDescription` Info.plist key must be added in order for audio to be captured by
`desktopCapturer`. If instead you are running Electron from another program like a terminal or IDE
then that parent program must contain the Info.plist key.

This is in order to facillitate use of Apple's new [CoreAudio Tap API](https://developer.apple.com/documentation/CoreAudio/capturing-system-audio-with-core-audio-taps#Configure-the-sample-code-project) by Chromium.

> [!WARNING]
> Failure of `desktopCapturer` to start an audio stream due to `NSAudioCaptureUsageDescription`
> permission not present will still create a dead audio stream however no warnings or errors are
> displayed.

As of Electron `v39.0.0-beta.4`, Chromium [made Apple's new `CoreAudio Tap API` the default](https://source.chromium.org/chromium/chromium/src/+/ad17e8f8b93d5f34891b06085d373a668918255e)
for desktop audio capture. There is no fallback to the older `Screen & System Audio Recording`
permissions system even if [CoreAudio Tap API](https://developer.apple.com/documentation/CoreAudio/capturing-system-audio-with-core-audio-taps) stream creation fails.

If you need to continue using `Screen & System Audio Recording` permissions for `desktopCapturer`
on macOS versions 14.2 and later, you can apply a Chromium feature flag to force use of that older
permissions system:

```js
// main.js (right beneath your require/import statments)
app.commandLine.appendSwitch('disable-features', 'MacCatapLoopbackAudioForScreenShare')
```

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

## Migrating from `getSources` to `setDisplayMediaRequestHandler`

`desktopCapturer.getSources` is deprecated. Applications should migrate to
[`session.setDisplayMediaRequestHandler`](session.md#sessetdisplaymediarequesthandlerhandler-opts),
which aligns with the web-standard [`navigator.mediaDevices.getDisplayMedia`][] API.

### Why migrate?

- **Web standards alignment** — `getDisplayMedia()` is the web-standard API for screen
  capture. Using `setDisplayMediaRequestHandler` lets you integrate with it directly.
- **Platform integration** — On macOS 15+, setting `useSystemPicker: true` invokes the
  native OS screen picker (SCContentSharingPicker), providing a familiar UX and
  correct permissions handling. On other platforms and earlier macOS versions the
  option is a no-op and the handler runs as normal.
- **Simpler architecture** — The handler receives the request when the renderer calls
  `getDisplayMedia()` and you respond with the chosen source. No need to pre-enumerate
  sources.

### Before (deprecated)

```js
const { app, BrowserWindow, desktopCapturer, session } = require('electron')

app.whenReady().then(() => {
  const mainWindow = new BrowserWindow()

  session.defaultSession.setDisplayMediaRequestHandler((request, callback) => {
    desktopCapturer.getSources({ types: ['screen'] }).then((sources) => {
      callback({ video: sources[0] })
    })
  })

  mainWindow.loadFile('index.html')
})
```

### After (recommended)

On macOS 15 and later, the simplest migration is to let the OS present its own
picker. When the system picker is available, the handler is not invoked:

```js
const { app, BrowserWindow, session } = require('electron')

app.whenReady().then(() => {
  const mainWindow = new BrowserWindow()

  session.defaultSession.setDisplayMediaRequestHandler((request, callback) => {
    // Fallback for platforms / OS versions where the system picker is unavailable.
    callback({})
  }, { useSystemPicker: true })

  mainWindow.loadFile('index.html')
})
```

If the renderer only needs to capture a specific `WebFrameMain` (for example,
the requesting tab itself), no source enumeration is needed — pass the frame
straight through:

```js
session.defaultSession.setDisplayMediaRequestHandler((request, callback) => {
  // request.preferredDisplaySurface is 'monitor', 'window', 'browser', or 'none'
  callback({ video: request.frame })
})
```

For a fully custom picker UI on platforms where the system picker is not
available, enumerate screens and windows inside the handler and respond with
the user's selection. There is currently no web-standard replacement for
source enumeration, so `desktopCapturer.getSources` remains functional during
the deprecation period and is the recommended bridge:

```js
const { desktopCapturer, session } = require('electron')

session.defaultSession.setDisplayMediaRequestHandler(async (request, callback) => {
  // Use request.preferredDisplaySurface to narrow the source list when the page
  // expresses a preference ('monitor', 'window', 'browser', or 'none').
  const types = request.preferredDisplaySurface === 'window' ? ['window'] : ['screen', 'window']
  const sources = await desktopCapturer.getSources({ types })

  // Render your own picker UI (e.g. by forwarding `sources` to a renderer) and
  // call back with the chosen source once the user picks one.
  const chosen = sources[0]
  callback({ video: chosen })
})
```
