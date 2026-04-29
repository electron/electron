import { app, BrowserWindow, session } from 'electron/main';

import { expect } from 'chai';

import * as http from 'node:http';
import { AddressInfo } from 'node:net';

import { ifdescribe } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

const configureWebAuthn = (app as any).configureWebAuthn?.bind(app) as (options?: unknown) => void;

ifdescribe(process.platform === 'darwin')('app.configureWebAuthn', () => {
  it('throws when called without an options object', () => {
    expect(() => configureWebAuthn()).to.throw(/configureWebAuthn requires an options object/);
  });

  it('throws when touchID.keychainAccessGroup is missing', () => {
    expect(() => configureWebAuthn({ touchID: {} })).to.throw(/keychainAccessGroup/);
  });

  it('throws when touchID.keychainAccessGroup is empty', () => {
    expect(() => configureWebAuthn({ touchID: { keychainAccessGroup: '' } })).to.throw(/keychainAccessGroup/);
  });

  it('accepts a valid touchID configuration', () => {
    expect(() =>
      configureWebAuthn({
        touchID: { keychainAccessGroup: 'TESTTEAMID.org.electron.spec.webauthn' }
      })
    ).to.not.throw();
  });
});

describe("session 'select-webauthn-account' event", () => {
  let server: http.Server;
  let serverUrl: string;
  let w: BrowserWindow;
  let authenticatorId: string;

  before(async () => {
    server = http.createServer((req, res) => {
      res.setHeader('Content-Type', 'text/html');
      res.end('<!doctype html><title>webauthn</title>');
    });
    await new Promise<void>((resolve) => server.listen(0, 'localhost', resolve));
    const { port } = server.address() as AddressInfo;
    serverUrl = `http://localhost:${port}/`;
  });

  after(() => {
    server.close();
  });

  beforeEach(async () => {
    w = new BrowserWindow({ show: false });
    await w.loadURL(serverUrl);
    w.webContents.debugger.attach();
    await w.webContents.debugger.sendCommand('WebAuthn.enable');
    const result = await w.webContents.debugger.sendCommand('WebAuthn.addVirtualAuthenticator', {
      options: {
        protocol: 'ctap2',
        transport: 'internal',
        hasResidentKey: true,
        hasUserVerification: true,
        isUserVerified: true,
        automaticPresenceSimulation: true
      }
    });
    authenticatorId = result.authenticatorId;
  });

  afterEach(async () => {
    session.defaultSession.removeAllListeners('select-webauthn-account');
    try {
      w.webContents.debugger.detach();
    } catch {}
    await closeAllWindows();
  });

  async function addCredential(opts: { id: string; userHandle: string; name: string; displayName: string }) {
    const privateKey = await w.webContents.executeJavaScript(`
      (async () => {
        const k = await crypto.subtle.generateKey(
          { name: 'ECDSA', namedCurve: 'P-256' }, true, ['sign']);
        const pkcs8 = await crypto.subtle.exportKey('pkcs8', k.privateKey);
        return btoa(String.fromCharCode(...new Uint8Array(pkcs8)));
      })()
    `);
    await w.webContents.debugger.sendCommand('WebAuthn.addCredential', {
      authenticatorId,
      credential: {
        credentialId: Buffer.from(opts.id).toString('base64'),
        isResidentCredential: true,
        rpId: 'localhost',
        privateKey,
        userHandle: Buffer.from(opts.userHandle).toString('base64'),
        userName: opts.name,
        userDisplayName: opts.displayName,
        signCount: 0
      }
    });
  }

  function getAssertion() {
    return w.webContents.executeJavaScript(`
      navigator.credentials.get({
        publicKey: {
          challenge: new Uint8Array(32),
          rpId: 'localhost',
          userVerification: 'required'
        }
      }).then(
        c => {
          const uh = c.response.userHandle;
          const userHandle = uh
            ? btoa(String.fromCharCode(...new Uint8Array(uh)))
                .replace(/\\+/g, '-').replace(/\\//g, '_').replace(/=+$/, '')
            : null;
          return { ok: true, id: c.id, userHandle };
        },
        e => ({ ok: false, name: e.name, message: e.message })
      )
    `);
  }

  // Regression test: MakeCredentialRequestHandler::SpecializeRequestForAuthenticator
  // dereferences observer() for ResidentKeyRequirement::kPreferred, which crashed
  // when the request delegate did not register itself as the handler's observer.
  it('does not crash on navigator.credentials.create() with residentKey "preferred"', async () => {
    const result = await w.webContents.executeJavaScript(`
      navigator.credentials.create({
        publicKey: {
          rp: { id: 'localhost', name: 'Electron Spec' },
          user: {
            id: new TextEncoder().encode('user-1'),
            name: 'alice@example.com',
            displayName: 'Alice'
          },
          challenge: new Uint8Array(32),
          pubKeyCredParams: [{ type: 'public-key', alg: -7 }],
          authenticatorSelection: {
            residentKey: 'preferred',
            userVerification: 'preferred'
          }
        }
      }).then(
        c => ({ ok: true, id: c.id }),
        e => ({ ok: false, name: e.name, message: e.message })
      )
    `);
    expect(result.ok).to.be.true();
    expect(result.id).to.be.a('string').and.not.be.empty();
  });

  // Pick byte sequences that exercise the URL-safe base64 alphabet — '?' and
  // '>' both produce '+' / '/' / padding in standard base64, so a regression
  // back to base::Base64Encode would surface here as a mismatch between the
  // event's credentialId/userHandle and the values the renderer sees.
  const BOB_ID = 'cred-bob??';
  const BOB_USER_HANDLE = 'uh-bob>>';

  const toBase64Url = (s: string) =>
    Buffer.from(s)
      .toString('base64')
      .replace(/\+/g, '-')
      .replace(/\//g, '_')
      .replace(/=+$/, '');

  it('fires with discoverable credentials and resolves get() with the chosen one', async () => {
    await addCredential({ id: 'cred-alice', userHandle: 'uh-alice', name: 'alice@example.com', displayName: 'Alice' });
    await addCredential({ id: BOB_ID, userHandle: BOB_USER_HANDLE, name: 'bob@example.com', displayName: 'Bob' });

    let received: any;
    (w.webContents.session as NodeJS.EventEmitter).on('select-webauthn-account', (event, details, callback) => {
      received = details;
      const bob = details.accounts.find((a: any) => a.name === 'bob@example.com');
      callback(bob.credentialId);
    });

    const result = await getAssertion();

    expect(received).to.exist();
    expect(received.relyingPartyId).to.equal('localhost');
    expect(received.accounts).to.have.lengthOf(2);
    const names = received.accounts.map((a: any) => a.name).sort();
    expect(names).to.deep.equal(['alice@example.com', 'bob@example.com']);
    const bob = received.accounts.find((a: any) => a.name === 'bob@example.com');
    expect(bob.displayName).to.equal('Bob');
    expect(bob.credentialId).to.be.a('string').and.not.be.empty();

    // Both credentialId and userHandle must be URL-safe base64 (no '+', '/'
    // or padding) so the values are byte-for-byte comparable to what the
    // WebAuthn JS API exposes on PublicKeyCredential.
    expect(bob.credentialId).to.equal(toBase64Url(BOB_ID));
    expect(bob.userHandle).to.equal(toBase64Url(BOB_USER_HANDLE));
    expect(bob.credentialId).to.not.match(/[+/=]/);
    expect(bob.userHandle).to.not.match(/[+/=]/);

    // The strong invariant: the credentialId surfaced via the main-process
    // event is the same string the renderer sees as PublicKeyCredential.id.
    expect(result.ok).to.be.true();
    expect(result.id).to.equal(bob.credentialId);
    expect(result.userHandle).to.equal(bob.userHandle);
  });

  it('cancels the request when the callback is invoked with an unknown credentialId', async () => {
    await addCredential({ id: 'cred-alice', userHandle: 'uh-alice', name: 'alice@example.com', displayName: 'Alice' });
    await addCredential({ id: 'cred-bob', userHandle: 'uh-bob', name: 'bob@example.com', displayName: 'Bob' });

    (w.webContents.session as NodeJS.EventEmitter).on('select-webauthn-account', (event, details, callback) => {
      callback('not-a-real-credential-id');
    });

    const result = await getAssertion();
    expect(result.ok).to.be.false();
    expect(result.name).to.equal('NotAllowedError');
  });

  it('cancels the request when the callback is invoked with no credential', async () => {
    await addCredential({ id: 'cred-alice', userHandle: 'uh-alice', name: 'alice@example.com', displayName: 'Alice' });
    await addCredential({ id: 'cred-bob', userHandle: 'uh-bob', name: 'bob@example.com', displayName: 'Bob' });

    (w.webContents.session as NodeJS.EventEmitter).on('select-webauthn-account', (event, details, callback) => {
      callback();
    });

    const result = await getAssertion();
    expect(result.ok).to.be.false();
    expect(result.name).to.equal('NotAllowedError');
  });

  it('cancels the request when no listener is registered', async () => {
    await addCredential({ id: 'cred-alice', userHandle: 'uh-alice', name: 'alice@example.com', displayName: 'Alice' });
    await addCredential({ id: 'cred-bob', userHandle: 'uh-bob', name: 'bob@example.com', displayName: 'Bob' });

    expect(w.webContents.session.listenerCount('select-webauthn-account')).to.equal(0);

    const result = await getAssertion();
    expect(result.ok).to.be.false();
    expect(result.name).to.equal('NotAllowedError');
  });
});
