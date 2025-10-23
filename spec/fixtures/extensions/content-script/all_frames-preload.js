const { ipcRenderer, webFrame } = require('electron');

if (process.isMainFrame) {
  // https://github.com/electron/electron/issues/17252
  ipcRenderer.on('executeJavaScriptInFrame', (event, frameToken, code, responseId) => {
    const frame = webFrame.findFrameByToken(frameToken);
    if (!frame) {
      throw new Error(`Can't find frame for frame token ${frameToken}`);
    }
    frame.executeJavaScript(code, false).then(result => {
      event.sender.send(`executeJavaScriptInFrame_${responseId}`, result);
    });
  });
}
