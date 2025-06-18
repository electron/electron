# desktopCapturer

> Access information about media sources that can be used to capture audio and
> video from the desktop using the [`navigator.mediaDevices.getUserMedia`][] API.

Process: [Main](../glossary.md#main-process)

The following example shows how to capture video and system audio of the first screen found:

```js
// main.js
const { app, BrowserWindow, desktopCapturer, session } = require('electron')

app.whenReady().then(() => {
  const mainWindow = new BrowserWindow()

  session.defaultSession.setDisplayMediaRequestHandler((request, callback) => {
    desktopCapturer.getSources({ types: ['screen'] }).then((sources) => {
      // Grant access to the first screen found.
      // Capture system audio if supported.
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
    <video width="320" height="240" autoplay controls></video>
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
> Capturing the screen contents requires user consent on macOS 10.15 Catalina or higher,
> which can detected by [`systemPreferences.getMediaAccessStatus`][].

[`navigator.mediaDevices.getUserMedia`]: https://developer.mozilla.org/en/docs/Web/API/MediaDevices/getUserMedia
[`systemPreferences.getMediaAccessStatus`]: system-preferences.md#systempreferencesgetmediaaccessstatusmediatype-windows-macos

## System Audio Capture (macOS 12.3+)

 On macOS 12.3+, `navigator.mediaDevices.getDisplayMedia` can be used to capture system audio
 if the [`setDisplayMediaRequestHandler`](./session.md#sessetdisplaymediarequesthandlerhandler-opts) `callback` 
 is passed a `streams` object that has `audio: 'loopback'`. See the example above.

> [!NOTE]
> Due to ongoing changes in Chromium, capturing system audio is experimental on macOS 15+ and requires the 
> following [command line switch](./command-line-switches.md) to be appended. Please note that you may experience bugs.
>
> ```js
> // main.js
> app.commandLine.appendSwitch('enable-features', 'MacSckSystemAudioLoopbackOverride');
> ```

## System Audio Capture (before macOS 12.3)

On macOS 12.3 or lower, system audio capture is not supported due to a fundamental limitation whereby 
apps that want to access the system's audio require a [signed kernel extension](https://developer.apple.com/library/archive/documentation/Security/ConceptualSystem_Integrity_Protection_Guide/KernelExtensions/KernelExtensions.html). 
Chromium, and by extension Electron, does not provide this.

It is possible to circumvent this limitation by capturing system audio with another macOS app like 
Soundflower and passing it through a virtual audio input device. This virtual device can then be 
queried with `navigator.mediaDevices.getUserMedia`.
