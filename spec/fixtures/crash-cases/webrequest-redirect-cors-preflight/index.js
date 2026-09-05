// Returning a redirectURL from onBeforeRequest for a CORS preflight request is
// not supported and should fail the request rather than crash the main process.
const { app, BrowserWindow, session } = require('electron');

const http = require('node:http');
const { once } = require('node:events');

app.whenReady().then(async () => {
  const page = http.createServer((req, res) => res.end('<html></html>'));
  const api = http.createServer((req, res) => {
    res.setHeader('access-control-allow-origin', '*');
    res.setHeader('access-control-allow-headers', '*');
    res.end('{}');
  });
  await Promise.all([page, api].map((s) => once(s.listen(0, '127.0.0.1'), 'listening')));
  const pageUrl = `http://127.0.0.1:${page.address().port}/`;
  const apiUrl = `http://127.0.0.1:${api.address().port}/`;

  session.defaultSession.webRequest.onBeforeRequest((details, callback) => {
    if (details.method === 'OPTIONS' && details.url === apiUrl) {
      callback({ redirectURL: apiUrl + 'elsewhere' });
    } else {
      callback({});
    }
  });

  const w = new BrowserWindow({ show: false });
  await w.loadURL(pageUrl);
  // Cross-origin with a non-simple header so that a preflight is sent.
  await w.webContents.executeJavaScript(`
    fetch(${JSON.stringify(apiUrl)}, {
      method: 'POST',
      headers: { 'content-type': 'application/json', 'x-custom': '1' },
      body: '{}'
    }).then(r => r.status, e => String(e))
  `);
  app.quit();
});
