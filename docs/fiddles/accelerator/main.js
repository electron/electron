// Adding a global shortcut to electron
//
// For more info, see:
// https://electronjs.org/docs/api/screen

const { app, globalShortcut, BrowserWindow } = require('electron')

let mainWindow = null;

app.on('ready', () => {
  // Register a 'CommandOrControl+Y' shortcut listener.
  globalShortcut.register('CommandOrControl+Y', () => {
    // Do stuff when Y and either Command/Control is pressed.
    console.log('creating new window...');
    mainWindow = new BrowserWindow({
      width: 800,
      height: 600,
      webPreferences: {
        nodeIntegration: true
      }
    });

    // and load the index.html of the app.
    mainWindow.loadFile('index.html');
  });
  
})

