# desktop-capturer

The `desktop-capturer` is a renderer module used to capture the content of
screen and individual app windows.

```javascript
// In the renderer process.
var desktopCapturer = require('desktop-capturer');

desktopCapturer.on('source-added', function(id, name) {
  console.log("source " + name + " is added.");
  // navigator.webkitGetUserMedia is not ready for use now.
});

desktopCapturer.on('source-thumbnail-changed', function(id, name, thumbnail) {
  if (name == "Electron") {
    // stopUpdating since we have found the window that we want to capture.
    desktopCapturer.stopUpdating();

    // It's ready to use webkitGetUserMedia right now.
    navigator.webkitGetUserMedia({
      audio: false,
      video: {
        mandatory: {
          chromeMediaSource: 'desktop',
          chromeMediaSourceId: id,
          minWidth: 1280,
          maxWidth: 1280,
          minHeight: 720,
          maxHeight: 720
        }
      }
    }, gotStream, getUserMediaError);
  }
});

// Let's start updating after setting all event of `desktopCapturer`
desktopCapturer.startUpdating();

function gotStream(stream) {
  document.querySelector('video').src = URL.createObjectURL(stream);
}

function getUserMediaError(e) {
  console.log('getUserMediaError');
}
```

## Events

### Event: 'source-added'

* `id` String - The id of the captured window or screen used in
  `navigator.webkitGetUserMedia`. The format looks like 'window:XX' or
  'screen:XX' where XX is a random generated number.
* `name` String - The descriped name of the capturing screen or window. If the
  source is a screen, the name will be 'Entire Screen' or 'Screen <index>'; if
  it is a window, the name will be the window's title.

Emits when there is a new source added, usually a new window is created or a new
screen is attached.

**Note:** `navigator.webkitGetUserMedia` is not ready for use in this event.

### Event: 'source-removed'

* `id` String - The id of the captured window or screen used in
  `navigator.webkitGetUserMedia`.
* `name` String - The descriped name of the capturing screen or window.

Emits when there is a source removed.

### Event: 'source-name-changed'

* `id` String - The id of the captured window or screen used in
  `navigator.webkitGetUserMedia`.
* `name` String - The descriped name of the capturing screen or window.

Emits when the name of source is changed.

### Event: 'source-thumbnail-changed'

* `id` String - The id of the captured window or screen used in
  `navigator.webkitGetUserMedia`.
* `name` String - The descriped name of the capturing screen or window.
* `thumbnail` [NativeImage](NativeImage.md) - A thumbnail image.

Emits when the thumbnail of source is changed. `desktopCapturer` will refresh
all sources every second.

### Event: 'refresh-finished'

Emits when `desktopCapturer` finishes a refresh.

## Methods

The `desktopCapturer` module has the following methods:

### `desktopCapturer.startUpdating(options)`

`options` Object, properties:

* `types` Array - An array of String that enums the types of desktop sources.
  * `screen` String - Screen
  * `window` String - Individual window
* `updatePeriod` Integer (optional) - The update period in milliseconds. By
  default, `desktopCapturer` updates every second.
* `thumbnailWidth` Integer (optional) - The width of thumbnail. By default, it
  is 150px.
* `thumbnailHeight` Integer (optional) - The height of thumbnail. By default, it
  is 150px.

Starts updating desktopCapturer. The events of `desktopCapturer` will only be
emitted after `startUpdating` API is invoked.

**Note:** At beginning, the desktopCapturer is initially empty, so the
`source-added` event will be emitted for each existing source as it is
enumrated.
On Windows, you will see the screen ficker when desktopCapturer starts updating.
This is normal because the desktop effects(e.g. Aero) will be disabled when
desktop capturer is working. The ficker will disappear once
`desktopCapturer.stopUpdating()` is invoked.

### `desktopCapturer.stopUpdating()`

Stops updating desktopCapturer. The events of `desktopCapturer` will not be
emitted after the API is invoked.

**Note:** It is a good practice to call `stopUpdating` when you do not need
getting any infomation of sources, usually after passing a id to
`navigator.webkitGetUserMedia`.
