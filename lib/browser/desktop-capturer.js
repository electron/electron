'use strict'

const {ipcMain} = require('electron')
const {desktopCapturer} = process.atomBinding('desktop_capturer')

const deepEqual = (a, b) => JSON.stringify(a) === JSON.stringify(b)

// A queue for holding all requests from renderer process.
let requestsQueue = []

const electronSources = 'ELECTRON_BROWSER_DESKTOP_CAPTURER_GET_SOURCES'
const capturerResult = (id) => `ELECTRON_RENDERER_DESKTOP_CAPTURER_RESULT_${id}`

ipcMain.on(electronSources, (event, captureWindow, captureScreen, thumbnailSize, id) => {
  const request = {
    id,
    options: {
      captureWindow,
      captureScreen,
      thumbnailSize
    },
    webContents: event.sender
  }
  requestsQueue.push(request)
  if (requestsQueue.length === 1) {
    desktopCapturer.startHandling(captureWindow, captureScreen, thumbnailSize)
  }

  // If the WebContents is destroyed before receiving result, just remove the
  // reference from requestsQueue to make the module not send the result to it.
  event.sender.once('destroyed', () => {
    request.webContents = null
  })
})

desktopCapturer.emit = (event, name, sources) => {
  // Receiving sources result from main process, now send them back to renderer.
  const handledRequest = requestsQueue.shift()
  const handledWebContents = handledRequest.webContents
  const unhandledRequestsQueue = []

  const result = sources.map(source => {
    return {
      id: source.id,
      name: source.name,
      thumbnail: source.thumbnail.toDataURL(),
      display_id: source.display_id
    }
  })

  if (handledWebContents) {
    handledWebContents.send(capturerResult(handledRequest.id), result)
  }

  // Check the queue to see whether there is another identical request & handle
  requestsQueue.forEach(request => {
    const webContents = request.webContents
    if (deepEqual(handledRequest.options, request.options)) {
      if (webContents) webContents.send(capturerResult(request.id), result)
    } else {
      unhandledRequestsQueue.push(request)
    }
  })
  requestsQueue = unhandledRequestsQueue

  // If the requestsQueue is not empty, start a new request handling.
  if (requestsQueue.length > 0) {
    const {captureWindow, captureScreen, thumbnailSize} = requestsQueue[0].options
    return desktopCapturer.startHandling(captureWindow, captureScreen, thumbnailSize)
  }
}
