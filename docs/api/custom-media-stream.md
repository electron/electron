# customMediaStream

> allows to create controlled [MediaStreamTrack](https://developer.mozilla.org/en-US/docs/Web/API/MediaStreamTrack) that can be attached to media stream via `MediaStream.addTrack()`. Custom media track can be used to convert bitmaps sequence to [MediaStream](https://developer.mozilla.org/en-US/docs/Web/API/MediaStream) for `<video>` element or transfer with `WebRTC`

Process: [Renderer](../glossary.md#renderer-process)

The following example shows how to create a media track, attach to a media stream and put it to the `<video>` element. Example stream is filled with green frames with varying suturation

```javascript
// In the renderer process.
const { customMediaStream } = require('electron')

let track = customMediaStream.createTrack({
  info: {
    resolution: { width: 1920, height: 1080 },
    frameRate: 30
  },
  startCapture: (controller) => {
    let i = 0
    this.interval = setInterval(() => {
      let frame = controller.allocateFrame()

      // frame.y, frame.u and frame.v are ArrayBuffers
      // containing pixels from each color plane
      new Uint8Array(frame.y).fill(i++ % 100) // filling the buffer with color green
      new Uint8Array(frame.u).fill(30)
      new Uint8Array(frame.v).fill(100)

      controller.queueFrame(frame, Date.now())
    }, 1000)
  },
  stopCapture: () => {
    clearInterval(this.interval)
  }
})

let mediaStream = new MediaStream()
mediaStream.addTrack(track)

// attach to HTML5 <video> element
const videoElem = document.getElementById('video-element')
videoElem.srcObject = mediaStream
videoElem.play()
```



## Class: CustomMediaStream
### Static Methods

### `customMediaStream.createTrack(sourceParams)`

* `sourceParams` Object
    * `info` Object
        * `resolution` [Size](structures/size.md) - The video frame size
        * `frameRate` Number - Frames per second parameter of resulting MediaStreamTrack
    * `startCapture` Function - A callback that is called once the customTrack is instantiated and ready for frames. Recieves a `CustomMediaStreamController` instance as a single parameter for video track contents manipulation
    * `stopCapture` Function

## Class: CustomMediaStreamController
There is an internal frame buffer which allows to avoid extra frame objects allocation. The `CustomMediaStreamController` instance wrapps the buffer and provides the following interface

### Instance methods
### `controller.allocateFrame()`
Returns `CustomMediaStreamVideoFrame` instance, which can be modified and put to the custom video track via `controller.queueFrame` method
### `controller.queueFrame(frame, captureTime)`
* `frame` CustomMediaStreamVideoFrame - frame to put into the custom video track (obtained from `controller.allocateFrame` method)
* `captureTime` Number - timestamp [ms] of frame capture

## Class: CustomMediaStreamVideoFrame
### Instance properties
#### `frame.width`
A `Integer` representing frame width
#### `frame.height`
A `Integer` representing frame height
#### `frame.timestamp`
A `Integer` representing the frame capture time (timestamp [ms]). Affects frames presentation sequence - an implementation will not not present the frame before this point-in-time
##### Pixel format fields
The only available for now pixel coding format is [I420](http://www.fourcc.org/pixel-format/yuv-i420/) (12bpp YUV planar 1x1 Y, 2x2 UV samples) since it is one of the most popular ones in video applications. Each plane pixels data is represented by
#### `frame.y`
A `Uint8Array` representing luminance color component
#### `frame.u`
A `Uint8Array` representing blue chrominance projection
#### `frame.v`
A `Uint8Array` representing red chrominance projection
