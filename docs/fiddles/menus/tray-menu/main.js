const { app, BrowserWindow, Menu, Tray } = require('electron/main')
const { nativeImage } = require('electron/common')

// save a reference to the Tray object globally to avoid garbage collection
let tray = null

function createWindow () {
  const mainWindow = new BrowserWindow()
  mainWindow.loadFile('index.html')
}

// The Tray object can only be instantiated after the 'ready' event is fired
app.whenReady().then(() => {
  createWindow()

  const red = nativeImage.createFromDataURL('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAACTSURBVHgBpZKBCYAgEEV/TeAIjuIIbdQIuUGt0CS1gW1iZ2jIVaTnhw+Cvs8/OYDJA4Y8kR3ZR2/kmazxJbpUEfQ/Dm/UG7wVwHkjlQdMFfDdJMFaACebnjJGyDWgcnZu1/lrCrl6NCoEHJBrDwEr5NrT6ko/UV8xdLAC2N49mlc5CylpYh8wCwqrvbBGLoKGvz8Bfq0QPWEUo/EAAAAASUVORK5CYII=')
  const green = nativeImage.createFromDataURL('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAACOSURBVHgBpZLRDYAgEEOrEzgCozCCGzkCbKArOIlugJvgoRAUNcLRpvGH19TkgFQWkqIohhK8UEaKwKcsOg/+WR1vX+AlA74u6q4FqgCOSzwsGHCwbKliAF89Cv89tWmOT4VaVMoVbOBrdQUz+FrD6XItzh4LzYB1HFJ9yrEkZ4l+wvcid9pTssh4UKbPd+4vED2Nd54iAAAAAElFTkSuQmCC')

  tray = new Tray(red)
  tray.setToolTip('Tray Icon Demo')

  const contextMenu = Menu.buildFromTemplate([
    {
      label: 'Open App',
      click: () => {
        const wins = BrowserWindow.getAllWindows()
        if (wins.length === 0) {
          createWindow()
        } else {
          wins[0].focus()
        }
      }
    },
    {
      label: 'Set Green Icon',
      type: 'checkbox',
      click: ({ checked }) => {
        checked ? tray.setImage(green) : tray.setImage(red)
      }
    },
    {
      label: 'Set Title',
      type: 'checkbox',
      click: ({ checked }) => {
        checked ? tray.setTitle('Title') : tray.setTitle('')
      }
    },
    { role: 'quit' }
  ])

  tray.setContextMenu(contextMenu)
})

app.on('window-all-closed', function () {
  // This will prevent the app from closing when windows close
})

app.on('activate', function () {
  if (BrowserWindow.getAllWindows().length === 0) createWindow()
})
