# desktop-capturer

The `desktop-capturer` is a renderer module used to capture the content of
screen and individual app windows.

```javascript
// In the renderer process.
var desktopCapturer = require('desktop-capturer');

desktopCapturer.getSources({types: ['window', 'screen']}, function(sources) {
  for (var i = 0; i < sources.length; ++i) {
    if (sources[i].name == "Electron") {
      navigator.webkitGetUserMedia({
        audio: false,
        video: {
          mandatory: {
            chromeMediaSource: 'desktop',
            chromeMediaSourceId: sources[i].id,
            minWidth: 1280,
            maxWidth: 1280,
            minHeight: 720,
            maxHeight: 720
          }
        }
      }, gotStream, getUserMediaError);
      return;
    }
  }
});

function gotStream(stream) {
  document.querySelector('video').src = URL.createObjectURL(stream);
}

function getUserMediaError(e) {
  console.log('getUserMediaError');
}
```

## Methods

The `desktopCapturer` module has the following methods:

### `desktopCapturer.getSources(options, callback)`

`options` Object, properties:
* `types` Array - An array of String that enums the types of desktop sources.
  * `screen` String - Screen
  * `window` String - Individual window
* `thumbnailSize` Object (optional) - The suggested size that thumbnail should
  be scaled.
  * `width` Integer - The width of thumbnail. By default, it is 150px.
  * `height` Integer - The height of thumbnail. By default, it is 150px.

`callback` Function - `function(sources) {}`

* `Sources` Array - An array of Source

Gets all desktop sources.

**Note:** There is no garuantee that the size of `source.thumbnail` is always
the same as the `thumnbailSize` in `options`. It also depends on the scale of
the screen or window.

## Source

`Source` is an object represents a captured screen or individual window. It has
following properties:

* `id` String - The id of the captured window or screen used in
  `navigator.webkitGetUserMedia`. The format looks like 'window:XX' or
  'screen:XX' where XX is a random generated number.
* `name` String - The descriped name of the capturing screen or window. If the
  source is a screen, the name will be 'Entire Screen' or 'Screen <index>'; if
  it is a window, the name will be the window's title.
* `thumbnail` [NativeImage](NativeImage.md) - A thumbnail image.
