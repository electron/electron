const { app, BrowserWindow } = require('electron')
const fs = require('fs');

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

let recentlyUsedDocument = fs.writeFileSync('recently-used.md', 'Lorem Ipsum');
app.addRecentDocument(`${process.cwd()}/${recentlyUsedDocument}`)

app.whenReady().then(createWindow)

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
