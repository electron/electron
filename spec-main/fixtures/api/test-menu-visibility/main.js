const { app, BrowserWindow } = require('electron')

let win
app.on('ready', function () {
  win = new BrowserWindow({})
  win.loadURL('about:blank')
  win.setMenuBarVisibility(false)

  setTimeout(() => {
    if (win.isMenuBarVisible()) {
      console.log('Window has a menu')
    } else {
      console.log('Window has no menu')
    }
    app.quit()
  })
})
