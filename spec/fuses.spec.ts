import { flipFuses, type FuseV1Config, type FuseV1Options, FuseVersion, getCurrentFuseWire } from '@electron/fuses';

import { afterAll, beforeAll, describe, expect, it } from 'vitest';

import { spawn, spawnSync } from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as originalFs from 'node:original-fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { copyApp } from './lib/fs-helpers.ts';
import { defer, ifdescribe, isTestingBindingAvailable, startRemoteControlApp } from './lib/spec-helpers.ts';

const noopApp = path.join(import.meta.dirname, 'fixtures', 'module', 'noop.js');

// The fuse wire release builds ship, from build/fuses/fuses.json5. Testing
// builds override some of these defaults (see electron_fuse_overrides in
// build/args/testing.gn).
const releaseFuseWire = (() => {
  const json5 = fs.readFileSync(path.resolve(import.meta.dirname, '..', 'build', 'fuses', 'fuses.json5'), 'utf-8');
  // Same parsing as build/fuses/build.py: drop comment lines, then parse as JSON.
  const defaults: Record<string, string> = JSON.parse(
    json5
      .split('\n')
      .filter((line) => !line.trim().startsWith('/'))
      .join('\n')
  );
  return Object.entries(defaults)
    .filter(([key]) => !key.startsWith('_'))
    .map(([, state]) => state)
    .join('');
})();

// Tries to fork a child process from the main process of a remote-control app
// and returns the error message, or 'forked' if it worked.
function forkFromApp(rc: Awaited<ReturnType<typeof startRemoteControlApp>>) {
  return rc.remotely((fixture: string) => {
    try {
      require('node:child_process').fork(fixture);
      return 'forked';
    } catch (error) {
      return (error as Error).message;
    }
  }, noopApp);
}

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

ifdescribe(isTestingBindingAvailable())('fuses', { retry: 0 }, () => {
  it('can be enabled by command-line argument during testing', async () => {
    // Testing builds keep node_options on (see build/args/testing.gn), so
    // NODE_OPTIONS is honored. Should exit with 9 because -e is not allowed in
    // NODE_OPTIONS
    const child0 = spawn(process.execPath, ['-v'], { env: { NODE_OPTIONS: '-e 0' } });
    const [code0] = await once(child0, 'exit');
    expect(code0).toBe(9);
    const child1 = spawn(process.execPath, ['--set-fuse-node_options=0', '-v'], { env: { NODE_OPTIONS: '-e 0' } });
    const [code1] = await once(child1, 'exit');
    // Should print the version and exit with 0
    expect(code1).toBe(0);
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

  it('makes child_process.fork throw when run_as_node is 0', async () => {
    const rc = await startRemoteControlApp(['--set-fuse-run_as_node=0']);
    expect(await forkFromApp(rc)).toContain('runAsNode fuse is disabled');
  });

  // Testing builds turn some fuses on that release builds ship off, and the
  // --set-fuse-* switches are read too late for run_as_node, so check the
  // shipped defaults on a copy of the app with the release fuse wire.
  // A copied ASan build can't start (the sanitizer runtime isn't part of the dist).
  ifdescribe(!process.env.IS_ASAN)('with the release fuse wire', () => {
    let tmpDir: string;
    let appPath: string;
    let exePath: string;

    beforeAll(async () => {
      tmpDir = await fs.promises.mkdtemp(path.resolve(os.tmpdir(), 'electron-fuses-spec-'));
      appPath = await copyApp(tmpDir);
      exePath = process.platform === 'darwin' ? path.resolve(appPath, 'Contents/MacOS/Electron') : appPath;
      // @electron/fuses can only write 0 and 1. Fuses with other states (such
      // as node_cli_inspect's 'd') are left as built, and testing builds don't
      // override those.
      const fuses: Partial<Record<FuseV1Options, boolean>> = {};
      for (const [index, state] of [...releaseFuseWire].entries()) {
        if (state === '0' || state === '1') fuses[index as FuseV1Options] = state === '1';
      }
      await flipFuses(appPath, {
        version: FuseVersion.V1,
        resetAdHocDarwinSignature: true,
        ...fuses
      } as FuseV1Config<boolean>);
    }, 180000);

    afterAll(async () => {
      if (tmpDir) {
        await originalFs.promises.rm(tmpDir, { recursive: true, force: true, maxRetries: 5, retryDelay: 500 });
      }
    }, 180000);

    it('matches build/fuses/fuses.json5', async () => {
      const config = await getCurrentFuseWire(appPath);
      const wire = Array.from(releaseFuseWire, (_, index) => String.fromCharCode(config[index as FuseV1Options]!));
      expect(wire.join('')).toBe(releaseFuseWire);
    });

    it('makes child_process.fork throw by default', async () => {
      const rc = await startRemoteControlApp([], undefined, exePath);
      expect(await forkFromApp(rc)).toContain('runAsNode fuse is disabled');
    });

    it('ignores ELECTRON_RUN_AS_NODE by default', async () => {
      const child = spawn(exePath, [path.join(import.meta.dirname, 'fixtures', 'module', 'print-process-type.js')], {
        env: { ...process.env, ELECTRON_RUN_AS_NODE: '1' }
      });
      defer(() => {
        if (child.exitCode === null && child.signalCode === null) child.kill();
      });
      let stdout = '';
      child.stdout.on('data', (data) => {
        stdout += data;
      });
      const [code] = await once(child, 'close');
      expect(code).toBe(0);
      // Runs as the main process of an app instead of as Node.js, where
      // process.type is undefined.
      expect(stdout).toMatch(/^browser$/m);
    });

    it('ignores NODE_OPTIONS by default', async () => {
      const child = spawn(exePath, ['-v'], { env: { NODE_OPTIONS: '-e 0' } });
      const [code] = await once(child, 'exit');
      // NODE_OPTIONS is ignored, so this should print the version and exit with
      // 0 instead of 9 for -e not being allowed in NODE_OPTIONS
      expect(code).toBe(0);
    });
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
