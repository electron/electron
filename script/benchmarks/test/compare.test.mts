// Run with: node --test script/benchmarks/test/*.test.mts
import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import { after, describe, it } from 'node:test';
import { fileURLToPath } from 'node:url';

import { collectSamples, compareTargets, datadogSeries, findProblems } from '../compare.mts';

import type { RunMetadata, TargetInfo } from '../lib/types.mts';

const COMPARE = fileURLToPath(new URL('../compare.mts', import.meta.url));
const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'benchmarks-compare-test-'));
after(() => fs.rmSync(tmp, { recursive: true, force: true }));

const TARGETS: TargetInfo[] = [
  { id: 'candidate', kind: 'electron', electronVersion: '47.0.0', chromiumVersion: '158.0.1.0' },
  { id: 'baseline', kind: 'electron', electronVersion: '46.0.0', chromiumVersion: '157.0.1.0' },
  { id: 'cft', kind: 'chrome', chromiumVersion: '158.0.1.0' }
];

// Writes a results dir laid out like run.mts and crossbench leave it. Each
// invocation gets a first_load story whose fcp has `values` (default: one
// value per repetition) unless `values` is null.
interface TestInvocation {
  round: number;
  target: string;
  exitCode?: number;
  values?: number[] | null;
}

function writeResults(
  name: string,
  invocations: TestInvocation[],
  { repeat = 3, targets = TARGETS }: { repeat?: number; targets?: TargetInfo[] } = {}
) {
  const dir = path.join(tmp, name);
  fs.mkdirSync(dir, { recursive: true });
  const metadata: RunMetadata = {
    startedAt: '2026-10-29T00:00:00.000Z',
    platform: 'linux',
    arch: 'x64',
    cpuModel: 'test cpu',
    cpuCount: 4,
    runnerName: 'test',
    crossbenchRevision: '0123456789abcdef',
    rounds: 1 + Math.max(...invocations.map((i) => i.round)),
    repeat,
    targets,
    invocations: []
  };
  for (const { round, target, exitCode = 0, values } of invocations) {
    const relativeDir = path.join('rounds', String(round), 'loading', target);
    metadata.invocations.push({ round, suite: 'loading', target, exitCode, repetitions: repeat, dir: relativeDir });
    if (values === null) continue;
    const storyDir = path.join(dir, relativeDir, `chromium_${target}`, 'stories', 'first_load');
    fs.mkdirSync(storyDir, { recursive: true });
    const fcp = values ?? Array.from({ length: repeat }, (_, i) => 100 + round + i);
    fs.writeFileSync(path.join(storyDir, 'js.json'), JSON.stringify({ fcp: { values: fcp } }));
  }
  fs.writeFileSync(path.join(dir, 'metadata.json'), JSON.stringify(metadata));
  return { dir, metadata };
}

const everyTarget = (round: number, overrides: Record<string, Partial<TestInvocation>> = {}): TestInvocation[] =>
  TARGETS.map(({ id }) => ({ round, target: id, ...overrides[id] }));

const runCompare = (dir: string) =>
  spawnSync(process.execPath, [COMPARE, '--results', dir, '--dry-run'], {
    encoding: 'utf8',
    env: { ...process.env, GITHUB_STEP_SUMMARY: '', GITHUB_ACTIONS: '' }
  });

