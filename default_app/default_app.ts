import { shell } from 'electron/common';
import { app, dialog, BrowserWindow, ipcMain } from 'electron/main';
import * as path from 'path';
import * as url from 'url';

let mainWindow: BrowserWindow | null = null;

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  app.quit();
});

function decorateURL (url: string) {
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

function isTrustedSender (webContents: Electron.WebContents) {
  if (webContents !== (mainWindow && mainWindow.webContents)) {
    return false;
  }

  try {
    return url.fileURLToPath(webContents.getURL()) === indexPath;
  } catch {
    return false;
  }
}

ipcMain.handle('bootstrap', (event) => {
  return isTrustedSender(event.sender) ? electronPath : null;
});

async function createWindow (backgroundColor?: string) {
  await app.whenReady();

  const options: Electron.BrowserWindowConstructorOptions = {
    width: 960,
    height: 620,
    autoHideMenuBar: true,
    backgroundColor,
    webPreferences: {
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

    const options: Electron.MessageBoxOptions = {
      title: 'Permission Request',
      message: `Allow '${parsedUrl.origin}' to access '${permission}'?`,
      buttons: ['OK', 'Cancel'],
      cancelId: 1
    };

    dialog.showMessageBox(mainWindow!, options).then(({ response }) => {
      done(response === 0);
    });
  });

  return mainWindow;
}

export const loadURL = async (appUrl: string) => {
  mainWindow = await createWindow();
  mainWindow.loadURL(appUrl);
  mainWindow.focus();
};

export const loadFile = async (appPath: string) => {
  mainWindow = await createWindow(appPath === 'index.html' ? '#2f3241' : undefined);
  mainWindow.loadFile(appPath);
  mainWindow.focus();
};
