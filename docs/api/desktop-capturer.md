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

  session.defaultSession.setDisplayMediaRequestHandler((request, callback) => {
    desktopCapturer.getSources({ types: ['screen'] }).then((sources) => {
      // Grant access to the first screen found.
      callback({ video: sources[0], audio: 'loopback' })
    })
    // If true, use the system picker if available.
    // Note: this is currently experimental. If the system picker
    // is available, it will be used and the media request handler
    // will not be invoked.
  }, { useSystemPicker: true })

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

### `desktopCapturer.getSources(options)`

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

> [!NOTE]

> * Capturing audio requires `NSAudioCaptureUsageDescription` Info.plist key on macOS 14.2 Sonoma and higher - [read more](#macos-versions-142-or-higher).
> * Capturing the screen contents requires user consent on macOS 10.15 Catalina or higher, which can detected by [`systemPreferences.getMediaAccessStatus`][].

[`navigator.mediaDevices.getUserMedia`]: https://developer.mozilla.org/en/docs/Web/API/MediaDevices/getUserMedia
[`systemPreferences.getMediaAccessStatus`]: system-preferences.md#systempreferencesgetmediaaccessstatusmediatype-windows-macos

## Caveats

### Linux

`desktopCapturer.getSources(options)` only returns a single source on Linux when using Pipewire.

PipeWire supports a single capture for both screens and windows. If you request the window and screen type, the selected source will be returned as a window capture.

---

### MacOS versions 14.2 or higher

`NSAudioCaptureUsageDescription` Info.plist key must be added in-order for audio to be captured by `desktopCapturer`. If instead you are running electron from another program like a terminal or IDE then that parent program must contain the Info.plist key.

This is in order to facillitate use of Apple's new [CoreAudio Tap API](https://developer.apple.com/documentation/CoreAudio/capturing-system-audio-with-core-audio-taps#Configure-the-sample-code-project) by Chromium.

> [!WARNING]
> Failure of `desktopCapturer` to start an audio stream due to `NSAudioCaptureUsageDescription` permission not present will still create a dead audio stream however no warnings or errors are displayed.

As of electron `v39.0.0-beta.4` Chromium [made Apple's new `CoreAudio Tap API` the default](https://source.chromium.org/chromium/chromium/src/+/ad17e8f8b93d5f34891b06085d373a668918255e) for desktop audio capture. There is no fallback to the older `Screen & System Audio Recording` permissions system even if [CoreAudio Tap API](https://developer.apple.com/documentation/CoreAudio/capturing-system-audio-with-core-audio-taps) stream creation fails.

If you need to continue using `Screen & System Audio Recording` permissions for `desktopCapturer` on macOS versions 14.2 and later, you can apply a chromium feature flag to force use of that older permissions system:

```js
// main.js (right beneath your require/import statments)
app.commandLine.appendSwitch('disable-features', 'MacCatapLoopbackAudioForScreenShare')
```

---

### MacOS versions 12.7.6 or lower

`navigator.mediaDevices.getUserMedia` does not work on macOS versions 12.7.6 and prior for audio capture due to a fundamental limitation whereby apps that want to access the system's audio require a [signed kernel extension](https://developer.apple.com/library/archive/documentation/Security/Conceptual/System_Integrity_Protection_Guide/KernelExtensions/KernelExtensions.html). Chromium, and by extension Electron, does not provide this. Only in macOS 13 and onwards does Apple provide APIs to capture desktop audio without the need for a signed kernel extension.

It is possible to circumvent this limitation by capturing system audio with another macOS app like [BlackHole](https://existential.audio/blackhole/) or [Soundflower](https://rogueamoeba.com/freebies/soundflower/) and passing it through a virtual audio input device. This virtual device can then be queried with `navigator.mediaDevices.getUserMedia`.