describe('collectSamples', () => {
  it('takes one sample per round and target from successful invocations', () => {
    const { dir, metadata } = writeResults('ok', [...everyTarget(0), ...everyTarget(1)]);
    const { samples, rejected } = collectSamples(dir, metadata);
    assert.deepEqual(samples['loading.first_load.fcp'], {
      candidate: [101, 102],
      baseline: [101, 102],
      cft: [101, 102]
    });
    assert.deepEqual(rejected, []);
  });

  it("leaves out a failed invocation's results", () => {
    const { dir, metadata } = writeResults('failed', [
      ...everyTarget(0),
      ...everyTarget(1, { candidate: { exitCode: 1, values: [500, 500, 500] } })
    ]);
    const { samples, rejected } = collectSamples(dir, metadata);
    assert.deepEqual(samples['loading.first_load.fcp'].candidate, [101]);
    assert.deepEqual(rejected, [
      { round: 1, suite: 'loading', target: 'candidate', reason: 'crossbench exited with 1' }
    ]);
  });

  it('leaves out measures with missing repetitions', () => {
    const { dir, metadata } = writeResults('incomplete', [
      ...everyTarget(0),
      ...everyTarget(1, { baseline: { values: [90, 91] } })
    ]);
    const { samples, rejected } = collectSamples(dir, metadata);
    assert.deepEqual(samples['loading.first_load.fcp'].baseline, [101]);
    assert.equal(rejected.length, 1);
    assert.match(rejected[0].reason, /loading\.first_load\.fcp \(2\/3 values\)/);
  });
});

describe('findProblems', () => {
  const problemsFor = (name: string, invocations: TestInvocation[], options?: { targets?: TargetInfo[] }) => {
    const { dir, metadata } = writeResults(name, invocations, options);
    const { samples } = collectSamples(dir, metadata);
    const comparisons = compareTargets(samples, metadata.targets, { alpha: 0.05, minChange: 0.1 });
    return findProblems(samples, comparisons, metadata.targets);
  };

  it('accepts results with every comparison', () => {
    assert.deepEqual(problemsFor('complete', everyTarget(0)), []);
  });

  it('reports a required comparison without results', () => {
    const problems = problemsFor('no-cft', everyTarget(0, { cft: { exitCode: 1, values: null } }));
    assert.deepEqual(problems, ['vs_cft: no metric has results for both cft and candidate']);
  });

  it('does not require Chrome for Testing when it was skipped', () => {
    assert.deepEqual(problemsFor('skipped-cft', everyTarget(0).slice(0, 2), { targets: TARGETS.slice(0, 2) }), []);
  });
});

describe('datadogSeries', () => {
  it('stamps points at upload time for a run that never finished', () => {
    // run.mts was killed, so metadata.json has startedAt (hours ago) but no
    // finishedAt.
    const { dir, metadata } = writeResults('unfinished', everyTarget(0));
    assert.equal(metadata.finishedAt, undefined);
    const { samples } = collectSamples(dir, metadata);
    const comparisons = compareTargets(samples, metadata.targets, { alpha: 0.05, minChange: 0.1 });
    const before = Math.floor(Date.now() / 1000);
    const series = datadogSeries({ metadata, samples, comparisons });
    const after = Math.ceil(Date.now() / 1000);
    assert.ok(series.length > 0);
    for (const { points } of series) {
      for (const { timestamp } of points) {
        assert.ok(timestamp >= before && timestamp <= after, `${timestamp} is not between ${before} and ${after}`);
      }
    }
  });
});

describe('compare.mts', () => {
  it('exits with 0 for complete results', () => {
    const { dir } = writeResults('cli-ok', [...everyTarget(0), ...everyTarget(1)]);
    const result = runCompare(dir);
    assert.equal(result.status, 0, result.stderr);
  });

  it('exits non-zero when every invocation failed', () => {
    const { dir } = writeResults('cli-all-failed', [
      ...everyTarget(0, Object.fromEntries(TARGETS.map(({ id }) => [id, { exitCode: 1, values: null }]))),
      ...everyTarget(1, Object.fromEntries(TARGETS.map(({ id }) => [id, { exitCode: 1, values: null }])))
    ]);
    const result = runCompare(dir);
    assert.equal(result.status, 1);
    assert.match(result.stderr, /no usable benchmark results/);
    // The diagnostics are still written.
    assert.ok(fs.existsSync(path.join(dir, 'comparison.json')));
    assert.ok(fs.existsSync(path.join(dir, 'summary.md')));
  });
});
