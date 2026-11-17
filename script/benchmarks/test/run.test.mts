// Run with: node --test script/benchmarks/test/*.test.mts
import assert from 'node:assert/strict';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import { after, describe, it } from 'node:test';
import { fileURLToPath } from 'node:url';

import { capLogs, failedStories, runCrossbench } from '../run.mts';

import type { Invocation } from '../lib/types.mts';

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

describe('capLogs', () => {
  it('cuts the middle out of logs over the limit', () => {
    const dir = path.join(tmp, 'logs');
    fs.mkdirSync(path.join(dir, 'story', '0'), { recursive: true });
    const big = path.join(dir, 'story', '0', 'browser.stdout.log');
    fs.writeFileSync(big, `${'a'.repeat(100)}${'b'.repeat(1000)}${'c'.repeat(100)}`);
    const small = path.join(dir, 'story', '0', 'cb.log');
    fs.writeFileSync(small, 'x'.repeat(500));
    const data = path.join(dir, 'story', '0', 'trace.json');
    fs.writeFileSync(data, 'y'.repeat(2000));
    capLogs(dir, { maxBytes: 1000, keepBytes: 100 });
    const cut = fs.readFileSync(big, 'utf8');
    assert.ok(cut.startsWith('a'.repeat(100)));
    assert.ok(cut.endsWith('c'.repeat(100)));
    assert.match(cut, /cut 1000 bytes/);
    assert.ok(!cut.includes('bb'));
    assert.equal(fs.statSync(small).size, 500);
    assert.equal(fs.statSync(data).size, 2000);
  });
});

describe('failedStories', () => {
  const STORIES = ['file_classic', 'file_module', 'register_file_protocol_classic'];

  // One file_loading invocation of the candidate in `round` that exited with
  // `exitCode`, with three files-loaded values for each story in `complete`
  // and one value for each story in `partial`.
  function invocation(
    outDir: string,
    round: number,
    exitCode: number,
    complete: string[],
    partial: string[] = []
  ): Invocation {
    const dir = path.join('rounds', String(round), 'file_loading', 'candidate');
    for (const [stories, values] of [
      [complete, [10, 11, 12]],
      [partial, [10]]
    ] as const) {
      for (const story of stories) {
        const storyDir = path.join(outDir, dir, 'chromium_candidate', 'stories', story);
        fs.mkdirSync(storyDir, { recursive: true });
        fs.writeFileSync(
          path.join(storyDir, 'performance.entries.json'),
          JSON.stringify({ 'mark/files-loaded/startTime': { values } })
        );
      }
    }
    return { round, suite: 'file_loading', target: 'candidate', exitCode, repetitions: 3, dir };
  }

  it('returns the stories without complete results in either of the first two rounds', () => {
    const outDir = path.join(tmp, 'failed-stories');
    const invocations = [
      invocation(outDir, 0, 3, ['file_classic'], ['file_module']),
      // A baseline failure does not count.
      { round: 1, suite: 'file_loading', target: 'baseline', exitCode: 3, repetitions: 3, dir: 'missing' },
      invocation(outDir, 1, 3, ['register_file_protocol_classic'], ['file_module'])
    ];
    assert.deepEqual(failedStories(outDir, 'file_loading', STORIES, invocations), ['file_module']);
  });

  it('counts a story crossbench reported errors for as failed', () => {
    const outDir = path.join(tmp, 'failed-stories-errors');
    const invocations = [invocation(outDir, 0, 3, STORIES), invocation(outDir, 1, 3, STORIES)];
    for (const run of invocations) {
      fs.writeFileSync(
        path.join(outDir, run.dir, 'chromium_candidate', 'stories', 'file_module', 'cb.errors.json'),
        JSON.stringify([{ type: 'builtins.TimeoutError', title: 'Waited for 0:01:00.1' }])
      );
    }
    assert.deepEqual(failedStories(outDir, 'file_loading', STORIES, invocations), ['file_module']);
  });

  it('returns every story when no story produced results', () => {
    const outDir = path.join(tmp, 'failed-stories-none');
    const invocations = [invocation(outDir, 0, 3, []), invocation(outDir, 1, 124, [])];
    assert.deepEqual(failedStories(outDir, 'file_loading', STORIES, invocations), STORIES);
  });

  it('returns nothing unless the candidate failed in both rounds', () => {
    const outDir = path.join(tmp, 'failed-stories-flake');
    const invocations = [invocation(outDir, 0, 3, []), invocation(outDir, 1, 0, STORIES)];
    assert.equal(failedStories(outDir, 'file_loading', STORIES, invocations), undefined);
  });
});
