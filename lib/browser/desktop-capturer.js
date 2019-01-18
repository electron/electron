'use strict'

const ipcMain = require('@electron/internal/browser/ipc-main-internal')

const { desktopCapturer } = process.atomBinding('desktop_capturer')
const eventBinding = process.atomBinding('event')

const deepEqual = (a, b) => JSON.stringify(a) === JSON.stringify(b)

// A queue for holding all requests from renderer process.
let requestsQueue = []

const electronSources = 'ELECTRON_BROWSER_DESKTOP_CAPTURER_GET_SOURCES'
const capturerResult = (id) => `ELECTRON_RENDERER_DESKTOP_CAPTURER_RESULT_${id}`

ipcMain.on(electronSources, (event, captureWindow, captureScreen, thumbnailSize, fetchWindowIcons, id) => {
  const customEvent = eventBinding.createWithSender(event.sender)
  event.sender.emit('desktop-capturer-get-sources', customEvent)

  if (customEvent.defaultPrevented) {
    event._replyInternal(capturerResult(id), [])
    return
  }

  const request = {
    id,
    options: {
      captureWindow,
      captureScreen,
      thumbnailSize,
      fetchWindowIcons
    },
    event
  }
  requestsQueue.push(request)
  if (requestsQueue.length === 1) {
    desktopCapturer.startHandling(captureWindow, captureScreen, thumbnailSize, fetchWindowIcons)
  }

  // If the WebContents is destroyed before receiving result, just remove the
  // reference from requestsQueue to make the module not send the result to it.
  event.sender.once('destroyed', () => {
    request.event = null
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

  if (handledRequest.event) {
    handledRequest.event._replyInternal(capturerResult(handledRequest.id), result)
  }

  // Check the queue to see whether there is another identical request & handle
  requestsQueue.forEach(request => {
    const event = request.event
    if (deepEqual(handledRequest.options, request.options)) {
      if (event) {
        event._replyInternal(capturerResult(request.id), result)
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
