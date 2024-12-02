const { BrowserWindow, app, protocol, net, session } = require('electron');

const { once } = require('node:events');
const path = require('node:path');
const { pathToFileURL } = require('node:url');

if (process.argv.length < 4) {
  console.error('Must pass allow_code_cache code_cache_dir');
  process.exit(1);
}

protocol.registerSchemesAsPrivileged([
  {
    scheme: 'atom',
    privileges: {
      standard: true,
      codeCache: process.argv[2] === 'true'
    }
  }
]);

app.once('ready', async () => {
  const codeCachePath = process.argv[3];
  session.defaultSession.setCodeCachePath(codeCachePath);

  protocol.handle('atom', (request) => {
    let { pathname } = new URL(request.url);
    if (pathname === '/mocha.js') { pathname = path.resolve(__dirname, '../../../node_modules/mocha/mocha.js'); } else { pathname = path.join(__dirname, pathname); }
    return net.fetch(pathToFileURL(pathname).toString());
  });

  const win = new BrowserWindow({ show: false });
  win.loadURL('atom://host/main.html');
  await once(win.webContents, 'did-finish-load');
  // Reload to generate code cache.
  win.reload();
  await once(win.webContents, 'did-finish-load');
  app.exit();
});
