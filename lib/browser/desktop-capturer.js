'use strict'

const ipcMainUtils = require('@electron/internal/browser/ipc-main-internal-utils')

const { desktopCapturer } = process.electronBinding('desktop_capturer')
const eventBinding = process.electronBinding('event')

const deepEqual = (a, b) => JSON.stringify(a) === JSON.stringify(b)

// A queue for holding all requests from renderer process.
let requestsQueue = []

ipcMainUtils.handle('ELECTRON_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', (event, captureWindow, captureScreen, thumbnailSize, fetchWindowIcons) => {
  const customEvent = eventBinding.createWithSender(event.sender)
  event.sender.emit('desktop-capturer-get-sources', customEvent)

  if (customEvent.defaultPrevented) {
    return []
  }

  return new Promise((resolve, reject) => {
    const request = {
      options: {
        captureWindow,
        captureScreen,
        thumbnailSize,
        fetchWindowIcons
      },
      resolve
    }
    requestsQueue.push(request)
    if (requestsQueue.length === 1) {
      desktopCapturer.startHandling(captureWindow, captureScreen, thumbnailSize, fetchWindowIcons)
    }

    // If the WebContents is destroyed before receiving result, just remove the
    // reference from requestsQueue to make the module not send the result to it.
    event.sender.once('destroyed', () => {
      request.resolve = null
    })
  })
})

desktopCapturer.emit = (event, name, sources, fetchWindowIcons) => {
  // Receiving sources result from main process, now send them back to renderer.
  const handledRequest = requestsQueue.shift()
  const unhandledRequestsQueue = []

  const result = sources.map(source => {
    return {
      id: source.id,
      name: source.name,
      thumbnail: source.thumbnail.toDataURL(),
      display_id: source.display_id,
      appIcon: (fetchWindowIcons && source.appIcon) ? source.appIcon.toDataURL() : null
    }
  })

  if (handledRequest.resolve) {
    handledRequest.resolve(result)
  }

  // Check the queue to see whether there is another identical request & handle
  requestsQueue.forEach(request => {
    if (deepEqual(handledRequest.options, request.options)) {
      if (request.resolve) {
        request.resolve(result)
      }
    } else {
      unhandledRequestsQueue.push(request)
    }
  })
  requestsQueue = unhandledRequestsQueue

  // If the requestsQueue is not empty, start a new request handling.
  if (requestsQueue.length > 0) {
    const { captureWindow, captureScreen, thumbnailSize, fetchWindowIcons } = requestsQueue[0].options
    return desktopCapturer.startHandling(captureWindow, captureScreen, thumbnailSize, fetchWindowIcons)
  }
}
