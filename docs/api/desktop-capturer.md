# desktopCapturer

> Access information about media sources that can be used to capture audio and
> video from the desktop using the [`navigator.mediaDevices.getUserMedia`] API.

Process: [Renderer](../glossary.md#renderer-process)

The following example shows how to capture video from a desktop window whose
title is `Electron`:

```javascript
// In the renderer process.
const {desktopCapturer} = require('electron')

desktopCapturer.getSources({types: ['window', 'screen']}, (error, sources) => {
  if (error) throw error
  for (let i = 0; i < sources.length; ++i) {
    if (sources[i].name === 'Electron') {
      navigator.mediaDevices.getUserMedia({
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
      })
      .then((stream) => handleStream(stream))
      .catch((e) => handleError(e))
      return
    }
  }
})

function handleStream (stream) {
  const video = document.querySelector('video')
  video.srcObject = stream
  video.onloadedmetadata = (e) => video.play()
}

function handleError (e) {
  console.log(e)
}
```

To capture video from a source provided by `desktopCapturer` the constraints
passed to [`navigator.mediaDevices.getUserMedia`] must include
`chromeMediaSource: 'desktop'`, and `audio: false`.

To capture both audio and video from the entire desktop the constraints passed
to [`navigator.mediaDevices.getUserMedia`] must include `chromeMediaSource: 'desktop'`,
for both `audio` and `video`, but should not include a `chromeMediaSourceId` constraint.

```javascript
const constraints = {
  audio: {
    mandatory: {
      chromeMediaSource: 'desktop'
    }
  },
  video: {
    mandatory: {
      chromeMediaSource: 'desktop'
    }
  }
}
```

## Methods

The `desktopCapturer` module has the following methods:

### `desktopCapturer.getSources(options, callback)`

* `options` Object
  * `types` String[] - An array of Strings that lists the types of desktop sources
    to be captured, available types are `screen` and `window`.
  * `thumbnailSize` [Size](structures/size.md) (optional) - The size that the media source thumbnail
    should be scaled to. Default is `150` x `150`.
* `callback` Function
  * `error` Error
  * `sources` [DesktopCapturerSource[]](structures/desktop-capturer-source.md)

Starts gathering information about all available desktop media sources,
and calls `callback(error, sources)` when finished.

`sources` is an array of [`DesktopCapturerSource`](structures/desktop-capturer-source.md)
objects, each `DesktopCapturerSource` represents a screen or an individual window that can be
captured.

[`navigator.mediaDevices.getUserMedia`]: https://developer.mozilla.org/en/docs/Web/API/MediaDevices/getUserMedia
