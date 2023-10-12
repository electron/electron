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
const mediaSession = win.webContents.mediaSession

mediaSession.on(
  'actions-changed',
  () => {
    // Determine media controls visibility based on available actions.
    if (mediaSession.actions.includes('play')) {
      // Show play button UI...
    } else {
      // Show hide button UI...
    }
  }
)

mediaSession.on('info-changed', () => {
  const playbackState = mediaSession.info.playbackState
  // Update UI with playback state...
})

function togglePlayback () {
  if (mediaSession.info.playbackState === 'playing') {
    mediaSession.pause()
  } else {
    mediaSession.play()
  }
}
```

### Instance Events

#### Event: 'actions-changed'

Returns:

* `event` Event
* `details` Object
  * `actions` string[] - An array of media session actions, can
  be `play`, `pause`, `previous-track`, `next-track`, `seek-backward`, `seek-forward`, `skip-ad`, `stop`, `seek-to`, `scrub-to`, `enter-picture-in-picture`, `exit-picture-in-picture`, `switch-audio-device`, `toggle-microphone`, `toggle-camera`, `hang-up`, `raise`, `set-mute`, `previous-slide`, `next-slide` or `enter-auto-picture-in-picture`.

Emitted when media session action list has changed. This list contains the
available actions which can be used to control the session.

#### Event: 'info-changed'

Returns:

* `event` Event
* `details` Object
  * `playbackState` string - Can be `playing` or `paused`.
  * `muted` boolean - Whether the media player is muted.

Emitted when the info associated with the session changed.

#### Event: 'metadata-changed'

Returns:

* `event` Event

Emitted when the metadata associated with the session changed.

#### Event: 'images-changed'

Returns:

* `event` Event

Emitted when the images associated with the session changed.

#### Event: 'position-changed'

Returns:

* `event` Event

Emitted when the position associated with the session changed.

### Instance Methods

#### `mediaSession.play()`

Resumes the media session.

#### `mediaSession.pause()`

Pauses the media session.

#### `mediaSession.stop()`

Stops the media session.

### Instance Properties

#### `mediaSession.actions`

A `string[]` array of media session actions, can be `play`, `pause`, `previous-track`, `next-track`, `seek-backward`, `seek-forward`, `skip-ad`, `stop`, `seek-to`, `scrub-to`, `enter-picture-in-picture`, `exit-picture-in-picture`, `switch-audio-device`, `toggle-microphone`, `toggle-camera`, `hang-up`, `raise`, `set-mute`, `previous-slide`, `next-slide` or `enter-auto-picture-in-picture`.

#### `mediaSession.info`

An `any` respresenting the media session's info.
<!-- TODO: document structure -->

#### `mediaSession.metadata`

An `any` respresenting the media session's metadata.
<!-- TODO: document structure -->

#### `mediaSession.position`

An `any` respresenting the media session's position.
<!-- TODO: document structure -->

#### `mediaSession.images`

An `any` respresenting the media session's images.
<!-- TODO: document structure -->
