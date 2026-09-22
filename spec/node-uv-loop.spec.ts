import { expect } from 'chai';

import * as childProcess from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { defer, isTestingBindingAvailable } from './lib/spec-helpers.ts';

const fixture = path.resolve(import.meta.dirname, 'fixtures', 'apps', 'uv-wake');

type Case = { proc: string; context: string; op: string; budget: number; samples: (number | string)[] };

async function runMatrix(args: string[] = []): Promise<Case[]> {
  const out = path.join(fs.mkdtempSync(path.join(os.tmpdir(), 'uv-wake-spec-')), 'results.json');
  const child = childProcess.spawn(
    process.execPath,
    [fixture, `--out=${out}`, `--iterations=${process.env.UV_WAKE_ITERATIONS || 3}`, ...args],
    { stdio: 'inherit' }
  );
  defer(() => {
    if (child.exitCode === null) child.kill();
  });
  const [code] = await once(child, 'close');
  expect(code).to.equal(0);
  const { cases } = JSON.parse(fs.readFileSync(out, 'utf8'));
  fs.rmSync(path.dirname(out), { recursive: true });
  expect(cases).to.not.be.empty();
  return cases;
}

// Cases whose trigger the platform could not produce report 'no-trigger', and
// ops that cannot complete inside a nested loop report 'skipped'; neither fails.
function late(cases: Case[]) {
  return cases
    .filter((c) => c.samples[0] !== 'no-trigger' && c.samples[0] !== 'skipped')
    .filter((c) => c.samples.some((s) => typeof s !== 'number' || s > c.budget))
    .map((c) => `${c.proc} ${c.context} -> ${c.op}: [${c.samples.join(', ')}] (budget ${c.budget} ms)`);
}

// Node's loop only runs when Electron's embed thread notices it has work. These
// start uv-backed operations from every kind of JS entry an otherwise idle
// process has and check each completes about as fast as it would in Node.
describe('uv loop integration', function () {
  this.timeout(5 * 60 * 1000);

  it('completes uv work started from an idle browser process and nodeIntegration renderer on time', async () => {
    const cases = await runMatrix();
    const failures = late(cases);
    expect(failures, failures.join('\n')).to.be.empty();
    // Testing builds can always produce these entries; make sure they ran.
    if (isTestingBindingAvailable()) {
      const untriggered = cases.filter(
        (c) =>
          [
            'nested-loop',
            'started-then-nested',
            'native-event',
            'native-addon-call',
            'native-event-in-nested-loop'
          ].includes(c.context) && c.samples[0] === 'no-trigger'
      );
      expect(untriggered.map((c) => `${c.context} -> ${c.op}`)).to.be.empty();
    }
  });

  it('completes uv work started from an idle isolated-world preload on time', async () => {
    const failures = late(await runMatrix(['--procs=renderer', '--isolated']));
    expect(failures, failures.join('\n')).to.be.empty();
  });
});
