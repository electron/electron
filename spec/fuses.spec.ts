import { flipFuses, FuseV1Options, FuseVersion } from '@electron/fuses';

import { expect } from 'chai';

import { spawn, spawnSync } from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { copyApp } from './lib/fs-helpers.ts';
import { ifdescribe, isTestingBindingAvailable, startRemoteControlApp } from './lib/spec-helpers.ts';

ifdescribe(process.platform === 'win32')('fuses Windows executable configuration', function () {
  this.timeout(120000);

  let tempDir: string;
  let executable: string;

  before(async () => {
    tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'electron-fuses-'));
    executable = await copyApp(tempDir);
  });

  after(() => {
    if (tempDir) fs.rmSync(tempDir, { recursive: true, force: true, maxRetries: 5 });
  });

  it('reads a fuse patched in the executable from the runtime DLL', async () => {
    const options = {
      env: { ...process.env, ELECTRON_RUN_AS_NODE: '1', NODE_OPTIONS: '-e 0' },
      encoding: 'utf8' as const,
      timeout: 30000
    };
    const before = spawnSync(executable, ['-e', 'console.log("ok")'], options);
    expect(before.error).to.equal(undefined);
    expect(before.status, before.stderr).to.equal(9);
    await flipFuses(executable, {
      version: FuseVersion.V1,
      [FuseV1Options.EnableNodeOptionsEnvironmentVariable]: false
    });
    const after = spawnSync(executable, ['-e', 'console.log("ok")'], options);
    expect(after.error).to.equal(undefined);
    expect(after.status, after.stderr).to.equal(0);
    expect(after.stdout.trim()).to.equal('ok');
  });
});

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
});
