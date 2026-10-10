import { expect } from 'chai';

import { spawn, spawnSync } from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as https from 'node:https';
import * as os from 'node:os';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { defer, ifdescribe, isTestingBindingAvailable, startRemoteControlApp, waitUntil } from './lib/spec-helpers.ts';

import type * as http from 'node:http';

ifdescribe(isTestingBindingAvailable())('fuses', () => {
  it('can be enabled by command-line argument during testing', async () => {
    const child0 = spawn(process.execPath, ['-v'], { env: { NODE_OPTIONS: '-e 0' } });
    const [code0] = await once(child0, 'exit');
    // Should exit with 9 because -e is not allowed in NODE_OPTIONS
    expect(code0).to.equal(9);
    const child1 = spawn(process.execPath, ['--set-fuse-node_options=0', '-v'], { env: { NODE_OPTIONS: '-e 0' } });
    const [code1] = await once(child1, 'exit');
    // Should print the version and exit with 0
    expect(code1).to.equal(0);
  });

  it('disables --inspect flag when node_cli_inspect is 0', () => {
    const { status, stderr } = spawnSync(process.execPath, ['--set-fuse-node_cli_inspect=0', '--inspect', '-v'], {
      encoding: 'utf-8'
    });
    expect(stderr).to.not.include('Debugger listening on ws://');
    // Should print the version and exit with 0
    expect(status).to.equal(0);
  });

  it('makes child_process.fork throw when run_as_node is 0', async () => {
    const rc = await startRemoteControlApp(['--set-fuse-run_as_node=0']);
    const message = await rc.remotely(
      (fixture: string) => {
        try {
          require('node:child_process').fork(fixture);
          return 'forked';
        } catch (error) {
          return (error as Error).message;
        }
      },
      path.join(import.meta.dirname, 'fixtures', 'module', 'noop.js')
    );
    expect(message).to.include('runAsNode fuse is disabled');
  });

  it('disables fetching file:// URLs when grant_file_protocol_extra_privileges is 0', async () => {
    const rc = await startRemoteControlApp(['--set-fuse-grant_file_protocol_extra_privileges=0']);
    await expect(
      rc.remotely(
        async (fixture: string) => {
          const { BrowserWindow } = require('electron');
          const bw = new BrowserWindow({ show: false });
          await bw.loadFile(fixture);
          return await bw.webContents.executeJavaScript("ajax('file:///etc/passwd')");
        },
        path.join(import.meta.dirname, 'fixtures', 'pages', 'fetch.html')
      )
    ).to.eventually.be.rejectedWith('Failed to fetch');
  });

  describe('cookie_encryption', () => {
    it('allows setting and retrieving cookies when enabled', async () => {
      const rc = await startRemoteControlApp(['--set-fuse-cookie_encryption=1']);
      const result = await rc.remotely(async () => {
        const { session } = require('electron');
        const ses = session.defaultSession;
        const testUrl = 'https://example.com';

        await ses.clearStorageData({ storages: ['cookies'] });

        await ses.cookies.set({
          url: testUrl,
          name: 'test_cookie',
          value: 'encrypted_value_12345',
          expirationDate: Math.floor(Date.now() / 1000) + 3600
        });

        await ses.cookies.set({
          url: testUrl,
          name: 'secure_cookie',
          value: 'secret_data_67890',
          secure: true,
          httpOnly: true,
          expirationDate: Math.floor(Date.now() / 1000) + 7200
        });

        const cookies = await ses.cookies.get({ url: testUrl });
        const testCookie = cookies.find((c: Electron.Cookie) => c.name === 'test_cookie');
        const secureCookie = cookies.find((c: Electron.Cookie) => c.name === 'secure_cookie');

        return {
          cookieCount: cookies.length,
          testCookieValue: testCookie?.value,
          secureCookieValue: secureCookie?.value,
          secureCookieIsSecure: secureCookie?.secure,
          secureCookieIsHttpOnly: secureCookie?.httpOnly
        };
      });

      expect(result.cookieCount).to.equal(2);
      expect(result.testCookieValue).to.equal('encrypted_value_12345');
      expect(result.secureCookieValue).to.equal('secret_data_67890');
      expect(result.secureCookieIsSecure).to.be.true();
      expect(result.secureCookieIsHttpOnly).to.be.true();
    });

    it('persists cookies across sessions when enabled', async () => {
      const rc = await startRemoteControlApp(['--set-fuse-cookie_encryption=1']);

      await rc.remotely(async () => {
        const { session } = require('electron');
        await session.defaultSession.clearStorageData({ storages: ['cookies'] });
        await session.defaultSession.cookies.set({
          url: 'https://example.com',
          name: 'persistent_cookie',
          value: 'persist_me',
          expirationDate: Math.floor(Date.now() / 1000) + 86400
        });
      });

      await rc.remotely(async () => {
        const { session } = require('electron');
        await session.defaultSession.cookies.flushStore();
      });

      const result = await rc.remotely(async () => {
        const { session } = require('electron');
        const cookies = await session.defaultSession.cookies.get({ url: 'https://example.com' });
        const cookie = cookies.find((c: Electron.Cookie) => c.name === 'persistent_cookie');
        return cookie?.value;
      });

      expect(result).to.equal('persist_me');
    });
  });
  describe('device_bound_sessions', () => {
    const softwareKeysFeature = 'EnableBoundSessionCredentialsSoftwareKeysForManualTesting';
    const softwareKeysArg = `--enable-features=${softwareKeysFeature}`;

    // The Device Bound Session Credentials handshake, as much of it as the browser needs: /login asks the browser to
    // register a session, and /register accepts the key it proves possession of. Browsers only act on these headers
    // over a valid TLS connection, even for localhost.
    const certificates = path.join(import.meta.dirname, 'fixtures', 'certificates');
    const startDbscServer = async () => {
      const registrations: http.IncomingHttpHeaders[] = [];
      const tls = {
        key: fs.readFileSync(path.join(certificates, 'server.key')),
        cert: fs.readFileSync(path.join(certificates, 'server.pem'))
      };
      const server = https.createServer(tls, (req, res) => {
        const url = new URL(req.url!, 'http://localhost');
        if (url.pathname === '/login') {
          res.setHeader(
            'Secure-Session-Registration',
            `(ES256 RS256);challenge="challenge_value";path="/register?${url.searchParams}"`
          );
          res.setHeader('Content-Type', 'text/html');
          res.end('<html></html>');
        } else if (url.pathname === '/register') {
          registrations.push(req.headers);
          res.setHeader('Set-Cookie', 'auth_cookie=abcdef0123; SameSite=Strict; Secure');
          res.setHeader('Content-Type', 'application/json');
          res.end(
            JSON.stringify({
              session_identifier: url.searchParams.get('session_id') ?? 'session_id',
              refresh_url: `https://${req.headers.host}/refresh`,
              scope: { include_site: false },
              credentials: [{ type: 'cookie', name: 'auth_cookie', attributes: 'SameSite=Strict; Secure' }],
              allowed_refresh_initiators: ['*']
            })
          );
        } else {
          res.statusCode = 404;
          res.end();
        }
      });
      await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve));
      defer(() => {
        server.closeAllConnections();
        server.close();
      });
      return { origin: `https://localhost:${(server.address() as { port: number }).port}`, registrations };
    };

    // Each app gets its own user data dir so that on-disk state starts clean and can be inspected.
    const startApp = async (args: string[]) => {
      const userData = fs.mkdtempSync(path.join(os.tmpdir(), 'electron-dbsc-'));
      defer(() => fs.rmSync(userData, { recursive: true, force: true }));
      const rc = await startRemoteControlApp([`--user-data-dir=${userData}`, ...args]);
      return { rc, userData };
    };

    type RemoteApp = Awaited<ReturnType<typeof startApp>>['rc'];

    // Records the session events that DevTools reports for the default session, then loads `url` in a window.
    const loadWithSessionEvents = async (rc: RemoteApp, url: string) => {
      await rc.remotely(async (url: string) => {
        const { BrowserWindow, session } = require('electron');
        // The test server's certificate is not issued by a trusted root.
        session.defaultSession.setCertificateVerifyProc((_request: unknown, callback: (result: number) => void) =>
          callback(0)
        );
        const win = new BrowserWindow({ show: false });
        await win.loadURL('about:blank');
        (global as any).dbscEvents = [];
        (global as any).dbscSessionLists = [];
        win.webContents.debugger.attach('1.3');
        win.webContents.debugger.on('message', (_event: unknown, method: string, params: any) => {
          if (method === 'Network.deviceBoundSessionEventOccurred') (global as any).dbscEvents.push(params);
          if (method === 'Network.deviceBoundSessionsAdded') (global as any).dbscSessionLists.push(params.sessions);
        });
        await win.webContents.debugger.sendCommand('Network.enableDeviceBoundSessions', { enable: true });
        (global as any).dbscWindow = win;
        await win.loadURL(url);
      }, url);
    };

    const loadInRecordingWindow = (rc: RemoteApp, url: string) =>
      rc.remotely((url: string) => (global as any).dbscWindow.loadURL(url), url);

    const createdSessionIds = async (rc: RemoteApp) => {
      const events: any[] = await rc.remotely(() => (global as any).dbscEvents);
      return events.filter((e) => e.succeeded && e.creationEventDetails).map((e) => e.sessionId);
    };

    // The ids of the sessions the network service holds. Turning the DevTools events off and on again makes it replay
    // its current set of sessions.
    const listSessionIds = (rc: RemoteApp): Promise<string[]> =>
      rc.remotely(async () => {
        const g = global as any;
        const { debugger: cdp } = g.dbscWindow.webContents;
        const listsBefore = g.dbscSessionLists.length;
        await cdp.sendCommand('Network.enableDeviceBoundSessions', { enable: false });
        await cdp.sendCommand('Network.enableDeviceBoundSessions', { enable: true });
        while (g.dbscSessionLists.length === listsBefore) await setTimeout(10);
        return g.dbscSessionLists[g.dbscSessionLists.length - 1].map((session: any) => session.key.id);
      });

    // How many times the browser process has signed for a session. The network service has a key service of its own that
    // it falls back to when the browser does not provide one, and which registers sessions just as well with software
    // keys, so a session alone does not show that the browser's key service is the one in use. Histograms are per
    // process, so this only counts the work that the browser process did.
    const browserSignCount = (rc: RemoteApp): Promise<number> =>
      rc.remotely(() =>
        process
          ._linkedBinding('electron_common_testing')
          .getHistogramTotalCount('Crypto.UnexportableKeys.BackgroundTaskResult.DeviceBoundSessions.Sign')
      );

    // How long to wait before concluding that a registration is not going to happen.
    const quietPeriod = 2000;

    it('registers a session with software keys when the fuse is off', async () => {
      const server = await startDbscServer();
      const { rc } = await startApp([softwareKeysArg]);
      await loadWithSessionEvents(rc, `${server.origin}/login?session_id=first`);

      await waitUntil(() => server.registrations.length > 0);
      expect(server.registrations[0]).to.have.property('secure-session-response');
      await waitUntil(async () => (await createdSessionIds(rc)).includes('first'));
      expect(await browserSignCount(rc)).to.be.greaterThan(0);
    });

    it('rebinds its key service when the network service restarts', async () => {
      const server = await startDbscServer();
      const { rc } = await startApp([softwareKeysArg]);
      await loadWithSessionEvents(rc, `${server.origin}/login?session_id=before`);
      await waitUntil(async () => (await createdSessionIds(rc)).includes('before'));
      const signedBefore = await browserSignCount(rc);
      expect(signedBefore).to.be.greaterThan(0);

      await rc.remotely(() => process._linkedBinding('electron_common_testing').simulateNetworkServiceCrash());
      // Give the storage partition time to recreate its network context in the new network service.
      await setTimeout(500);
      // The certificate verifier and the DevTools observer belonged to the old network context, so set them up again.
      await rc.remotely(async () => {
        const { session } = require('electron');
        session.defaultSession.setCertificateVerifyProc((_request: unknown, callback: (result: number) => void) =>
          callback(0)
        );
        const { debugger: cdp } = (global as any).dbscWindow.webContents;
        await cdp.sendCommand('Network.enableDeviceBoundSessions', { enable: false });
        await cdp.sendCommand('Network.enableDeviceBoundSessions', { enable: true });
      });
      await loadInRecordingWindow(rc, `${server.origin}/login?session_id=after`);

      await waitUntil(async () => (await createdSessionIds(rc)).includes('after'));
      // The new network service signed through the browser's key service too.
      expect(await browserSignCount(rc)).to.be.greaterThan(signedBefore);
    });

    describe('clearing data', () => {
      const clearCookies = (rc: RemoteApp, api: 'clearStorageData' | 'clearData') =>
        rc.remotely(async (api: string) => {
          const { session } = require('electron');
          if (api === 'clearStorageData') await session.defaultSession.clearStorageData({ storages: ['cookies'] });
          else await session.defaultSession.clearData({ dataTypes: ['cookies'] });
        }, api);

      const registerSession = async (id: string) => {
        const server = await startDbscServer();
        const { rc } = await startApp([softwareKeysArg]);
        await loadWithSessionEvents(rc, `${server.origin}/login?session_id=${id}`);
        await waitUntil(async () => (await createdSessionIds(rc)).includes(id));
        expect(await listSessionIds(rc)).to.include(id);
        return rc;
      };

      // A session refreshes the cookie it is bound to, so a session that outlives a "clear cookies" would quietly undo
      // an app's logout.
      for (const api of ['clearStorageData', 'clearData'] as const) {
        it(`ends sessions when ${api} clears cookies`, async () => {
          const rc = await registerSession('doomed');
          await clearCookies(rc, api);
          expect(await listSessionIds(rc)).to.not.include('doomed');
        });
      }

      it('keeps sessions when only other storage is cleared', async () => {
        const rc = await registerSession('kept');
        await rc.remotely(() =>
          require('electron').session.defaultSession.clearStorageData({ storages: ['localstorage'] })
        );
        expect(await listSessionIds(rc)).to.include('kept');
      });
    });

    it('does not register sessions when the fuse is off and software keys are not requested', async () => {
      const server = await startDbscServer();
      const { rc } = await startApp([]);
      await loadWithSessionEvents(rc, `${server.origin}/login`);

      await setTimeout(quietPeriod);
      expect(server.registrations).to.be.empty();
      expect(await createdSessionIds(rc)).to.be.empty();
    });

    it('stores sessions on disk only when DBSC is enabled', async () => {
      const server = await startDbscServer();
      // The network context keeps its databases in the user data directory, or in a Network subdirectory of it when
      // the network service is sandboxed. The session database goes wherever the cookie database does.
      const storageDirectory = (userData: string) =>
        [userData, path.join(userData, 'Network')].find((dir) => fs.existsSync(path.join(dir, 'Cookies')));
      const settleNetworkContext = (rc: RemoteApp) =>
        rc.remotely(async () => {
          const { session } = require('electron');
          await session.defaultSession.cookies.get({});
          await session.defaultSession.cookies.flushStore();
        });

      const enabled = await startApp([softwareKeysArg]);
      await loadWithSessionEvents(enabled.rc, `${server.origin}/login`);
      await settleNetworkContext(enabled.rc);
      await waitUntil(() => {
        const dir = storageDirectory(enabled.userData);
        return dir !== undefined && fs.existsSync(path.join(dir, 'Device Bound Sessions'));
      });

      const disabled = await startApp([]);
      await loadWithSessionEvents(disabled.rc, `${server.origin}/login`);
      await settleNetworkContext(disabled.rc);
      // The cookie database exists by now, so the session database is really missing rather than not written yet.
      const dir = storageDirectory(disabled.userData);
      expect(dir).to.be.a('string');
      expect(fs.existsSync(path.join(dir!, 'Device Bound Sessions'))).to.be.false();
    });

    const isSoftwareKeysEnabled = (rc: RemoteApp) =>
      rc.remotely(() => process._linkedBinding('electron_common_testing').isBoundSessionSoftwareKeysEnabled());

    it('enables software keys from the flag when the fuse is off', async () => {
      // The positive control for the assertions about the fuse below.
      const { rc } = await startApp([softwareKeysArg]);
      expect(await isSoftwareKeysEnabled(rc)).to.be.true();
    });

    describe('when the fuse is on', () => {
      it('ignores the flag that enables software keys', async () => {
        const { rc } = await startApp(['--set-fuse-device_bound_sessions=1', softwareKeysArg]);
        expect(await isSoftwareKeysEnabled(rc)).to.be.false();
      });

      it('ignores the flag when it comes with a field trial group or other features', async () => {
        const { rc } = await startApp([
          '--set-fuse-device_bound_sessions=1',
          `--enable-features=NetworkQualityEstimator,${softwareKeysFeature}<SomeStudy`
        ]);
        expect(await isSoftwareKeysEnabled(rc)).to.be.false();
      });

      it('does not rewrite the process command line', async () => {
        const { rc } = await startApp([
          '--set-fuse-device_bound_sessions=1',
          `--enable-features=NetworkQualityEstimator,${softwareKeysFeature}`,
          '--disable-features=SomeOtherFeature'
        ]);
        const switches = await rc.remotely(() => {
          const { app } = require('electron');
          return {
            enable: app.commandLine.getSwitchValue('enable-features'),
            disable: app.commandLine.getSwitchValue('disable-features')
          };
        });
        expect(switches.enable).to.equal(`NetworkQualityEstimator,${softwareKeysFeature}`);
        expect(switches.disable).to.equal('SomeOtherFeature');
      });

      it('adds no command line switches of its own', async () => {
        const { rc } = await startApp(['--set-fuse-device_bound_sessions=1']);
        const hasSwitch = await rc.remotely(() => {
          const { app } = require('electron');
          return app.commandLine.hasSwitch('disable-features') || app.commandLine.hasSwitch('enable-features');
        });
        expect(hasSwitch).to.be.false();
      });
    });
  });
});
