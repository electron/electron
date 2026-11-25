const { app, ipcMain, BrowserWindow } = require('electron');

const http = require('node:http');

if (process.argv.length > 3) {
  app.setPath(process.argv[2], process.argv[3]);
}

// Each step is logged before it starts, so that the output of a run that
// times out shows how far it got.
const html = `
<script>
const { ipcRenderer } = require('electron')

async function main() {
  ipcRenderer.send('step', 'writing localStorage')
  localStorage.setItem('myCat', 'Tom')
  ipcRenderer.send('step', 'opening IndexedDB')
  const db = indexedDB.open('db-name', 1)
  await new Promise((resolve, reject) => {
    db.onsuccess = resolve
    db.onerror = () => reject(db.error)
  })
  ipcRenderer.send('step', 'registering the service worker')
  await navigator.serviceWorker.register('sw.js', {scope: './'})
}

main().then(() => {
  ipcRenderer.send('success')
}, (error) => {
  ipcRenderer.send('failure', String(error))
})
</script>
`;

const js = 'console.log("From service worker")';

// Only the first error counts. Set by it and when the app starts to quit, as
// closing the window on a quit or an exit can raise more errors that only
// follow from what came first.
let done = false;

// Exit with the error instead of waiting for the test to time out.
const fail = (error) => {
  if (done) return;
  done = true;
  console.error(error);
  app.exit(1);
};

process.on('uncaughtException', fail);
app.on('before-quit', () => {
  done = true;
});

app.once('ready', () => {
  ipcMain.on('step', (event, step) => console.log(step));
  ipcMain.on('failure', (event, error) => fail(error));
  ipcMain.on('success', () => {
    console.log('quitting');
    app.quit();
  });

  const server = http
    .createServer((request, response) => {
      if (request.url === '/') {
        response.writeHead(200, { 'Content-Type': 'text/html' });
        response.end(html);
      } else if (request.url === '/sw.js') {
        response.writeHead(200, { 'Content-Type': 'text/javascript' });
        response.end(js);
      }
    })
    .listen(0, '127.0.0.1', () => {
      const serverUrl = 'http://127.0.0.1:' + server.address().port;
      const mainWindow = new BrowserWindow({
        show: false,
        webPreferences: { webSecurity: true, nodeIntegration: true, contextIsolation: false }
      });
      // Not loadURL()'s rejection: a renderer crash rejects it too, just
      // before 'render-process-gone' says why.
      mainWindow.webContents.on('did-fail-load', (event, code, description, url, isMainFrame) => {
        if (isMainFrame) fail(`Could not load ${url}: ${description} (${code})`);
      });
      mainWindow.webContents.on('render-process-gone', (event, details) => {
        fail(`The renderer went away: ${details.reason}, exit code ${details.exitCode}`);
      });
      console.log(`loading ${serverUrl}`);
      mainWindow.loadURL(serverUrl);
    });
});
