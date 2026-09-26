// A CDP message with a non-string value for an optional string param must be
// rejected with a protocol error rather than abort the browser process (Node's
// crdtp::ProtocolTypeTraits<std::string> CHECKs where Chromium's reports an error).
const { app, BrowserWindow } = require('electron');

app.whenReady().then(async () => {
  const w = new BrowserWindow({ show: false });
  await w.loadURL('about:blank');
  w.webContents.debugger.attach('1.3');
  try {
    await w.webContents.debugger.sendCommand('Input.dispatchKeyEvent', {
      type: 'keyDown',
      text: 1
    });
  } catch {
    // Rejecting is the expected outcome.
  }
  app.quit();
});
