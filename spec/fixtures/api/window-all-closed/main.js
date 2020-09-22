const { app, BrowserWindow } = require('electron');

let handled = false;

if (app.commandLine.hasSwitch('handle-event')) {
  app.on('window-all-closed', () => {
    handled = true;
    app.quit();
  });
}

app.on('quit', () => {
  process.stdout.write(JSON.stringify(handled));
  process.stdout.end();
});

app.whenReady().then(() => {
  const win = new BrowserWindow({
    webPreferences: {
      contextIsolation: true
    }
  });
  win.close();
});
