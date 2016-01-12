{ipcMain} = require 'electron'
{desktopCapturer} = process.atomBinding 'desktop_capturer'

deepEqual = (opt1, opt2) ->
  return JSON.stringify(opt1) is JSON.stringify(opt2)

### A queue for holding all requests from renderer process. ###
requestsQueue = []

ipcMain.on 'ATOM_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', (event, captureWindow, captureScreen, thumbnailSize, id) ->
  request = id: id, options: {captureWindow, captureScreen, thumbnailSize}, webContents: event.sender
  requestsQueue.push request
  desktopCapturer.startHandling captureWindow, captureScreen, thumbnailSize if requestsQueue.length is 1
  ###
    If the WebContents is destroyed before receiving result, just remove the
    reference from requestsQueue to make the module not send the result to it.
  ###
  event.sender.once 'destroyed', ->
    request.webContents = null

desktopCapturer.emit = (event, name, sources) ->
  ### Receiving sources result from main process, now send them back to renderer. ###
  handledRequest = requestsQueue.shift 0
  result = ({ id: source.id, name: source.name, thumbnail: source.thumbnail.toDataUrl() } for source in sources)
  handledRequest.webContents?.send "ATOM_RENDERER_DESKTOP_CAPTURER_RESULT_#{handledRequest.id}", result

  ###
    Check the queue to see whether there is other same request. If has, handle
    it for reducing redunplicated `desktopCaptuer.startHandling` calls.
  ###
  unhandledRequestsQueue = []
  for request in requestsQueue
    if deepEqual handledRequest.options, request.options
      request.webContents?.send "ATOM_RENDERER_DESKTOP_CAPTURER_RESULT_#{request.id}", errorMessage, result
    else
      unhandledRequestsQueue.push request
  requestsQueue = unhandledRequestsQueue
  ### If the requestsQueue is not empty, start a new request handling. ###
  if requestsQueue.length > 0
    {captureWindow, captureScreen, thumbnailSize} = requestsQueue[0].options
    desktopCapturer.startHandling captureWindow, captureScreen, thumbnailSize
