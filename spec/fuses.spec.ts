import { describe, expect, it } from 'vitest';

import { spawn, spawnSync } from 'node:child_process';
import { once } from 'node:events';
import * as path from 'node:path';

import { defer, ifdescribe, isTestingBindingAvailable, startRemoteControlApp } from './lib/spec-helpers.ts';

const noopApp = path.join(import.meta.dirname, 'fixtures', 'module', 'noop.js');

// Runs an app that exits immediately with --inspect=0 and reports whether the
// inspector started.
async function runWithInspect(args: string[], env: NodeJS.ProcessEnv = {}) {
  const child = spawn(process.execPath, [...args, noopApp, '--inspect=0'], {
    env: { ...process.env, ...env }
  });
  defer(() => {
    if (child.exitCode === null && child.signalCode === null) child.kill();
  });
  let output = '';
  child.stdout.on('data', (data) => {
    output += data;
  });
  child.stderr.on('data', (data) => {
    output += data;
  });
  const [code] = await once(child, 'close');
  return { code, inspectorStarted: output.includes('Debugger listening on ws://') };
}

ifdescribe(isTestingBindingAvailable())('fuses', () => {
  it('can be enabled by command-line argument during testing', async () => {
    const child0 = spawn(process.execPath, ['-v'], { env: { NODE_OPTIONS: '-e 0' } });
    const [code0] = await once(child0, 'exit');
    // NODE_OPTIONS is ignored because node_options is disabled by default, so
    // this should print the version and exit with 0
    expect(code0).toBe(0);
    const child1 = spawn(process.execPath, ['--set-fuse-node_options=1', '-v'], { env: { NODE_OPTIONS: '-e 0' } });
    const [code1] = await once(child1, 'exit');
    // Should exit with 9 because -e is not allowed in NODE_OPTIONS
    expect(code1).toBe(9);
  });

  it('enables --inspect by default when the app is not packaged', async () => {
    const { code, inspectorStarted } = await runWithInspect([]);
    expect(code).toBe(0);
    expect(inspectorStarted).toBe(true);
  });

  it('disables --inspect by default when the app is packaged', async () => {
    const { code, inspectorStarted } = await runWithInspect([], { ELECTRON_FORCE_IS_PACKAGED: 'true' });
    expect(code).toBe(0);
    expect(inspectorStarted).toBe(false);
  });

  it('enables --inspect only when packaged when node_cli_inspect is p', async () => {
    const unpackaged = await runWithInspect(['--set-fuse-node_cli_inspect=p']);
    expect(unpackaged.inspectorStarted).toBe(false);
    const packaged = await runWithInspect(['--set-fuse-node_cli_inspect=p'], { ELECTRON_FORCE_IS_PACKAGED: 'true' });
    expect(packaged.inspectorStarted).toBe(true);
  });

  it('enables --inspect when packaged when node_cli_inspect is 1', async () => {
    const { inspectorStarted } = await runWithInspect(['--set-fuse-node_cli_inspect=1'], {
      ELECTRON_FORCE_IS_PACKAGED: 'true'
    });
    expect(inspectorStarted).toBe(true);
  });

  it('disables --inspect flag when node_cli_inspect is 0', () => {
    const { status, stderr } = spawnSync(process.execPath, ['--set-fuse-node_cli_inspect=0', '--inspect', '-v'], {
      encoding: 'utf-8'
    });
    expect(stderr).not.toContain('Debugger listening on ws://');
    // Should print the version and exit with 0
    expect(status).toBe(0);
  });

  for (const [description, args] of [
    ['by default', []],
    ['when run_as_node is 0', ['--set-fuse-run_as_node=0']]
  ] as const) {
    it(`makes child_process.fork throw ${description}`, async () => {
      const rc = await startRemoteControlApp([...args]);
      const message = await rc.remotely((fixture: string) => {
        try {
          require('node:child_process').fork(fixture);
          return 'forked';
        } catch (error) {
          return (error as Error).message;
        }
      }, noopApp);
      expect(message).toContain('runAsNode fuse is disabled');
    });
  }

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
    ).rejects.toThrow('Failed to fetch');
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

      expect(result.cookieCount).toBe(2);
      expect(result.testCookieValue).toBe('encrypted_value_12345');
      expect(result.secureCookieValue).toBe('secret_data_67890');
      expect(result.secureCookieIsSecure).toBe(true);
      expect(result.secureCookieIsHttpOnly).toBe(true);
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

      expect(result).toBe('persist_me');
    });
  });
});
