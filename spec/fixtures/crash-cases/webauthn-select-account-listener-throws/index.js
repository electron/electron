// A 'select-webauthn-account' listener that invokes the callback and then
// throws must not crash the main process. The throw should surface as an
// ordinary uncaughtException and the assertion should still resolve.
const { app, BrowserWindow } = require('electron');

const http = require('node:http');

const expectedError = new Error('boom');
let caught = null;
process.on('uncaughtException', (err) => {
  caught = err;
});

async function addCredential(w, authenticatorId, id, userHandle, name) {
  const privateKey = await w.webContents.executeJavaScript(`
    (async () => {
      const k = await crypto.subtle.generateKey({ name: 'ECDSA', namedCurve: 'P-256' }, true, ['sign']);
      const pkcs8 = await crypto.subtle.exportKey('pkcs8', k.privateKey);
      return btoa(String.fromCharCode(...new Uint8Array(pkcs8)));
    })()
  `);
  await w.webContents.debugger.sendCommand('WebAuthn.addCredential', {
    authenticatorId,
    credential: {
      credentialId: Buffer.from(id).toString('base64'),
      isResidentCredential: true,
      rpId: 'localhost',
      privateKey,
      userHandle: Buffer.from(userHandle).toString('base64'),
      userName: name,
      userDisplayName: name,
      signCount: 0
    }
  });
}

app
  .whenReady()
  .then(async () => {
    const server = http.createServer((req, res) => {
      res.setHeader('Content-Type', 'text/html');
      res.end('<!doctype html><title>webauthn</title>');
    });
    await new Promise((resolve) => server.listen(0, 'localhost', resolve));

    const w = new BrowserWindow({ show: false });
    await w.loadURL(`http://localhost:${server.address().port}/`);
    w.webContents.debugger.attach();
    await w.webContents.debugger.sendCommand('WebAuthn.enable');
    const { authenticatorId } = await w.webContents.debugger.sendCommand('WebAuthn.addVirtualAuthenticator', {
      options: {
        protocol: 'ctap2',
        transport: 'internal',
        hasResidentKey: true,
        hasUserVerification: true,
        isUserVerified: true,
        automaticPresenceSimulation: true
      }
    });
    await addCredential(w, authenticatorId, 'cred-alice', 'uh-alice', 'alice@example.com');
    await addCredential(w, authenticatorId, 'cred-bob', 'uh-bob', 'bob@example.com');

    w.webContents.session.on('select-webauthn-account', (event, details, callback) => {
      callback(details.accounts[0].credentialId);
      throw expectedError;
    });

    const result = await w.webContents.executeJavaScript(`
    navigator.credentials.get({
      publicKey: { challenge: new Uint8Array(32), rpId: 'localhost', userVerification: 'required' }
    }).then(c => ({ ok: true, id: c.id }), e => ({ ok: false, name: e.name, message: e.message }))
  `);

    if (!result.ok) throw new Error(`assertion failed: ${result.name}: ${result.message}`);
    if (caught !== expectedError) throw new Error(`expected listener error to reach uncaughtException, got: ${caught}`);

    server.close();
    app.quit();
  })
  .catch((err) => {
    console.error(err);
    app.exit(1);
  });
