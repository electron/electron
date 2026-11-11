// Run with: node --test script/benchmarks/test/*.test.mts
import assert from 'node:assert/strict';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import { after, describe, it } from 'node:test';
import { fileURLToPath } from 'node:url';

import { runCrossbench } from '../run.mts';

const FIXTURE = fileURLToPath(new URL('fixtures/spawn-descendant.mts', import.meta.url));
const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'benchmarks-run-test-'));
after(() => fs.rmSync(tmp, { recursive: true, force: true }));

// A zombie (killed, not yet reaped by init) counts as gone.
function alive(pid: number): boolean {
  try {
    const stat = fs.readFileSync(`/proc/${pid}/stat`, 'utf8');
    return stat.slice(stat.lastIndexOf(')') + 2)[0] !== 'Z';
  } catch (error) {
    if ((error as NodeJS.ErrnoException).code !== 'ENOENT' || fs.existsSync('/proc/self')) return false;
  }
  try {
    process.kill(pid, 0);
    return true;
  } catch {
    return false;
  }
}

describe('runCrossbench', { skip: process.platform === 'win32' && 'uses POSIX process groups' }, () => {
  it('kills the whole process tree on timeout', async () => {
    const pidFile = path.join(tmp, 'timeout.pid');
    const code = await runCrossbench(process.execPath, [FIXTURE, pidFile, 'hang'], path.join(tmp, 'timeout.log'), 500, {
      graceMs: 300
    });
    assert.equal(code, 124);
    const pid = Number(fs.readFileSync(pidFile, 'utf8'));
    assert.equal(alive(pid), false, `descendant ${pid} is still running`);
  });

  it('kills descendants left behind by a normal exit', async () => {
    const pidFile = path.join(tmp, 'exit.pid');
    const code = await runCrossbench(process.execPath, [FIXTURE, pidFile, 'exit'], path.join(tmp, 'exit.log'), 10_000, {
      graceMs: 300
    });
    assert.equal(code, 0);
    const pid = Number(fs.readFileSync(pidFile, 'utf8'));
    assert.equal(alive(pid), false, `descendant ${pid} is still running`);
  });

  it('reports a command that cannot start as a failure', async () => {
    const code = await runCrossbench(path.join(tmp, 'missing'), [], path.join(tmp, 'missing.log'), 10_000);
    assert.notEqual(code, 0);
  });
});
