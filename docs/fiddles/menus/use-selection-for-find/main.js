const { app, BrowserWindow, ClipboardItem, Menu, clipboard, ipcMain } = require('electron/main')
const path = require('node:path')

const isMac = process.platform === 'darwin'
const FIND_TEXT_MIME = 'electron application/findtext'

// The text the app is currently searching for. On macOS this is kept in sync
// with the system-wide find pasteboard so that other apps can share it.
let findText = ''
let lastSearchedText = ''

async function readFindPasteboard () {
  const items = await clipboard.read()
  for (const item of items) {
    if (item.types.includes(FIND_TEXT_MIME)) {
      const blob = await item.getType(FIND_TEXT_MIME)
      return blob.text()
    }
  }
  return ''
}

async function setFindText (text) {
  updateFindText(text)
  if (isMac) {
    // Writing to the find pasteboard does not touch the regular clipboard.
    await clipboard.write([new ClipboardItem({ [FIND_TEXT_MIME]: text })])
  }
}

function updateFindText (text) {
  findText = text
  for (const win of BrowserWindow.getAllWindows()) {
    // Clear the highlights left over from the previous search.
    win.webContents.stopFindInPage('keepSelection')
    win.webContents.send('find:text-changed', text)
  }
}

function findInFocusedWindow (forward) {
  const win = BrowserWindow.getFocusedWindow()
  if (!win || !findText) return
  // `findNext: true` starts a new search session, so only pass it when the
  // search text has changed since the last request.
  win.webContents.findInPage(findText, { forward, findNext: findText !== lastSearchedText })
  lastSearchedText = findText
}

function sendToFocusedWindow (channel) {
  BrowserWindow.getFocusedWindow()?.webContents.send(channel)
}

function createWindow () {
  const mainWindow = new BrowserWindow({
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  })

  mainWindow.webContents.on('did-finish-load', () => {
    mainWindow.webContents.send('find:text-changed', findText)
  })

  mainWindow.webContents.on('found-in-page', (_event, result) => {
    mainWindow.webContents.send('find:result', result.activeMatchOrdinal, result.matches)
  })

  mainWindow.loadFile('index.html')
}

const template = [
  ...(isMac ? [{ role: 'appMenu' }] : []),
  {
    label: 'Edit',
    submenu: [
      { role: 'undo' },
      { role: 'redo' },
      { type: 'separator' },
      { role: 'cut' },
      { role: 'copy' },
      { role: 'paste' },
      { role: 'selectAll' },
      { type: 'separator' },
      {
        label: 'Find',
        submenu: [
          {
            label: 'Find…',
            accelerator: 'CommandOrControl+F',
            click: () => sendToFocusedWindow('find:focus')
          },
          {
            label: 'Find Next',
            accelerator: 'CommandOrControl+G',
            click: () => findInFocusedWindow(true)
          },
          {
            label: 'Find Previous',
            accelerator: 'Shift+CommandOrControl+G',
            click: () => findInFocusedWindow(false)
          },
          // highlight-start
          {
            label: 'Use Selection for Find',
            accelerator: 'CommandOrControl+E',
            click: () => sendToFocusedWindow('find:use-selection')
          }
          // highlight-end
        ]
      }
    ]
  },
  { role: 'windowMenu' }
]

// Sent by the page when "Use Selection for Find" is used.
ipcMain.on('find:set-text', (_event, text) => {
  setFindText(text)
})

// Sent by the page when Enter is pressed in the find field.
ipcMain.on('find:search', async (_event, text) => {
  await setFindText(text)
  findInFocusedWindow(true)
})

// Sent by the page when Enter or Shift+Enter is pressed in the empty find field.
ipcMain.on('find:next', (_event, forward) => {
  findInFocusedWindow(forward)
})

// highlight-start
// Pick up text that another app put on the find pasteboard while this app
// was in the background.
app.on('did-become-active', async () => {
  const text = await readFindPasteboard()
  if (text && text !== findText) updateFindText(text)
})
// highlight-end

app.whenReady().then(async () => {
  if (isMac) findText = await readFindPasteboard()

  Menu.setApplicationMenu(Menu.buildFromTemplate(template))
  createWindow()

  app.on('activate', function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit()
})
