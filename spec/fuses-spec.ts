import { BrowserWindow } from 'electron';

import { expect } from 'chai';

import { spawn, spawnSync } from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as os from 'node:os';
import path = require('node:path');
import { setTimeout } from 'node:timers/promises';

import { defer, startRemoteControlApp } from './lib/spec-helpers';

type RemoteControlApp = Awaited<ReturnType<typeof startRemoteControlApp>>;

describe('fuses', () => {
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

  it('disables fetching file:// URLs when grant_file_protocol_extra_privileges is 0', async () => {
    const rc = await startRemoteControlApp(['--set-fuse-grant_file_protocol_extra_privileges=0']);
    await expect(
      rc.remotely(
        async (fixture: string) => {
          const bw = new BrowserWindow({ show: false });
          await bw.loadFile(fixture);
          return await bw.webContents.executeJavaScript("ajax('file:///etc/passwd')");
        },
        path.join(__dirname, 'fixtures', 'pages', 'fetch.html')
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

    // Each app gets its own user data dir so the on-disk assertions below start
    // from a clean state.
    const startApp = async (extraArgs: string[] = []) => {
      const userDataDir = fs.mkdtempSync(path.join(os.tmpdir(), 'electron-dbsc-'));
      const rc = await startRemoteControlApp([`--user-data-dir=${userDataDir}`, ...extraArgs]);
      // defer() is LIFO, so this runs before startRemoteControlApp's own kill
      // handler: stop the app here and wait for it to exit before deleting the
      // directory, since on Windows the files stay locked until it does. The
      // later kill is then a no-op on an already-dead process.
      defer(async () => {
        // Only kill a child that is still running: for one that already died
        // (a crash, or a failure earlier in the test) 'exit' has fired and
        // once() would never settle, hanging until the mocha timeout and
        // masking the real failure. A ChildProcess keeps its .pid after
        // exiting, unlike UtilityProcess, so check the exit state instead.
        if (rc.process.exitCode === null && rc.process.signalCode === null) {
          const exit = once(rc.process, 'exit');
          rc.process.kill('SIGINT');
          await exit.catch(() => {});
        }
        fs.rmSync(userDataDir, { recursive: true, force: true, maxRetries: 5, retryDelay: 100 });
      });
      // The DBSC store lives directly under userData unless the sandboxed-data
      // migration has run, which only happens on Windows, so check both.
      return {
        rc,
        sessionDbPaths: [
          path.join(userDataDir, 'Device Bound Sessions'),
          path.join(userDataDir, 'Network', 'Device Bound Sessions')
        ]
      };
    };

    // Feature switches reach child processes from the browser's FeatureList
    // instance rather than from the browser's own argv, so a renderer's argv
    // reports the effective feature state.
    const getEffectiveFeatures = (rc: RemoteControlApp) => rc.remotely(async (fixture: string) => {
      const bw = new BrowserWindow({
        show: false,
        webPreferences: { nodeIntegration: true, contextIsolation: false, sandbox: false }
      });
      await bw.loadFile(fixture);
      const argv: string[] = await bw.webContents.executeJavaScript('process.argv');
      bw.destroy();
      const valueOf = (name: string) => {
        const arg = argv.find((a: string) => a.startsWith(`--${name}=`));
        return arg ? arg.slice(name.length + 3) : '';
      };
      return { enabled: valueOf('enable-features'), disabled: valueOf('disable-features') };
    }, path.join(__dirname, 'fixtures', 'pages', 'blank.html'));

    // The session database is only created once the network context has a key
    // service, so its presence means DBSC is actually running and not just
    // configured.
    const hasSessionDatabase = async (rc: RemoteControlApp, sessionDbPaths: string[]) => {
      // Force the network context to be created.
      await rc.remotely(async () => {
        const { session } = require('electron');
        await session.defaultSession.cookies.get({});
      });
      // The store opens its database on a background sequence.
      for (let i = 0; i < 50; i++) {
        if (sessionDbPaths.some((p) => fs.existsSync(p))) return true;
        await setTimeout(100);
      }
      return false;
    };

    it('is off by default', async () => {
      const { rc, sessionDbPaths } = await startApp();
      expect(await hasSessionDatabase(rc, sessionDbPaths)).to.be.false('DBSC database should not exist');
    });

    it('is enabled by the software keys testing feature when the fuse is off', async () => {
      const { rc, sessionDbPaths } = await startApp([`--enable-features=${softwareKeysFeature}`]);

      const features = await getEffectiveFeatures(rc);
      expect(features.enabled).to.include(softwareKeysFeature);
      expect(features.disabled).to.not.include(softwareKeysFeature);

      expect(await hasSessionDatabase(rc, sessionDbPaths)).to.be.true('DBSC database should exist');
    });

    it('rejects the software keys testing feature when the fuse is enabled', async () => {
      const { rc } = await startApp([
        '--set-fuse-device_bound_sessions=1',
        `--enable-features=${softwareKeysFeature},SomeOtherFeature`
      ]);

      const features = await getEffectiveFeatures(rc);
      expect(features.enabled).to.not.include(softwareKeysFeature);
      expect(features.enabled).to.include('SomeOtherFeature');
      expect(features.disabled).to.include(softwareKeysFeature);
    });

    it('does not rewrite the process command line when the fuse is enabled', async () => {
      const { rc } = await startApp(['--set-fuse-device_bound_sessions=1']);
      const result = await rc.remotely(() => {
        const { app } = require('electron');
        return app.commandLine.getSwitchValue('disable-features');
      });
      // InitializeFeatureList() runs twice in the browser process, so rewriting
      // the process command line there appended the name once per run and leaked
      // it into app.commandLine and into second-instance argv.
      expect(result).to.not.include(softwareKeysFeature);
    });
  });
});
