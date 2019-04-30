'use strict'

const ipcMainUtils = require('@electron/internal/browser/ipc-main-internal-utils')

const { createDesktopCapturer } = process.electronBinding('desktop_capturer')
const eventBinding = process.electronBinding('event')

const deepEqual = (a, b) => JSON.stringify(a) === JSON.stringify(b)

let currentlyRunning = []

ipcMainUtils.handle('ELECTRON_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', (event, captureWindow, captureScreen, thumbnailSize, fetchWindowIcons) => {
  const customEvent = eventBinding.createWithSender(event.sender)
  event.sender.emit('desktop-capturer-get-sources', customEvent)

  if (customEvent.defaultPrevented) {
    return []
  }

  const options = {
    captureWindow,
    captureScreen,
    thumbnailSize,
    fetchWindowIcons
  }

  for (const running of currentlyRunning) {
    if (deepEqual(running.options, options)) {
      // If a request is currently running for the same options
      // return that promise
      return running.getSources
    }
  }

  const getSources = new Promise((resolve, reject) => {
    const stopRunning = () => {
      // Remove from currentlyRunning once we resolve or reject
      currentlyRunning = currentlyRunning.filter(running => running.options !== options)
    }
    const request = {
      options,
      resolve: (value) => {
        stopRunning()
        resolve(value)
      },
      reject: (err) => {
        stopRunning()
        reject(err)
      },
      capturer: createDesktopCapturer()
    }
    request.capturer.emit = createCapturerEmitHandler(request.capturer, request)
    request.capturer.startHandling(captureWindow, captureScreen, thumbnailSize, fetchWindowIcons)

    // If the WebContents is destroyed before receiving result, just remove the
    // reference to resolve, emit and the capturer itself so that it never dispatches
    // back to the renderer
    event.sender.once('destroyed', () => {
      request.resolve = null
      delete request.capturer.emit
      delete request.capturer
      stopRunning()
    })
  })

  currentlyRunning.push({
    options,
    getSources
  })

  return getSources
})

const createCapturerEmitHandler = (capturer, request) => {
  return function handlEmitOnCapturer (event, name, sources, fetchWindowIcons) {
    // Ensure that this capturer instance can only ever receive a single event
    // if we get more than one it is a bug but will also cause strange behavior
    // if we still try to handle it
    delete capturer.emit

    if (name === 'error') {
      const error = sources
      request.reject(error)
      return
    }

    const result = sources.map(source => {
      return {
        id: source.id,
        name: source.name,
        thumbnail: source.thumbnail.toDataURL(),
        display_id: source.display_id,
        appIcon: (fetchWindowIcons && source.appIcon) ? source.appIcon.toDataURL() : null
      }
    })

    if (request.resolve) {
      request.resolve(result)
    }
  }
}
