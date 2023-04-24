import { shell, app, dialog, BrowserWindow, ipcMain, Menu } from 'electron';
import * as path from 'path';
import * as url from 'url';

let mainWindow: BrowserWindow | null = null;

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  app.quit();
});

function decorateURL(url: string) {
  // safely add `?utm_source=default_app
  const parsedUrl = new URL(url);
  parsedUrl.searchParams.append('utm_source', 'default_app');
  return parsedUrl.toString();
}

// Find the shortest path to the electron binary
const absoluteElectronPath = process.execPath;
const relativeElectronPath = path.relative(process.cwd(), absoluteElectronPath);
const electronPath = absoluteElectronPath.length < relativeElectronPath.length
  ? absoluteElectronPath
  : relativeElectronPath;

const indexPath = path.resolve(app.getAppPath(), 'index.html');

function isTrustedSender(webContents: Electron.WebContents) {
  if (webContents !== (mainWindow && mainWindow.webContents)) {
    return false;
  }

  try {
    return url.fileURLToPath(webContents.getURL()) === indexPath;
  } catch {
    return false;
  }
}

async function createWindow(backgroundColor?: string) {
  await app.whenReady();

  const options: Electron.BrowserWindowConstructorOptions = {
    width: 800,
    height: 600,
    autoHideMenuBar: true,
    backgroundColor,
    webPreferences: {
      nodeIntegration: true,
      preload: path.resolve(__dirname, 'preload.js'),
      contextIsolation: true,
      sandbox: true
    },
    useContentSize: true,
    show: false
  };

  if (process.platform === 'linux') {
    options.icon = path.join(__dirname, 'icon.png');
  }

  mainWindow = new BrowserWindow(options);
  mainWindow.on('ready-to-show', () => mainWindow!.show());

  mainWindow.webContents.setWindowOpenHandler(details => {
    shell.openExternal(decorateURL(details.url));
    return { action: 'deny' };
  });

  mainWindow.webContents.session.setPermissionRequestHandler((webContents, permission, done) => {
    const parsedUrl = new URL(webContents.getURL());

    const messageBoxOptions: Electron.MessageBoxOptions = {
      title: 'Permission Request',
      message: `Allow '${parsedUrl.origin}' to access '${permission}'?`,
      buttons: ['OK', 'Cancel'],
      cancelId: 1
    };

    dialog.showMessageBox(mainWindow!, messageBoxOptions).then(({ response }) => {
      done(response === 0);
    });
  });

  const template: Electron.MenuItemConstructorOptions[] = [
    {
      label: 'File',
      submenu: [
        { role: 'quit' }
      ]
    },
    {
      label: 'Help',
      submenu: [
        {
          label: 'Node.js Documentation',
          click() { shell.openExternal('https://nodejs.org/docs/') }
        },
        {
          label: 'Chromium Documentation',
          click() { shell.openExternal('https://www.chromium.org/Home') }
        },
        {
          label: 'Electron Documentation',
          click() { shell.openExternal('https://www.electronjs.org/docs') }
        }
      ]
    }
  ];

  const menu = Menu.buildFromTemplate(template);
  Menu.setApplicationMenu(menu);
};
