const { app, BaseWindow, WebContentsView } = require('electron')

function createWindow () {
  const win = new BaseWindow({
    width: 100,
    height: 100,
    resizable: false,
    frame: false,
    transparent: true
  })

  const view = new WebContentsView()
  win.contentView.addChildView(view)
  view.webContents.loadFile('index.html')
  view.setBounds({ x: 0, y: 0, width: 100, height: 100 })

  view.setBackgroundColor('rgba(0, 0, 0, 0)') // Set view to transparent
}

app.whenReady().then(() => {
  createWindow()
})
