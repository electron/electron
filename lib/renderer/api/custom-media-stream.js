'use strict'

const internal = process.electronBinding('custom_media_stream')

// This function wraps call to native createTrack function
// in a way that allows user to avoid passing time-related arguments
// to allocateFrame and queueFrame functions
// TODO: Rename or refactor info to get rid of "info.info"
// TODO: make code more readable
exports.createTrack = (info) => {
  return internal.createTrack(info.info.resolution, info.info.frameRate, (control) => {
    const allocate = control.allocateFrame
    const queue = control.queueFrame

    const resolution = { width: info.info.resolution.width, height: info.info.resolution.height }
    const epoch = Date.now()

    control.allocateFrame = (format, timestamp) => {
      format = format || resolution
      timestamp = timestamp || (Date.now() - epoch)

      return allocate.call(control, format, timestamp)
    }

    control.queueFrame = (frame, estimatedCaptureTime) => {
      estimatedCaptureTime = estimatedCaptureTime || Date.now()

      queue.call(control, frame, estimatedCaptureTime)
    }

    info.startCapture(control)
  },
  () => {
    info.stopCapture()
  })
}
