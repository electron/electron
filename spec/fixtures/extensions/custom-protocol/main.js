const { app, BrowserWindow, protocol, session } = require('electron/main');

const path = require('node:path');

const html = '<html><body><h1>EMPTY PAGE</h1></body></html>';
const scheme = 'example';

protocol.registerSchemesAsPrivileged([
  {
    scheme,
    privileges: {
      standard: true,
      allowExtensions: true
    }
  }
]);

app
  .whenReady()
  .then(async () => {
    const ses = session.defaultSession;

    ses.protocol.handle(
      scheme,
      () =>
        new Response(html, {
          headers: { 'Content-Type': 'text/html' }
        })
    );

    // loadExtension() can resolve before the background message listener exists.
    const backgroundReady = new Promise((resolve) => {
      const onMessage = (_event, { message }) => {
        if (message !== 'CUSTOM PROTOCOL BACKGROUND READY') return;
        ses.serviceWorkers.removeListener('console-message', onMessage);
        resolve();
      };
      ses.serviceWorkers.on('console-message', onMessage);
    });

    await ses.extensions.loadExtension(path.join(__dirname, 'extension'));
    await backgroundReady;

    const win = new BrowserWindow();

    win.on('page-title-updated', (_event, title) => {
      console.log(`Title: ${title}`);
      app.quit();
    });

    await win.loadURL(`${scheme}://app/`);
  })
  .catch((error) => {
    console.error(error);
    app.exit(1);
  });
