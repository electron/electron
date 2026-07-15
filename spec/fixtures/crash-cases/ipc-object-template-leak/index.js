// Regression test for https://github.com/electron/electron/issues/52343
//
// frame_converter.cc was creating a new v8::ObjectTemplate on every IPC call
// via Converter<AccessorValue<RenderFrameHost*>>::ToV8(). Over millions of
// calls this accumulated FunctionTemplateInfo objects in V8's old generation,
// causing heap corruption and a crash in InstantiateFunction() during idle GC.
//
// The fix caches the ObjectTemplate using v8::Eternal<v8::ObjectTemplate>.
// This test verifies that many rapid sendSync calls complete without crashing.

const { app, BrowserWindow, ipcMain } = require('electron');

const IPC_COUNT = 10_000;

app.on('ready', async () => {
  const win = new BrowserWindow({
    show: false,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });

  let count = 0;
  ipcMain.on('ping', (event) => {
    count++;
    // Access senderFrame to exercise the AccessorValue<RFH*> converter path
    // that was previously creating a new ObjectTemplate on every call.
    void event.senderFrame;
    event.returnValue = count;
  });

  ipcMain.on('done', () => {
    if (count !== IPC_COUNT) {
      process.exit(1);
    }
    app.quit();
  });

  await win.loadURL(`data:text/html,<script>
    const { ipcRenderer } = require('electron');
    for (let i = 0; i < ${IPC_COUNT}; i++) {
      ipcRenderer.sendSync('ping');
    }
    ipcRenderer.send('done');
  </script>`);
});
