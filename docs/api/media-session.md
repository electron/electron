## Class: MediaSession

> Access to media metadata and action handlers.

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

The [W3C Media Session API](https://www.w3.org/TR/mediasession/)
provides a way for web apps to communicate to the user agent about metadata
related to media actively playing in the page. It also provides action handlers
which the user agent can use to control media playback.

Using Electron's `MediaSession` API—available via
[`WebContents`](web-contents.md)—an application could listen for metadata
changes and display a custom UI for playback.

```javascript
const { BrowserWindow } = require('electron')

const win = new BrowserWindow()
win.loadURL('https://example.com/videos/123')

win.webContents.mediaSession.on(
  'actions-changed',
  (event, details) => {
    // Determine media controls visibility based on available actions.
    if (details.actions.includes('play')) {
      showPlayButtonUI()
    } else {
      hidePlayButtonUI()
    }
  }
)

let playbackState

win.webContents.mediaSession.on(
  'info-changed',
  (event, details) => {
    // Save last known playback state.
    playbackState = details.playbackState

    updatePlaybackUI(playbackState)
  }
)

function togglePlayback () {
  if (playbackState === 'playing') {
    win.webContents.mediaSession.pause()
  } else {
    win.webContents.mediaSession.play()
  }
}
```

### Instance Events

#### Event: 'actions-changed'

Returns:

* `event` Event
* `details` Object
  * `actions` string[] - An array of media session actions, can
  be `play`, `pause`, `previous-track`, `next-track`, `seek-backward`, `seek-forward`, `skip-ad`, `stop`, `seek-to`, `scrub-to`, `enter-picture-in-picture`, `exit-picture-in-picture`, `switch-audio-device`, `toggle-microphone`, `toggle-camera`, `hang-up`, `raise` or `set-mute`.

Emitted when media session action list has changed. This list contains the
available actions which can be used to control the session.

#### Event: 'info-changed'

Returns:

* `event` Event
* `details` Object
  * `playbackState` string - Can be `playing` or `paused`.

Emitted when the info associated with the session changed.

### Instance Methods

#### `mediaSession.play()`

Resumes the media session.

#### `mediaSession.pause()`

Pauses the media session.

#### `mediaSession.stop()`

Stops the media session.
