'use strict'

const internal = process.electronBinding('custom_media_stream')

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
