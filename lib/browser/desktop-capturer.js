'use strict'

const ipcMain = require('electron').ipcMain
const desktopCapturer = process.atomBinding('desktop_capturer').desktopCapturer

var deepEqual = function (opt1, opt2) {
  return JSON.stringify(opt1) === JSON.stringify(opt2)
}

// A queue for holding all requests from renderer process.
var requestsQueue = []

ipcMain.on('ELECTRON_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', function (event, captureWindow, captureScreen, thumbnailSize, id) {
  var request
  request = {
    id: id,
    options: {
      captureWindow: captureWindow,
      captureScreen: captureScreen,
      thumbnailSize: thumbnailSize
    },
    webContents: event.sender
  }
  requestsQueue.push(request)
  if (requestsQueue.length === 1) {
    desktopCapturer.startHandling(captureWindow, captureScreen, thumbnailSize)
  }

  // If the WebContents is destroyed before receiving result, just remove the
  // reference from requestsQueue to make the module not send the result to it.
  return event.sender.once('destroyed', function () {
    request.webContents = null
  })
})

desktopCapturer.emit = function (event, name, sources) {
  // Receiving sources result from main process, now send them back to renderer.
  var handledRequest, i, len, ref, ref1, request, result, source, unhandledRequestsQueue
  handledRequest = requestsQueue.shift(0)
  result = (function () {
    var i, len, results
    results = []
    for (i = 0, len = sources.length; i < len; i++) {
      source = sources[i]
      results.push({
        id: source.id,
        name: source.name,
        thumbnail: source.thumbnail.toDataUrl()
      })
    }
    return results
  })()
  if ((ref = handledRequest.webContents) != null) {
    ref.send('ELECTRON_RENDERER_DESKTOP_CAPTURER_RESULT_' + handledRequest.id, result)
  }

  // Check the queue to see whether there is other same request. If has, handle
  // it for reducing redunplicated `desktopCaptuer.startHandling` calls.
  unhandledRequestsQueue = []
  for (i = 0, len = requestsQueue.length; i < len; i++) {
    request = requestsQueue[i]
    if (deepEqual(handledRequest.options, request.options)) {
      if ((ref1 = request.webContents) != null) {
        ref1.send('ELECTRON_RENDERER_DESKTOP_CAPTURER_RESULT_' + request.id, result)
      }
    } else {
      unhandledRequestsQueue.push(request)
    }
  }
  requestsQueue = unhandledRequestsQueue

  // If the requestsQueue is not empty, start a new request handling.
  if (requestsQueue.length > 0) {
    const {captureWindow, captureScreen, thumbnailSize} = requestsQueue[0].options
    return desktopCapturer.startHandling(captureWindow, captureScreen, thumbnailSize)
  }
}
