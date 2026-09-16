// net.fetch() from the main process must work when an extension with the
// webRequest permission is loaded and no session.webRequest listeners are
// registered.
const { app, net, session } = require('electron');

const http = require('node:http');
const { once } = require('node:events');
const path = require('node:path');

app.whenReady().then(async () => {
  const server = http.createServer((req, res) => res.end('ok'));
  await once(server.listen(0, '127.0.0.1'), 'listening');
  const url = `http://127.0.0.1:${server.address().port}/`;

  await session.defaultSession.extensions.loadExtension(path.join(__dirname, 'extension'));
  const res = await net.fetch(url);
  if ((await res.text()) !== 'ok') process.exitCode = 1;
  const fileRes = await net.fetch(require('node:url').pathToFileURL(__filename).toString());
  if (!(await fileRes.text()).includes('webRequest permission')) process.exitCode = 1;
  app.quit();
});
