const http = require('http');
const { app, ipcMain, BrowserWindow } = require('electron');

if (process.argv.length > 3) {
  app.setPath(process.argv[2], process.argv[3]);
}

const html = `
<script>
async function main() {
  localStorage.setItem('myCat', 'Tom')
  const db = indexedDB.open('db-name', 1)
  await new Promise(resolve => db.onsuccess = resolve)
  await navigator.serviceWorker.register('sw.js', {scope: './'})
}

main().then(() => {
  require('electron').ipcRenderer.send('success')
})
</script>
`;

const js = 'console.log("From service worker")';

app.once('ready', () => {
  ipcMain.on('success', () => {
    app.quit();
  });

  const server = http.createServer((request, response) => {
    if (request.url === '/') {
      response.writeHead(200, { 'Content-Type': 'text/html' });
      response.end(html);
    } else if (request.url === '/sw.js') {
      response.writeHead(200, { 'Content-Type': 'text/javascript' });
      response.end(js);
    }
  }).listen(0, '127.0.0.1', () => {
    const serverUrl = 'http://127.0.0.1:' + server.address().port;
    const mainWindow = new BrowserWindow({ show: false, webPreferences: { webSecurity: true, nodeIntegration: true, contextIsolation: false } });
    mainWindow.loadURL(serverUrl);
  });
});
