# desktop-capturer

The `desktop-capturer` is a renderer module used to capture the content of
screen and individual app windows.

```javascript
// In the renderer process.
var desktopCapturer = require('desktop-capturer');

desktopCapturer.on('source-added', function(source) {
  console.log("source " + source.name + " is added.");
  // source.thumbnail is not ready to use here and webkitGetUserMedia neither.
});

desktopCapturer.on('source-thumbnail-changed', function(source) {
  if (source.name == "Electron") {
    // stopUpdating since we have found the window that we want to capture.
    desktopCapturer.stopUpdating();

    // It's ready to use webkitGetUserMedia right now.
    navigator.webkitGetUserMedia({
      audio: false,
      video: {
        mandatory: {
          chromeMediaSource: 'desktop',
          chromeMediaSourceId: source.id,
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

* `source` Source

Emits when there is a new source added, usually a new window is created,
a new screen is attached.

**Note:** The thumbnail of the source is not ready for use when 'source-added'
event is emitted, and `navigator.webkitGetUserMedia` neither.

### Event: 'source-removed'

* `source` Source

Emits when there is a source removed.

### Event: 'source-name-changed'

* `source` Source

Emits when the name of source is changed.

### Event: 'source-thumbnail-changed'

* `source` Source

Emits when the thumbnail of source is changed. `desktopCapturer` will refresh
all sources every second.

## Methods

The `desktopCapturer` module has the following methods:

### `desktopCapturer.startUpdating(options)`

* `options` Array - An array of String that enums the types of desktop sources.
  * `screen` String - Screen
  * `window` String - Individual window

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
getting any infomation of sources, usually after passing a id of source to
`navigator.webkitGetUserMedia`.

## Source

`Source` is an object represents a captured screen or individual window. It has
following properties:

* `id` String - The id of the capturing window or screen used in
  `navigator.webkitGetUserMedia`.
* `name` String - The descriped name of the capturing screen or window.
* `thumbnail` [NativeImage](NativeImage.md) - A thumbnail image.
