import { flipFuses, FuseV1Options, FuseVersion } from '@electron/fuses';

import { expect } from 'chai';

import * as childProcess from 'node:child_process';
import * as fs from 'node:fs';
import * as originalFs from 'node:original-fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { copyApp } from './lib/fs-helpers';
import { ifdescribe, isTestingBindingAvailable } from './lib/spec-helpers';

// Asserts the build-time V8 code cache for each internal/electron/js2c/* bundle -- and
// for Node's own builtins, from the embedded Node snapshot in the browser
// process and from the build-time cache everywhere else -- is consumed (not
// compiled from source) in every process type. Runs in a separately-spawned
// app: spec/index.js injects --js-flags=--expose_gc suite-wide which changes
// V8's FlagList::Hash, so the production-flavor cache would (correctly) be
// rejected if run inside the spec runner.
type Status = Record<string, boolean>;
type Result = { browser: Status; sandbox: Status; renderer: Status; utility: Status; runAsNode: Status };

const APP = path.resolve(__dirname, 'fixtures/api/js2c-code-cache/app');

async function runFixtureApp(execPath: string): Promise<Result> {
  const out = await new Promise<string>((resolve, reject) => {
    const child = childProcess.spawn(execPath, [APP], { stdio: ['ignore', 'pipe', 'inherit'] });
    let stdout = '';
    child.stdout.on('data', (d) => {
      stdout += d.toString();
    });
    const to = setTimeout(() => {
      child.kill('SIGKILL');
      reject(new Error('fixture app timed out\n' + stdout));
    }, 90000);
    child.on('exit', (code) => {
      clearTimeout(to);
      if (code !== 0) reject(new Error(`fixture app exited ${code}\n${stdout}`));
      else resolve(stdout);
    });
  });
  const m = /JS2C_RESULT (.+)/.exec(out);
  expect(m, `fixture app did not report a result. Output:\n${out}`).to.not.equal(null);
  return JSON.parse(m![1]);
}

function expectConsumed(status: Status, id: string) {
  expect(status, `${id} should be present (compiled in this process)`).to.have.property(id);
  expect(status[id], `${id} build-time code cache must be CONSUMED (accepted), not rejected/absent`).to.equal(true);
}

// Every Node builtin the process compiled so far must have consumed a cache
// entry. internal/deps/* (undici, acorn, ...) are deliberately not cached --
// nothing loads them during startup -- so they are exempt if app code happened
// to pull one in.
function expectNodeBuiltinsConsumed(status: Status, processType: string) {
  const nodeIds = Object.keys(status).filter(
    (id) => !id.startsWith('internal/electron/js2c/') && !id.startsWith('internal/deps/')
  );
  expect(nodeIds.length, `${processType} should have compiled some Node builtins`).to.be.greaterThan(20);
  const notConsumed = nodeIds.filter((id) => !status[id]);
  expect(notConsumed, `${processType}: Node builtins compiled WITHOUT consuming the code cache`).to.deep.equal([]);
}

ifdescribe(isTestingBindingAvailable())('js2c build-time code cache', () => {
  it('is consumed across browser / sandboxed renderer / renderer / utility / run-as-node', async () => {
    const r = await runFixtureApp(process.execPath);

    expectConsumed(r.browser, 'internal/electron/js2c/browser_init');
    expectConsumed(r.browser, 'internal/electron/js2c/node_init');

    expectConsumed(r.sandbox, 'internal/electron/js2c/sandbox_bundle');

    expectConsumed(r.renderer, 'internal/electron/js2c/renderer_init');
    expectConsumed(r.renderer, 'internal/electron/js2c/node_init');

    expectConsumed(r.utility, 'internal/electron/js2c/utility_init');
    expectConsumed(r.utility, 'internal/electron/js2c/node_init');

    expectConsumed(r.runAsNode, 'internal/electron/js2c/node_init');

    for (const processType of ['browser', 'renderer', 'utility', 'runAsNode'] as const) {
      expectNodeBuiltinsConsumed(r[processType], processType);
    }
  });

  // A copied ASan build can't start (the sanitizer runtime isn't part of the dist).
  ifdescribe(!process.env.IS_ASAN)('with the LoadBrowserProcessSpecificV8Snapshot fuse enabled', function () {
    this.timeout(180000);

    let tmpDir: string;
    let appPath: string;

    beforeEach(async () => {
      tmpDir = await fs.promises.mkdtemp(path.resolve(os.tmpdir(), 'electron-js2c-fuse-spec-'));
      appPath = await copyApp(tmpDir);
      const assetsDir =
        process.platform === 'darwin'
          ? path.resolve(appPath, 'Contents/Frameworks/Electron Framework.framework/Resources')
          : path.dirname(appPath);
      const builtInBlob = fs.readdirSync(assetsDir).find((f) => /^v8_context_snapshot.*\.bin$/.test(f));
      expect(builtInBlob, `no v8 context snapshot in ${assetsDir}`).to.be.a('string');
      fs.copyFileSync(
        path.resolve(assetsDir, builtInBlob!),
        path.resolve(assetsDir, 'browser_v8_context_snapshot.bin')
      );
      await flipFuses(appPath, {
        version: FuseVersion.V1,
        resetAdHocDarwinSignature: true,
        [FuseV1Options.LoadBrowserProcessSpecificV8Snapshot]: true
      });
    });

    afterEach(async () => {
      await originalFs.promises.rm(tmpDir, { recursive: true, force: true, maxRetries: 5, retryDelay: 500 });
    });

    it('is still consumed in every process, the browser one included while its blob matches the built-in one', async () => {
      const r = await runFixtureApp(
        process.platform === 'darwin' ? path.resolve(appPath, 'Contents/MacOS/Electron') : appPath
      );
      expectConsumed(r.utility, 'internal/electron/js2c/utility_init');
      expectConsumed(r.runAsNode, 'internal/electron/js2c/node_init');
      for (const processType of ['browser', 'renderer', 'utility', 'runAsNode'] as const) {
        expectNodeBuiltinsConsumed(r[processType], processType);
      }
    });
  });
});
