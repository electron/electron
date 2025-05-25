const {
  app,
  BrowserWindow,
  Menu,
  MenuItem,
  ipcMain,
  shell,
  dialog,
  autoUpdater
} = require('electron/main');
const path = require('node:path');

let mainWindow;

// Context menu (right-click)
const contextMenu = new Menu();
contextMenu.append(new MenuItem({ label: 'Hello' }));
contextMenu.append(new MenuItem({ type: 'separator' }));
contextMenu.append(new MenuItem({ label: 'Electron', type: 'checkbox', checked: true }));

// Application Menu Template
const template = [
  {
    label: 'Edit',
    submenu: [
      { label: 'Undo', accelerator: 'CmdOrCtrl+Z', role: 'undo' },
      { label: 'Redo', accelerator: 'Shift+CmdOrCtrl+Z', role: 'redo' },
      { type: 'separator' },
      { label: 'Cut', accelerator: 'CmdOrCtrl+X', role: 'cut' },
      { label: 'Copy', accelerator: 'CmdOrCtrl+C', role: 'copy' },
      { label: 'Paste', accelerator: 'CmdOrCtrl+V', role: 'paste' },
      { label: 'Select All', accelerator: 'CmdOrCtrl+A', role: 'selectAll' }
    ]
  },
  {
    label: 'View',
    submenu: [
      {
        label: 'Reload',
        accelerator: 'CmdOrCtrl+R',
        click: (_, focusedWindow) => {
          if (focusedWindow) {
            if (focusedWindow.id === 1) {
              for (const win of BrowserWindow.getAllWindows()) {
                if (win.id > 1) win.close();
              }
            }
            focusedWindow.reload();
          }
        }
      },
      {
        label: 'Toggle Full Screen',
        accelerator: process.platform === 'darwin' ? 'Ctrl+Command+F' : 'F11',
        click: (_, focusedWindow) => {
          if (focusedWindow) {
            focusedWindow.setFullScreen(!focusedWindow.isFullScreen());
          }
        }
      },
      {
        label: 'Toggle Developer Tools',
        accelerator: process.platform === 'darwin' ? 'Alt+Command+I' : 'Ctrl+Shift+I',
        click: (_, focusedWindow) => {
          if (focusedWindow) {
            focusedWindow.webContents.toggleDevTools();
          }
        }
      },
      { type: 'separator' },
      {
        label: 'App Menu Demo',
        click: (item, focusedWindow) => {
          if (focusedWindow) {
            dialog.showMessageBox(focusedWindow, {
              type: 'info',
              title: 'Application Menu Demo',
              buttons: ['OK'],
              message: 'This is a demo for creating menus in Electron.'
            });
          }
        }
      }
    ]
  },
  {
    label: 'Window',
    role: 'window',
    submenu: [
      { label: 'Minimize', accelerator: 'CmdOrCtrl+M', role: 'minimize' },
      { label: 'Close', accelerator: 'CmdOrCtrl+W', role: 'close' },
      { type: 'separator' },
      {
        label: 'Reopen Window',
        accelerator: 'CmdOrCtrl+Shift+T',
        enabled: false,
        key: 'reopenMenuItem',
        click: () => app.emit('activate')
      }
    ]
  },
  {
    label: 'Help',
    role: 'help',
    submenu: [
      {
        label: 'Learn More',
        click: () => {
          shell.openExternal('https://electronjs.org');
        }
      }
    ]
  }
];

function addUpdateMenuItems(items, position) {
  if (process.mas) return;

  const version = app.getVersion();
  const updateItems = [
    { label: `Version ${version}`, enabled: false },
    { label: 'Checking for Update', enabled: false, key: 'checkingForUpdate' },
    {
      label: 'Check for Update',
      visible: false,
      key: 'checkForUpdate',
      click: () => autoUpdater.checkForUpdates()
    },
    {
      label: 'Restart and Install Update',
      enabled: true,
      visible: false,
      key: 'restartToUpdate',
      click: () => autoUpdater.quitAndInstall()
    }
  ];

  items.splice(position, 0, ...updateItems);
}

function findReopenMenuItem() {
  const menu = Menu.getApplicationMenu();
  if (!menu) return null;

  for (const item of menu.items) {
    if (item.submenu) {
      for (const subitem of item.submenu.items) {
        if (subitem.key === 'reopenMenuItem') {
          return subitem;
        }
      }
    }
  }
  return null;
}

// macOS specific adjustments
if (process.platform === 'darwin') {
  const name = app.getName();
  template.unshift({
    label: name,
    submenu: [
      { label: `About ${name}`, role: 'about' },
      { type: 'separator' },
      { label: 'Services', role: 'services', submenu: [] },
      { type: 'separator' },
      { label: `Hide ${name}`, accelerator: 'Command+H', role: 'hide' },
      { label: 'Hide Others', accelerator: 'Command+Alt+H', role: 'hideOthers' },
      { label: 'Show All', role: 'unhide' },
      { type: 'separator' },
      {
        label: 'Quit',
        accelerator: 'Command+Q',
        click: () => app.quit()
      }
    ]
  });

  template[3].submenu.push({ type: 'separator' }, { label: 'Bring All to Front', role: 'front' });
  addUpdateMenuItems(template[0].submenu, 1);
} else if (process.platform === 'win32') {
  const helpMenu = template[template.length - 1].submenu;
  addUpdateMenuItems(helpMenu, 0);
}

// Create the main window
function createWindow() {
  mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  });

  mainWindow.loadFile('index.html');

  mainWindow.on('closed', () => {
    mainWindow = null;
  });

  mainWindow.webContents.on('will-navigate', (event, url) => {
    event.preventDefault();
    shell.openExternal(url);
  });
}

// App lifecycle
app.whenReady().then(() => {
  createWindow();
  const appMenu = Menu.buildFromTemplate(template);
  Menu.setApplicationMenu(appMenu);
});

app.on('window-all-closed', () => {
  const reopenMenuItem = findReopenMenuItem();
  if (reopenMenuItem) reopenMenuItem.enabled = true;

  if (process.platform !== 'darwin') app.quit();
});

app.on('activate', () => {
  if (mainWindow === null) createWindow();
});

app.on('browser-window-created', (event, win) => {
  const reopenMenuItem = findReopenMenuItem();
  if (reopenMenuItem) reopenMenuItem.enabled = false;

  win.webContents.on('context-menu', (e, params) => {
    contextMenu.popup(win, params.x, params.y);
  });
});

ipcMain.on('show-context-menu', (event) => {
  const win = BrowserWindow.fromWebContents(event.sender);
  contextMenu.popup(win);
});
