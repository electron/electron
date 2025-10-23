const { app, BrowserWindow, Menu } = require('electron/main')
const { shell } = require('electron/common')

function createWindow () {
  const win = new BrowserWindow()
  win.loadFile('index.html')
}

function closeAllWindows () {
  const wins = BrowserWindow.getAllWindows()
  for (const win of wins) {
    win.close()
  }
}

app.whenReady().then(() => {
  createWindow()

  const dockMenu = Menu.buildFromTemplate([
    {
      label: 'New Window',
      click: () => { createWindow() }
    },
    {
      label: 'Close All Windows',
      click: () => { closeAllWindows() }
    },
    {
      label: 'Open Electron Docs',
      click: () => {
        shell.openExternal('https://electronjs.org/docs')
      }
    }
    // add more menu options to the array
  ])

  app.dock.setMenu(dockMenu)

  app.on('activate', function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit()
})
