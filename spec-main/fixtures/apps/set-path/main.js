const http = require('http');
const { app, BrowserWindow } = require('electron');

if (process.argv.length > 3) {
  app.setPath(process.argv[2], process.argv[3]);
}

app.once('ready', () => {
  const server = http.createServer((request, response) => {
    response.end('some text');
  }).listen(0, '127.0.0.1', () => {
    const serverUrl = 'http://127.0.0.1:' + server.address().port;
    const mainWindow = new BrowserWindow({ show: false });
    mainWindow.webContents.once('did-finish-load', () => {
      app.quit();
    });
    mainWindow.loadURL(serverUrl);
  });
});
