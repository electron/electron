const ipcMain = require('electron').ipcMain;
const desktopCapturer = process.atomBinding('desktop_capturer').desktopCapturer;

var deepEqual = function(opt1, opt2) {
  return JSON.stringify(opt1) === JSON.stringify(opt2);
};

// A queue for holding all requests from renderer process.
var requestsQueue = [];

ipcMain.on('ATOM_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', function(event, captureWindow, captureScreen, thumbnailSize, id) {
  const request = {
    id: id,
    options: {
      captureWindow: captureWindow,
      captureScreen: captureScreen,
      thumbnailSize: thumbnailSize
    },
    webContents: event.sender
  };
  requestsQueue.push(request);
  if (requestsQueue.length === 1) {
    desktopCapturer.startHandling(captureWindow, captureScreen, thumbnailSize);
  }

  // If the WebContents is destroyed before receiving result, just remove the
  // reference from requestsQueue to make the module not send the result to it.
  return event.sender.once('destroyed', function() {
    return request.webContents = null;
  });
});

desktopCapturer.emit = function(event, name, sources) {
  // Receiving sources result from main process, now send them back to renderer.
  const handledRequest = requestsQueue.shift(0);
  const result = sources.map(function(source) {
    return {
      id: source.id,
      name: source.name,
      thumbnail: source.thumbnail.toDataUrl()
    };
  });

  if (handledRequest.webContents != null) {
    handledRequest.webContents.send("ATOM_RENDERER_DESKTOP_CAPTURER_RESULT_" + handledRequest.id, result);
  }

  // Check the queue to see whether there is other same request. If has, handle
  // it for reducing reduplicated `desktopCaptuer.startHandling` calls.
  requestsQueue = requestsQueue.filter(function(request) {
    if (deepEqual(handledRequest.options, request.options)) {
      if (request.webContents != null) {
        request.webContents.send("ATOM_RENDERER_DESKTOP_CAPTURER_RESULT_" + request.id, result);
      }
      return false;
    } else {
      return true;
    }
  });

  // If the requestsQueue is not empty, start a new request handling.
  if (requestsQueue.length > 0) {
    const options = requestsQueue[0].options;
    return desktopCapturer.startHandling(options.captureWindow, options.captureScreen, options.thumbnailSize);
  }
};
