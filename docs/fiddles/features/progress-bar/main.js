const { app, BrowserWindow } = require('electron')

let indeterminateTimeout, progressInterval

function createWindow () {
  const win = new BrowserWindow({
    width: 800,
    height: 600
  })

  win.loadFile('index.html')

  const INCREMENT = 0.02
  const INTERVAL_DELAY = 100 // ms
  const INDETERMINATE_PAUSE = 5000 // ms

  let c = 0
  progressInterval = setInterval(() => {
    
    // update progress bar to next value
    win.setProgressBar(c)

      if (c > 1) {
        // set to indeterminate state
        win.setProgressBar(2)
        
        // stop the interval
        clearInterval(progressInterval)

        // and after INDETERMINATE_PAUSE reset the progress bar
        indeterminateTimeout = setTimeout(() => win.setProgressBar(-1), INDETERMINATE_PAUSE)
      }
      c += INCREMENT
  }, INTERVAL_DELAY)
}

app.whenReady().then(createWindow)

// before the app is terminated, clear both timers
app.on('before-quit', () => {
  clearInterval(progressInterval)
  clearTimeout(indeterminateTimeout)
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
