const { app, BrowserWindow, protocol, session } = require('electron/main');

const { once } = require('node:events');
const path = require('node:path');

const html = '<html><body><h1>EMPTY PAGE</h1></body></html>';
const scheme = 'custom';

protocol.registerSchemesAsPrivileged([
  {
    scheme,
    privileges: {
      standard: true,
      allowExtensions: true
    }
  }
]);

app.whenReady().then(async () => {
  const ses = session.defaultSession;

  ses.protocol.handle(
    scheme,
    () =>
      new Response(html, {
        headers: { 'Content-Type': 'text/html' }
      })
  );

  await ses.extensions.loadExtension(path.join(__dirname, 'extension'));

  const win = new BrowserWindow();

  win.webContents.openDevTools();
  await once(win.webContents, 'devtools-opened');

  // The extension's devtools_page re-announces the marker on an interval to
  // close a lost-event race (see extension/devtools.js). Because `app.quit()`
  // is async, a second announce can arrive before the process tears down and
  // log the marker twice; guard so the first marker wins and repeats are
  // ignored.
  let handled = false;
  win.devToolsWebContents.on('console-message', ({ message }) => {
    if (handled || message !== 'ELECTRON TEST PANEL created') return;
    handled = true;
    console.log(message);
    app.quit();
  });

  await win.loadURL(`${scheme}://app/`);
});
