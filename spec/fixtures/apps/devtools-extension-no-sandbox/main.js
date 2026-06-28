// Regression fixture for https://github.com/electron/electron/issues/48705.
// When launched with --no-sandbox, the extension's devtools_page runs in a
// non-sandboxed renderer. Renderer init must not abort there, otherwise
// chrome.devtools.panels.create() never runs and the panel never appears.
// Exits 0 when the extension panel runs (sends 'winning'), 1 on timeout.
const { app, BrowserWindow, ipcMain, session } = require('electron');
const http = require('node:http');
const path = require('node:path');

const extensionPath = path.join(__dirname, '..', '..', 'extensions', 'devtools-extension');

const showLastDevToolsPanel = (w) => {
  w.webContents.once('devtools-opened', () => {
    const show = () => {
      if (w == null || w.isDestroyed()) return;
      const { devToolsWebContents } = w;
      if (devToolsWebContents == null || devToolsWebContents.isDestroyed()) return;
      const showLastPanel = () => {
        const { EUI } = window;
        const instance = EUI.InspectorView.InspectorView.instance();
        const tabs = instance.tabbedPane.tabs;
        instance.showPanel(tabs[tabs.length - 1].id);
      };
      devToolsWebContents.executeJavaScript(`(${showLastPanel})()`, false)
        .then(() => setTimeout(show, 100))
        .catch(() => setTimeout(show, 100));
    };
    setTimeout(show, 100);
  });
};

app.whenReady().then(async () => {
  const server = http.createServer((_req, res) => res.end('<title>host</title>'));
  await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
  const url = `http://127.0.0.1:${server.address().port}/`;

  await session.defaultSession.extensions.loadExtension(extensionPath);

  ipcMain.once('winning', () => app.exit(0));
  setTimeout(() => app.exit(1), 30000);

  const w = new BrowserWindow({
    show: true,
    webPreferences: { nodeIntegration: true, contextIsolation: false }
  });
  await w.loadURL(url);
  w.webContents.openDevTools();
  showLastDevToolsPanel(w);
});
