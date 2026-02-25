const { app, BrowserWindow, protocol, session } = require('electron/main');

const path = require('node:path');

const html = '<html><body><h1>EMPTY PAGE</h1></body></html>';
const scheme = 'example';

protocol.registerSchemesAsPrivileged([
  {
    scheme,
    privileges: {
      standard: true,
      secure: true,
      allowServiceWorkers: true,
      supportFetchAPI: true,
      bypassCSP: false,
      corsEnabled: true,
      stream: true
    }
  }
]);

app.enableExtensionsOnAllProtocols();

app.whenReady().then(async () => {
  const ses = session.defaultSession;

  ses.protocol.handle(scheme, () => new Response(html, {
    headers: { 'Content-Type': 'text/html' }
  }));

  await ses.extensions.loadExtension(path.join(__dirname, 'extension'));

  const win = new BrowserWindow();

  win.on('page-title-updated', (_event, title) => {
    console.log(`Title: ${title}`);
    app.quit();
  });

  await win.loadURL(`${scheme}://app/`);
});
