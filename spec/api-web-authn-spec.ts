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

  it('throws when touchID.promptReason is empty', () => {
    expect(() =>
      configureWebAuthn({
        touchID: {
          keychainAccessGroup: 'TESTTEAMID.org.electron.spec.webauthn',
          promptReason: ''
        }
      })
    ).to.throw(/promptReason/);
  });

  it('accepts a touchID.promptReason with a $1 placeholder', () => {
    expect(() =>
      configureWebAuthn({
        touchID: {
          keychainAccessGroup: 'TESTTEAMID.org.electron.spec.webauthn',
          promptReason: 'sign in to $1'
        }
      })
    ).to.not.throw();
  });

  it('accepts a touchID.promptReason without a placeholder', () => {
    expect(() =>
      configureWebAuthn({
        touchID: {
          keychainAccessGroup: 'TESTTEAMID.org.electron.spec.webauthn',
          promptReason: 'sign in'
        }
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
          const userHandle = c.response.userHandle
            ? new Uint8Array(c.response.userHandle)
                .toBase64({ alphabet: 'base64url', omitPadding: true })
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

  const toBase64Url = (s: string) => Buffer.from(s).toString('base64url');

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

// CDP virtual authenticators implement no CTAP client PIN, so the
// 'enter-webauthn-pin' path needs a physical PIN-protected key to exercise.
describe('session WebAuthn ceremony events', () => {
  const CEREMONY_EVENTS = [
    'webauthn-request-started',
    'webauthn-request-touch-needed',
    'webauthn-request-completed',
    'enter-webauthn-pin'
  ];

  let server: http.Server;
  let serverUrl: string;
  let w: BrowserWindow;

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
    // Without enableUI the virtual authenticator environment calls
    // DisableUI() and ceremony events are (correctly) suppressed.
    await w.webContents.debugger.sendCommand('WebAuthn.enable', { enableUI: true });
    await w.webContents.debugger.sendCommand('WebAuthn.addVirtualAuthenticator', {
      options: {
        protocol: 'ctap2',
        transport: 'usb',
        hasResidentKey: false,
        hasUserVerification: false,
        automaticPresenceSimulation: true
      }
    });
  });

  afterEach(async () => {
    for (const name of CEREMONY_EVENTS) {
      session.defaultSession.removeAllListeners(name);
    }
    try {
      w.webContents.debugger.detach();
    } catch {}
    await closeAllWindows();
  });

  function recordEvents() {
    const events: { name: string; details: any }[] = [];
    for (const name of CEREMONY_EVENTS) {
      (w.webContents.session as NodeJS.EventEmitter).on(name, (event, details) => {
        events.push({ name, details });
      });
    }
    return events;
  }

  function createCredential(opts: { excludePrevious?: boolean } = {}) {
    return w.webContents.executeJavaScript(`
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
          excludeCredentials: ${opts.excludePrevious ? 'window.__excludeList' : '[]'},
          authenticatorSelection: { userVerification: 'discouraged' }
        }
      }).then(
        c => {
          window.__excludeList = [{ type: 'public-key', id: c.rawId }];
          window.__lastCredentialId = new Uint8Array(c.rawId);
          return { ok: true, id: c.id };
        },
        e => ({ ok: false, name: e.name, message: e.message })
      )
    `);
  }

  it('emits webauthn-request-started and a successful webauthn-request-completed for create()', async () => {
    const events = recordEvents();

    const result = await createCredential();
    expect(result.ok).to.be.true();

    const started = events.filter((e) => e.name === 'webauthn-request-started');
    expect(started).to.have.lengthOf(1);
    expect(started[0].details.relyingPartyId).to.equal('localhost');
    expect(started[0].details.requestType).to.equal('create');
    expect(started[0].details.transports).to.be.an('array').that.includes('usb');
    expect(started[0].details.userVerification).to.equal('discouraged');
    expect(started[0].details.frame).to.not.be.null();

    const completed = events.filter((e) => e.name === 'webauthn-request-completed');
    expect(completed).to.have.lengthOf(1);
    expect(completed[0].details.relyingPartyId).to.equal('localhost');
    expect(completed[0].details.success).to.be.true();
    expect(completed[0].details.reason).to.be.undefined();

    // started must precede completed
    expect(events.findIndex((e) => e.name === 'webauthn-request-started')).to.be.lessThan(
      events.findIndex((e) => e.name === 'webauthn-request-completed')
    );
  });

  it('emits ceremony events for get() with requestType "get"', async () => {
    expect((await createCredential()).ok).to.be.true();

    const events = recordEvents();
    const result = await w.webContents.executeJavaScript(`
      navigator.credentials.get({
        publicKey: {
          challenge: new Uint8Array(32),
          rpId: 'localhost',
          userVerification: 'discouraged',
          allowCredentials: [{ type: 'public-key', id: window.__lastCredentialId }]
        }
      }).then(
        c => ({ ok: true, id: c.id }),
        e => ({ ok: false, name: e.name, message: e.message })
      )
    `);
    expect(result.ok).to.be.true();

    const started = events.filter((e) => e.name === 'webauthn-request-started');
    expect(started).to.have.lengthOf(1);
    expect(started[0].details.requestType).to.equal('get');

    const completed = events.filter((e) => e.name === 'webauthn-request-completed');
    expect(completed).to.have.lengthOf(1);
    expect(completed[0].details.success).to.be.true();
  });

  it('reports "key-already-registered" when create() excludes an existing credential', async () => {
    expect((await createCredential()).ok).to.be.true();

    const events = recordEvents();
    const result = await createCredential({ excludePrevious: true });
    expect(result.ok).to.be.false();
    expect(result.name).to.equal('InvalidStateError');

    const completed = events.filter((e) => e.name === 'webauthn-request-completed');
    expect(completed).to.have.lengthOf(1);
    expect(completed[0].details.success).to.be.false();
    expect(completed[0].details.reason).to.equal('key-already-registered');
  });

  it('does not interfere with requests when no ceremony listeners are registered', async () => {
    const result = await createCredential();
    expect(result.ok).to.be.true();
  });
});
