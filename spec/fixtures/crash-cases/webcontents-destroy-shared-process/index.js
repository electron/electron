// Regression test: destroying a webContents that shares its renderer process
// with another webContents used to leave a dangling pointer that was read the
// next time that process was (re)launched, e.g. for a service worker.

const { app, BrowserWindow, session } = require('electron');

const http = require('node:http');
const { once } = require('node:events');
const { setTimeout } = require('node:timers/promises');

// Make window.open() children share the opener's process deterministically.
app.commandLine.appendSwitch('disable-features', 'SpareRendererForSitePerProcess');

const pages = {
  '/index.html': `<!DOCTYPE html><script>
    window.swReady = navigator.serviceWorker.register('/sw.js', { scope: '/' })
      .then(() => navigator.serviceWorker.ready).then(() => true);
  </script>`,
  '/child.html': '<!DOCTYPE html><h1>child</h1>',
  '/sw.js': `self.addEventListener('install', () => self.skipWaiting());
    self.addEventListener('activate', (e) => e.waitUntil(self.clients.claim()));
    self.addEventListener('fetch', () => {});`
};

const server = http.createServer((req, res) => {
  const { pathname } = new URL(req.url, 'http://localhost');
  const body = pages[pathname];
  if (!body) {
    res.statusCode = 404;
    res.end();
    return;
  }
  res.setHeader('Content-Type', pathname.endsWith('.js') ? 'application/javascript' : 'text/html');
  res.setHeader('Cache-Control', 'no-store');
  res.end(body);
});

app.whenReady().then(async () => {
  await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
  const base = `http://localhost:${server.address().port}`;

  const w1 = new BrowserWindow({ show: false, webPreferences: { sandbox: true } });
  w1.webContents.setWindowOpenHandler(() => ({
    action: 'allow',
    outlivesOpener: true,
    overrideBrowserWindowOptions: { show: false, webPreferences: { sandbox: true } }
  }));
  await w1.loadURL(`${base}/index.html`);
  await w1.webContents.executeJavaScript('window.swReady');

  const created = once(w1.webContents, 'did-create-window');
  w1.webContents.executeJavaScript(`window.open('${base}/child.html'); true`);
  const [w2] = await created;
  if (w2.webContents.isLoading()) await once(w2.webContents, 'did-stop-loading');
  await setTimeout(300);
  if (w1.webContents.getOSProcessId() !== w2.webContents.getOSProcessId()) {
    throw new Error('expected the opener and the popup to share a renderer process');
  }

  // Make w1 the most recent registrant for the shared process, then destroy it
  // while w2 keeps the process host alive.
  await w1.loadURL(`${base}/index.html?again=1`);
  w1.webContents.destroy();
  await setTimeout(500);

  // Kill the shared renderer and have the service worker relaunch it.
  const gone = once(w2.webContents, 'render-process-gone');
  w2.webContents.forcefullyCrashRenderer();
  await gone;
  await setTimeout(300);
  await session.defaultSession.serviceWorkers.startWorkerForScope(`${base}/`).catch(() => {});
  await setTimeout(300);

  server.close();
  app.quit();
});

app.on('window-all-closed', () => {});
