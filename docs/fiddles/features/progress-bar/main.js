const { app, BrowserWindow } = require('electron')

function createWindow () {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      nodeIntegration: true
    }
  })

  win.loadFile('index.html')
  
  const INCREMENT = 0.02;
  const INTERVAL_DELAY = 100; //ms
  let c = 0;
  let interval = setInterval(() => {
    win.setProgressBar(c);
    if(c > 1) {
      win.setProgressBar(-1);
      clearTimeout(interval);
    }
    c += INCREMENT;
  }, INTERVAL_DELAY);
}


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
