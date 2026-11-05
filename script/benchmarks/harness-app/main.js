// Host app for the crossbench benchmarks (see ../README.md).
//
// crossbench launches the stock `electron` binary through Electron's own
// chromedriver with `--app=<this dir>`. chromedriver attaches over CDP and
// drives the first BrowserWindow's webContents like a Chrome tab, so this app
// only has to open one window and get out of the way.
//
// It also hosts the Electron-side pieces a page cannot do on its own:
//   - One privileged custom scheme per file-loading strategy, all serving the
//     local filesystem, so the same N-file page can be timed through each:
//       cblegacy://  protocol.registerFileProtocol (deprecated)
//       cbfs://      protocol.handle + new Response(fs.readFile)
//       cbnet://     protocol.handle + net.fetch(file://)
//     file:// itself needs no handler.
//   - cbHarness.setSize(w, h) for the resize scenario (window.resizeTo is a
//     no-op for top-level windows and chromedriver's window-rect commands are
//     not implemented by Electron).
//
// The app must keep working on older Electron releases, which lack some of
// these APIs: anything missing is skipped and reported by --cb-capabilities
// so the runner leaves those stories out instead of timing out on them.
//
// `electron <dir> --cb-capabilities=<file>` writes the supported features to
// <file> as JSON and quits without opening a window.
const { app, BrowserWindow, ipcMain, net, protocol } = require('electron');

const fs = require('node:fs');
const path = require('node:path');
const url = require('node:url');

const SCHEMES = {
  cblegacy: 'registerFileProtocol',
  cbfs: 'handleFs',
  cbnet: 'handleNetFetch'
};

const MIME_TYPES = {
  '.html': 'text/html',
  '.js': 'text/javascript',
  '.mjs': 'text/javascript',
  '.css': 'text/css',
  '.json': 'application/json'
};

const capabilities = {
  electron: process.versions.electron,
  chrome: process.versions.chrome,
  registerFileProtocol: typeof protocol.registerFileProtocol === 'function',
  handleFs: typeof protocol.handle === 'function' && typeof globalThis.Response === 'function',
  handleNetFetch: typeof protocol.handle === 'function' && !!net && typeof net.fetch === 'function',
  setSize: typeof ipcMain.handle === 'function'
};

// Standard schemes need a host, so URLs look like cbnet://local/<abs path>.
const toFilePath = (requestUrl) => decodeURIComponent(new URL(requestUrl).pathname);

function registerSchemes() {
  if (capabilities.registerFileProtocol) {
    protocol.registerFileProtocol('cblegacy', (request, callback) => callback({ path: toFilePath(request.url) }));
  }
  if (capabilities.handleFs) {
    protocol.handle('cbfs', async (request) => {
      const filePath = toFilePath(request.url);
      const body = await fs.promises.readFile(filePath);
      return new Response(body, {
        headers: { 'content-type': MIME_TYPES[path.extname(filePath)] || 'application/octet-stream' }
      });
    });
  }
  if (capabilities.handleNetFetch) {
    protocol.handle('cbnet', (request) => net.fetch(url.pathToFileURL(toFilePath(request.url)).toString()));
  }
}

function startHarness() {
  // Must happen before the ready event.
  const supportedSchemes = Object.keys(SCHEMES).filter((scheme) => capabilities[SCHEMES[scheme]]);
  if (typeof protocol.registerSchemesAsPrivileged === 'function') {
    protocol.registerSchemesAsPrivileged(
      supportedSchemes.map((scheme) => ({
        scheme,
        privileges: { standard: true, secure: true, supportFetchAPI: true, corsEnabled: true, codeCache: true }
      }))
    );
  }
  app.whenReady().then(() => {
    registerSchemes();
    createWindow();
  });
  app.on('window-all-closed', () => app.quit());
}

function createWindow() {
  const win = new BrowserWindow({
    width: 1280,
    height: 800,
    show: true,
    webPreferences: {
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
      // A hidden or occluded window must keep producing frames.
      backgroundThrottling: false,
      preload: path.join(__dirname, 'preload.js')
    }
  });

  if (capabilities.setSize) {
    ipcMain.handle('cb-set-size', (_event, width, height) => {
      win.setSize(width, height);
      return win.getSize();
    });
    ipcMain.handle('cb-get-size', () => win.getSize());
  }

  win.loadURL('about:blank');
}

const capabilitiesArg = process.argv.find((arg) => arg.startsWith('--cb-capabilities='));
if (capabilitiesArg) {
  fs.writeFileSync(capabilitiesArg.slice('--cb-capabilities='.length), JSON.stringify(capabilities, null, 2));
  app.exit(0);
} else {
  startHarness();
}
