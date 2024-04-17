// Display native system dialogs for opening and saving files, alerting, etc.
//
// For more info, see:
// https://electronjs.org/docs/api/dialog

const { app, BrowserWindow, dialog } = require('electron/main');

app.whenReady().then(async () => {
  const mainWindow = new BrowserWindow({ height: 600, width: 600 });

  // Show an "Open File" dialog and attempt to open
  // the chosen file in our window.
  try {
    dialog.showOpenDialog(mainWindow, {
      properties: ['openFile', 'showHiddenFiles'],
      buttonLabel: 'OPEN ME!'
    }).then(({ filePaths, canceled }) => {
      if (canceled) {
        console.log('Dialog was canceled');
      } else {
        const file = filePaths[0];
        mainWindow.loadURL(`file://${file}`);
      }
    });
    // console.log('dialog should be closed!!!');
  } catch (err) {
    console.log(err);
  }
});
