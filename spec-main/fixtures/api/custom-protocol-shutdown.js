const { app, webContents, protocol, session } = require('electron');

protocol.registerSchemesAsPrivileged([
  { scheme: 'test', privileges: { standard: true, secure: true } }
]);

app.whenReady().then(function () {
  const ses = session.fromPartition('persist:test-standard-shutdown');
  const web = webContents.create({ session: ses });

  ses.protocol.registerStringProtocol('test', (request, callback) => {
    callback('Hello World!');
  });

  web.loadURL('test://abc/hello.txt');

  web.on('did-finish-load', () => app.quit());
});
