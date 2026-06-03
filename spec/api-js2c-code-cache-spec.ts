import { expect } from 'chai';

import * as childProcess from 'node:child_process';
import * as path from 'node:path';

import { ifdescribe, isTestingBindingAvailable } from './lib/spec-helpers';

// Asserts the build-time V8 code cache for each electron/js2c/* bundle is
// consumed (not compiled from source) in every process type. Runs in a
// separately-spawned app: spec/index.js injects --js-flags=--expose_gc
// suite-wide which changes V8's FlagList::Hash, so the production-flavor
// cache would (correctly) be rejected if run inside the spec runner.
type Status = Record<string, boolean>;

const APP = path.resolve(__dirname, 'fixtures/api/js2c-code-cache/app');

function expectConsumed(status: Status, id: string) {
  expect(status, `${id} should be present (compiled in this process)`).to.have.property(id);
  expect(status[id], `${id} build-time code cache must be CONSUMED (accepted), not rejected/absent`).to.equal(true);
}

ifdescribe(isTestingBindingAvailable())('js2c build-time code cache', () => {
  it('is consumed across browser / sandboxed renderer / renderer / utility', async () => {
    const out = await new Promise<string>((resolve, reject) => {
      const child = childProcess.spawn(process.execPath, [APP], { stdio: ['ignore', 'pipe', 'inherit'] });
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
    const r = JSON.parse(m![1]) as { browser: Status; sandbox: Status; renderer: Status; utility: Status };

    expectConsumed(r.browser, 'electron/js2c/browser_init');
    expectConsumed(r.browser, 'electron/js2c/node_init');

    expectConsumed(r.sandbox, 'electron/js2c/sandbox_bundle');

    expectConsumed(r.renderer, 'electron/js2c/renderer_init');
    expectConsumed(r.renderer, 'electron/js2c/node_init');

    expectConsumed(r.utility, 'electron/js2c/utility_init');
    expectConsumed(r.utility, 'electron/js2c/node_init');
  });
});
