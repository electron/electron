// Starts a SharedWorker with --enable-experimental-web-platform-features set.
// Exits with 0 once the worker's message arrives, or 1 if the renderer goes
// away first (e.g. killed for a bad Mojo message).
const { app, BrowserWindow } = require('electron');

const http = require('node:http');

app.commandLine.appendSwitch('enable-experimental-web-platform-features');

const server = http.createServer((req, res) => {
  if (req.url === '/worker.js') {
    res.setHeader('content-type', 'text/javascript');
    res.end('onconnect = (e) => e.ports[0].postMessage("hello");');
  } else {
    res.setHeader('content-type', 'text/html');
    res.end('<script>new SharedWorker("/worker.js").port.onmessage = (e) => { document.title = e.data; };</script>');
  }
});

app.whenReady().then(() => {
  server.listen(0, '127.0.0.1', () => {
    const { port } = server.address();
    const w = new BrowserWindow({ show: false });
    w.webContents.on('render-process-gone', (event, details) => {
      console.log(`render-process-gone: ${JSON.stringify(details)}`);
      process.exit(1);
    });
    w.webContents.on('page-title-updated', (event, title) => {
      if (title === 'hello') {
        console.log(`title: ${title}`);
        process.exit(0);
      }
    });
    w.loadURL(`http://127.0.0.1:${port}/`);
  });
});
