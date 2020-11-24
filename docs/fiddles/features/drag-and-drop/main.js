const { app, BrowserWindow, ipcMain, nativeImage, NativeImage } = require('electron')
const fs = require('fs');
const http = require('http');

function createWindow () {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      nodeIntegration: true
    }
  })

  win.loadFile('index.html')
}
const iconName = 'iconForDragAndDrop.png';
const icon = fs.createWriteStream(`${process.cwd()}/${iconName}`);
http.get('http://img.icons8.com/ios/452/drag-and-drop.png', (response) => {
  response.pipe(icon);
});

app.whenReady().then(createWindow)

ipcMain.on('ondragstart', (event, filePath) => {
  event.sender.startDrag({
    file: filePath,
    icon: `${process.cwd()}/${iconName}`
  })
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow()
  }
})
