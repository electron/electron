# desktopCapturer

> List `getUserMedia` sources for capturing audio, video, and images from a
microphone, camera, or screen.

```javascript
// In the renderer process.
const {desktopCapturer} = require('electron');

desktopCapturer.getSources({types: ['window', 'screen']}, (error, sources) => {
  if (error) throw error;
  for (let i = 0; i < sources.length; ++i) {
    if (sources[i].name === 'Electron') {
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

When creating a constraints object for the `navigator.webkitGetUserMedia` call,
if you are using a source from `desktopCapturer` your `chromeMediaSource` must
be set to `"desktop"` and your `audio` must be set to `false`.

If you wish to
capture the audio and video from the entire desktop you can set
`chromeMediaSource` to `"screen"` and `audio` to `true`. When using this method
you cannot specify a `chromeMediaSourceId`.

## Methods

The `desktopCapturer` module has the following methods:

### `desktopCapturer.getSources(options, callback)`

* `options` Object
  * `types` Array - An array of String that lists the types of desktop sources
    to be captured, available types are `screen` and `window`.
  * `thumbnailSize` Object (optional) - The suggested size that thumbnail should
    be scaled, it is `{width: 150, height: 150}` by default.
* `callback` Function

Starts a request to get all desktop sources, `callback` will be called with
`callback(error, sources)` when the request is completed.

The `sources` is an array of `Source` objects, each `Source` represents a
captured screen or individual window, and has following properties:

* `id` String - The id of the captured window or screen used in
  `navigator.webkitGetUserMedia`. The format looks like `window:XX` or
  `screen:XX` where `XX` is a random generated number.
* `name` String - The described name of the capturing screen or window. If the
  source is a screen, the name will be `Entire Screen` or `Screen <index>`; if
  it is a window, the name will be the window's title.
* `thumbnail` [NativeImage](native-image.md) - A thumbnail native image.

**Note:** There is no guarantee that the size of `source.thumbnail` is always
the same as the `thumnbailSize` in `options`. It also depends on the scale of
the screen or window.
