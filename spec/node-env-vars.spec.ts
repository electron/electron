import { flipFuses, FuseV1Options, FuseVersion } from '@electron/fuses';

import * as originalFs from 'original-fs';
import { afterAll, beforeAll, describe, expect, it } from 'vitest';

import { spawn } from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { copyApp } from './lib/fs-helpers.ts';
import { ciGpuArgs, ifdescribe } from './lib/spec-helpers.ts';

const fixture = path.join(import.meta.dirname, 'fixtures', 'apps', 'node-env-vars');

interface Report {
  env: string[];
  compileCache: boolean;
}

type AppResult = Record<'main' | 'worker' | 'utility' | 'utilityWithEnv' | 'renderer', Report> & {
  enabledByApp: boolean;
};

describe('Node.js environment variables', { timeout: 120000 }, () => {
  let dir: string;
  let gated: Record<string, string>;

  beforeAll(async () => {
    dir = await fs.promises.mkdtemp(path.join(os.tmpdir(), 'electron-node-env-vars-'));
    gated = {
      NODE_COMPILE_CACHE: path.join(dir, 'cache'),
      NODE_COMPILE_CACHE_PORTABLE: '1',
      NODE_COMPILE_CACHE_READONLY: '0',
      NODE_EXTRA_CA_CERTS: path.join(dir, 'missing.pem')
    };
  });

  afterAll(async () => {
    await originalFs.promises.rm(dir, { recursive: true, force: true, maxRetries: 5, retryDelay: 500 });
  });

  const run = async (exe: string, args: string[], vars: Record<string, string>) => {
    const env: NodeJS.ProcessEnv = {};
    for (const [name, value] of Object.entries(process.env)) {
      if (!name.startsWith('NODE_')) env[name] = value;
    }
    Object.assign(env, vars, {
      TEST_APP_VARS: JSON.stringify(gated),
      TEST_CACHE_DIR: path.join(dir, 'app-cache')
    });
    const child = spawn(exe, args, { env });
    let stdout = '';
    let stderr = '';
    child.stdout.on('data', (chunk) => {
      stdout += chunk;
    });
    child.stderr.on('data', (chunk) => {
      stderr += chunk;
    });
    const [code] = await once(child, 'close');
    expect(code, stderr).toBe(0);
    const line = stdout.split(/\r?\n/).find((l) => l.startsWith('{'));
    expect(line, `no report in ${stdout}\n${stderr}`).toBeDefined();
    return JSON.parse(line!);
  };
  const runApp = (exe: string, vars: Record<string, string>): Promise<AppResult> =>
    run(exe, [fixture, ...ciGpuArgs], vars);
  const runAsNode = (exe: string, vars: Record<string, string>): Promise<Report> =>
    run(exe, [path.join(fixture, 'report.js')], { ...vars, ELECTRON_RUN_AS_NODE: '1' });

  describe('with the nodeOptions fuse enabled', () => {
    it('are respected', async () => {
      const result = await runApp(process.execPath, gated);
      const all = Object.keys(gated);
      for (const type of ['main', 'worker', 'utility', 'utilityWithEnv', 'renderer'] as const) {
        expect(result[type].env, type).toEqual(expect.arrayContaining(all));
        expect(result[type].compileCache, type).toBe(true);
      }
    });

    it('are respected with ELECTRON_RUN_AS_NODE', async () => {
      const result = await runAsNode(process.execPath, gated);
      expect(result.env).toEqual(expect.arrayContaining(Object.keys(gated)));
      expect(result.compileCache).toBe(true);
    });

    it('do not include NODE_COMPILE_CACHE in packaged apps', async () => {
      fs.rmSync(gated.NODE_COMPILE_CACHE, { recursive: true, force: true });
      const result = await runApp(process.execPath, { ...gated, ELECTRON_FORCE_IS_PACKAGED: '1' });
      for (const type of ['main', 'worker', 'utility', 'utilityWithEnv', 'renderer'] as const) {
        expect(result[type].env, type).not.toContain('NODE_COMPILE_CACHE');
        expect(result[type].env, type).toContain('NODE_EXTRA_CA_CERTS');
        expect(result[type].compileCache, type).toBe(false);
      }
      expect(fs.existsSync(gated.NODE_COMPILE_CACHE)).toBe(false);
      expect(result.enabledByApp).toBe(true);
    });
  });

  // A copied ASan build can't start (the sanitizer runtime isn't part of the dist).
  ifdescribe(!process.env.IS_ASAN)('with the nodeOptions fuse disabled', () => {
    let appDir: string;
    let exe: string;

    // --set-fuse-node_options is not passed on to child processes, so flip the
    // fuse in a copy of the app.
    beforeAll(async () => {
      appDir = await fs.promises.mkdtemp(path.join(os.tmpdir(), 'electron-node-env-vars-app-'));
      const appPath = await copyApp(appDir);
      await flipFuses(appPath, {
        version: FuseVersion.V1,
        resetAdHocDarwinSignature: true,
        [FuseV1Options.EnableNodeOptionsEnvironmentVariable]: false
      });
      exe = process.platform === 'darwin' ? path.resolve(appPath, 'Contents/MacOS/Electron') : appPath;
    }, 180000);

    afterAll(async () => {
      await originalFs.promises.rm(appDir, { recursive: true, force: true, maxRetries: 5, retryDelay: 500 });
    }, 180000);

    it('are ignored', async () => {
      fs.rmSync(gated.NODE_COMPILE_CACHE, { recursive: true, force: true });
      const result = await runApp(exe, gated);
      for (const type of ['main', 'worker', 'utility', 'utilityWithEnv', 'renderer'] as const) {
        expect(result[type], type).toEqual({ env: [], compileCache: false });
      }
      expect(fs.existsSync(gated.NODE_COMPILE_CACHE)).toBe(false);
    });

    it('do not prevent module.enableCompileCache()', async () => {
      const { enabledByApp } = await runApp(exe, {});
      expect(enabledByApp).toBe(true);
    });

    it('are ignored with ELECTRON_RUN_AS_NODE', async () => {
      const result = await runAsNode(exe, gated);
      expect(result).toEqual({ env: [], compileCache: false });
    });
  });
});
