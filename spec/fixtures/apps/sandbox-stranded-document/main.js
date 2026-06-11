// Repro for the sandboxed-renderer "startupData is null" TypeError: a
// main-process navigation guard cancels the first navigation before commit,
// stranding the view on its initial empty document, then a forced script
// context (CDP debugger or DevTools) runs the sandbox bundle. Startup data
// is now pushed at frame creation, so PRELOAD-RAN logs instead of a throw.
const { app, BaseWindow, WebContentsView } = require('electron');
const http = require('node:http');
const path = require('node:path');

// 'debugger' (default), 'devtools', or 'nothing'
const MODE = process.env.STRAND_MODE || 'debugger';

app.whenReady().then(async () => {
  const server = http.createServer((req, res) => {
    if (req.url === '/start') {
      res.writeHead(302, { Location: '/blocked' });
      res.end();
    } else {
      res.writeHead(200, { 'Content-Type': 'text/html' });
      res.end('<html><body>blocked page</body></html>');
    }
  });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const base = `http://127.0.0.1:${server.address().port}`;

  const win = new BaseWindow({ show: false });
  const view = new WebContentsView({
    webPreferences: {
      sandbox: true,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js')
    }
  });
  win.contentView.addChildView(view);
  const wc = view.webContents;

  wc.on('console-message', (event) => {
    console.log(`[console-message level=${event.level}] ${event.message}`);
  });

  // The navigation guard: cancel the redirect, stranding the view on the
  // initial empty document.
  wc.on('will-redirect', (event) => {
    console.log('[will-redirect] cancelling');
    event.preventDefault();
  });

  console.log(`[load] ${base}/start (mode=${MODE})`);
  wc.loadURL(`${base}/start`).catch(err => {
    console.log(`[loadURL rejected] ${err.message || err}`);
  });

  setTimeout(() => {
    console.log(`[provoke] mode=${MODE}`);
    if (MODE === 'debugger') {
      wc.debugger.attach('1.3');
      wc.debugger.sendCommand('Runtime.enable');
    } else if (MODE === 'devtools') {
      wc.openDevTools({ mode: 'detach' });
    }
    setTimeout(() => {
      console.log('[exit]');
      app.exit(0);
    }, 3000);
  }, 1500);
});
